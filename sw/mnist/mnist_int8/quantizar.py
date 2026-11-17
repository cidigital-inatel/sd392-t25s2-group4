"""Quantização pós-treinamento da CNN MNIST. Dependência: numpy."""
import argparse
import gzip
import json
import struct
from pathlib import Path

import numpy as np
from numpy.lib.stride_tricks import sliding_window_view

Q = 30
CHAVES = ('camadas.0', 'camadas.3', 'camadas.7')


def ler_idx(p):
    with gzip.open(p, 'rb') if str(p).endswith('.gz') else open(p, 'rb') as f:
        magic, n = struct.unpack('>II', f.read(8))
        if magic == 2051:
            h, w = struct.unpack('>II', f.read(8))
            return np.frombuffer(f.read(), np.uint8).reshape(n, h, w)
        if magic == 2049:
            return np.frombuffer(f.read(), np.uint8).reshape(n)
        raise ValueError('Arquivo IDX inválido')


def conv(x, w, b):
    # NCHW; padding zero, stride 1; kernels armazenados em OIHW.
    j = sliding_window_view(np.pad(x, ((0,0),(0,0),(1,1),(1,1))), (3,3), axis=(2,3))
    n, c, h, l, _, _ = j.shape
    cols = j.transpose(0,2,3,1,4,5).reshape(-1, c*9)
    return (cols @ w.reshape(len(w), -1).T + b).reshape(n,h,l,len(w)).transpose(0,3,1,2)


def pool(x):
    n,c,h,w = x.shape
    return x.reshape(n,c,h//2,2,w//2,2).max(axis=(3,5))


def flutuante(img, parametros):
    x = img[:,None].astype(np.float32) / 255
    for w,b in parametros[:2]:
        x = pool(np.maximum(conv(x,w,b), 0))
    w,b = parametros[2]
    return x.reshape(len(x), -1) @ w.T + b


def inferir(img, m):
    # Conversão exata em inteiros: round(pixel * 127 / 255).
    x = ((img[:,None].astype(np.int32)*127 + 127)//255).astype(np.int8)
    for i in (1,2):
        acc = conv(x.astype(np.int32), m[f'w{i}'].astype(np.int32), m[f'b{i}'])
        # ReLU antes da requantização. Empate arredonda para cima.
        y = (np.maximum(acc,0).astype(np.int64)*int(m[f'm{i}']) + (1<<(Q-1))) >> Q
        x = pool(np.minimum(y,127).astype(np.int8))
    # Escala de saída comum às 10 classes: argmax direto no acumulador int32.
    return x.reshape(len(x), -1).astype(np.int32) @ m['w3'].astype(np.int32).T + m['b3']


def quantizar(parametros, imgs):
    # A calibração usa somente imagens do treino, sem consultar rótulos.
    maximos = np.zeros(2)
    for k in range(0,len(imgs),64):
        x = imgs[k:k+64,None].astype(np.float32)/255
        for i,(w,b) in enumerate(parametros[:2]):
            x = pool(np.maximum(conv(x,w,b),0))
            maximos[i] = max(maximos[i], float(x.max()))
    escalas = [1/127, *(np.maximum(maximos, 1e-8)/127)]
    m = {'fracao':np.int32(Q), 'escalas_ativacao':np.array(escalas)}
    for i,(w,b) in enumerate(parametros,1):
        sw = max(float(np.abs(w).max()),1e-8)/127
        sa = escalas[i-1]*sw
        qw = np.rint(w.astype(np.float64)/sw).clip(-127,127).astype(np.int8)
        qb = np.rint(b.astype(np.float64)/sa)
        limite = 127*np.abs(qw.astype(np.int64)).reshape(len(qw),-1).sum(1)+np.abs(qb)
        assert np.all(limite <= np.iinfo(np.int32).max), 'Acumulador int32 insuficiente'
        m.update({f'w{i}':qw, f'b{i}':qb.astype(np.int32), f'sw{i}':sw,
                  f'limite_acc{i}':int(limite.max())})
        if i<3:
            mul = round(sa/escalas[i]*(1<<Q))
            assert 0 < mul <= np.iinfo(np.int32).max
            m[f'm{i}'] = np.int32(mul)
        else:
            m['escala_logits'] = sa
    return m


def cabecalho(m, arq):
    def vetor(nome, tipo, a):
        vals = np.ravel(a).tolist()
        linhas = [', '.join(map(str,vals[i:i+20])) for i in range(0,len(vals),20)]
        return f'static const {tipo} mnist_{nome}[{len(vals)}] = {{\n  '+',\n  '.join(linhas)+'\n};\n'
    s = '#ifndef MNIST_MODELO_INT8_H\n#define MNIST_MODELO_INT8_H\n#include <stdint.h>\n'
    s += '// Conv: OIHW, linear: [classe][entrada], flatten: CHW.\n'
    for i in (1,2,3):
        s += vetor(f'w{i}','int8_t',m[f'w{i}']) + vetor(f'b{i}','int32_t',m[f'b{i}'])
    s += f'#define MNIST_Q {Q}\n#define MNIST_M1 {m["m1"]}\n#define MNIST_M2 {m["m2"]}\n#endif\n'
    arq.write_text(s,encoding='utf-8')


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for nome in ('pesos','treino','teste','rotulos'):
        p.add_argument('--'+nome, required=True, type=Path)
    p.add_argument('--saida', type=Path, default=Path('.'))
    a=p.parse_args()
    a.saida.mkdir(parents=True,exist_ok=True)
    with np.load(a.pesos,allow_pickle=False) as orig:
        params=[(orig[k+'.weight'],orig[k+'.bias']) for k in CHAVES]
    treino=ler_idx(a.treino)
    ids=np.random.default_rng(42).choice(len(treino),1024,replace=False)
    m=quantizar(params,treino[ids])
    x,y=ler_idx(a.teste),ler_idx(a.rotulos)
    assert len(x)==len(y)>0
    f=[]; q=[]; logits=[]
    for i in range(0,len(x),64):
        xx=x[i:i+64]
        f.extend(flutuante(xx,params).argmax(1))
        z=inferir(xx,m); q.extend(z.argmax(1)); logits.append(z)
    f,q=np.asarray(f),np.asarray(q); z=np.concatenate(logits)
    r=dict(imagens_teste=len(x), imagens_calibracao=len(ids),
           acertos_float32=int((f==y).sum()), acertos_int8=int((q==y).sum()),
           acuracia_float32=float((f==y).mean()), acuracia_int8=float((q==y).mean()),
           concordancia=float((f==q).mean()),
           delta_pontos_percentuais=float(100*((q==y).mean()-(f==y).mean())),
           limites_acc_int32=[m[f'limite_acc{i}'] for i in (1,2,3)])
    np.savez(a.saida/'modelo_int8.npz',**m)
    np.savez_compressed(a.saida/'vetores_teste.npz',imagens=x[:100],rotulos=y[:100],logits_int32=z[:100],previsoes=q[:100])
    np.savetxt(a.saida/'matriz_confusao_int8.csv',np.bincount(10*y.astype(int)+q,minlength=100).reshape(10,10),fmt='%d',delimiter=',')
    (a.saida/'resultados_int8.json').write_text(json.dumps(r,indent=2)+'\n',encoding='utf-8')
    cabecalho(m,a.saida/'modelo_int8.h')
    print(json.dumps(r,indent=2))

if __name__=='__main__':
    main()
