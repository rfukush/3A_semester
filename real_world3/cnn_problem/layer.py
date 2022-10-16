#import numpy as np
import cupy as np
import util
import sys


class Layer(object):
    def __init__(self, lr=0.01, momentum=0.9, weight_decay_rate=5e-4):
        self.params = {}
        self.grads = {}
        self.v = None
        self.momentum = momentum
        self.lr = lr
        self.weight_decay_rate = weight_decay_rate

    def update(self):
        if self.v == None:
            self.v = {}
            for k in self.params.keys():
                self.v[k] = np.zeros(shape = self.params[k].shape, dtype = self.params[k].dtype)
                
        for k in self.params.keys():
            self.v[k] = self.v[k] * self.momentum - self.lr * self.grads[k]
            self.params[k] = (1 - self.lr * self.weight_decay_rate) * self.params[k] + self.v[k]

    def zerograd(self):
        for k in self.params.keys():
            self.grads[k] = np.zeros(shape = self.params[k].shape, dtype = self.params[k].dtype)


class ReLULayer(Layer):
    def __init__(self):
        super(ReLULayer, self).__init__()

    def forward(self, x):
        out = np.maximum(x, 0)
        self.mask = np.sign(out) #TODO4 change right side
        return out

    def backward(self, dout):
        return self.mask * dout


class LinearLayer(Layer):
    def __init__(self, input_dim, output_dim):
        super(LinearLayer, self).__init__()
        self.params['W'] = np.random.normal(scale=np.sqrt(1.0/input_dim), size=(input_dim, output_dim)).astype(np.float32)
        self.params['b'] = np.zeros(shape = (1, output_dim), dtype=np.float32)

    def forward(self, x):
        self.x = x
        return np.dot(x, self.params['W']) + self.params['b']

    def backward(self, dout):
        #print(self.params['b'].shape)
        self.grads['W'] = np.dot(self.x.T ,dout) #TODO1 change right side
        self.grads['b'] = np.sum(dout, axis = 0, keepdims = True) #TODO2 change right side
        return np.dot(dout, self.params['W'].T) #TODO3 change return value

    
class FlattenLayer(Layer):
    def __init__(self):
        super(FlattenLayer, self).__init__()

    def forward(self, x):
        self.origshape = x.shape
        return np.reshape(x, (x.shape[0], x.size // x.shape[0]))

    def backward(self, dout):
        return np.reshape(dout, self.origshape)


class Convsize3Layer(Layer):
    def __init__(self, input_dim, output_dim):
        super(Convsize3Layer, self).__init__()
        self.params['W'] = np.random.normal(scale=np.sqrt(1.0/input_dim/9), size=(output_dim, input_dim, 3, 3)).astype(np.float32)
        self.params['b'] = np.zeros(shape = (1, output_dim, 1, 1), dtype=np.float32)

    def _im2col(self, data):
        bs, fsize, w, h = data.shape
        assert w%2 ==0 and h%2 == 0, 'input image size should be even'
        col = np.zeros((bs, w, h, fsize, 3, 3), dtype=data.dtype)
        data = np.pad(data, [(0, 0), (0, 0), (1, 1), (1, 1)], 'constant')

        """for fx in range(3):
                for fy in range(3):
                    for b in range(bs):
                        for f in range(fsize):
                            for w0 in range(w):
                                for h0 in range(h):
                                    col[b,w0,h0,f,fx,fy]=data[b,f,fx+w0,fy+h0] """
        #TODO7 comment out above 7 lines, uncomment below 3 lines, and change the right side of the third line
        for fx in range(3):
            for fy in range(3):
                col[:, :, :, :, fx, fy] = data.transpose(0, 2, 3, 1)[:, fx: w + fx, fy: h + fy, :] #TODO7 change the right side

        return col

    def _conv(self, data, filt):
        bs, fsize, w, h = data.shape
        mult = np.dot(np.reshape(self._im2col(data), (bs, w, h, fsize*3*3)), np.reshape(filt, (-1, fsize*3*3)).T)
        return np.transpose(mult, (0, 3, 1, 2))

    def forward(self, x):
        self.x = x
        return self._conv(x, self.params['W']) + self.params['b']

    def backward(self, dout):
        col = self._im2col(self.x)
        cs = col.shape
        self.grads['W'] = np.reshape(np.dot(np.reshape(np.transpose(dout,(1,0,2,3)),(dout.shape[1],-1)),np.reshape(col,(cs[0]*cs[1]*cs[2],cs[3]*cs[4]*cs[5]))), self.params['W'].shape)
        self.grads['b'] = np.sum(dout, axis=(0,2,3), keepdims=True)
        return self._conv(dout, np.transpose(self.params['W'][:,:,::-1,::-1],(1,0,2,3)))


class PoolingLayer(Layer):
    def __init__(self, ksize):
        super(PoolingLayer,self).__init__()
        self.ksize = ksize

    def _im2col(self, data):
        k = self.ksize
        bs, fsize, w, h = data.shape
        assert w%k ==0 and h%k == 0, 'input image size should be multiple of kernel size'
        ow, oh = w//k, h//k
        col = np.zeros((bs,fsize,ow,oh,k,k),dtype=data.dtype)
        """ for fx in range(k):
            for fy in range(k):
                for b in range(bs):
                    for f in range(fsize):
                        for w0 in range(ow):
                            for h0 in range(oh):
                                col[b,f,w0,h0,fx,fy] = data[b, f, fx + w0 * k, fy + h0 * k] #TODO8 change the right side  """
        #TODO9 comment out above 7 lines, uncomment below 3 lines, and change the right side of the third line
        for fx in range(k):
            for fy in range(k):
                col[:,:,:,:,fx,fy] = data[:, :, fx: fx + ow * k:k , fy: fy + oh * k: k] #TODO9 change the right side
        return col 

    def _col2im(self, col):
        k = self.ksize
        bs,fsize,ow,oh=col.shape[0],col.shape[1],col.shape[2],col.shape[3]
        data = np.zeros((bs,fsize,ow*k,oh*k),dtype=col.dtype)
        for fx in range(k):
            for fy in range(k):
                data[:,:,fx:fx+ow*k:k,fy:fy+oh*k:k] = col[:,:,:,:,fx,fy]
        return data


class AvgPoolingLayer(PoolingLayer):
    def __init__(self, ksize):
        super(AvgPoolingLayer, self).__init__(ksize)

    def forward(self, x):
        return np.mean(self._im2col(x), axis=(4,5))

    def backward(self, dout):
        return np.repeat(np.repeat(dout, self.ksize, axis=2), self.ksize, axis=3) / (self.ksize ** 2)


class MaxPoolingLayer(PoolingLayer):
    def __init__(self, ksize):
        super(MaxPoolingLayer, self).__init__(ksize)

    def forward(self, x):
        col = self._im2col(x)
        col = np.reshape(col, (col.shape[0], col.shape[1], col.shape[2], col.shape[3], -1))
        self.maxinds = np.argmax(col, axis=-1).flatten()
        return np.max(col, axis=-1)

    def backward(self, dout):
        bs, fsize, ow, oh = dout.shape
        mask = np.zeros((self.maxinds.size, self.ksize**2), dtype=dout.dtype)
        mask[np.arange(mask.shape[0]), self.maxinds] = 1
        mask = self._col2im(np.reshape(mask, (bs, fsize, ow, oh, self.ksize, self.ksize)))
        return mask * np.repeat(np.repeat(dout, self.ksize, axis=2), self.ksize, axis=3)
        
    
class Sequential:
    def __init__(self, layers = []):
        self.layers = layers

    def addlayer(self, layer):
        self.layers.append(layer)
        
    def forward(self, x):
        for l in self.layers:
            x = l.forward(x) #TODO5 change this line
        return x

    def backward(self, dout):
        for l in reversed(self.layers):
            dout = l.backward(dout) #TODO6 change this line
        return dout

    def update(self):
        for l in self.layers:
            l.update()

    def zerograd(self):
        for l in self.layers:
            l.zerograd()


class Classifier:
    def __init__(self, model):
        self.model = model

    def predict(self, x, y):
        h = self.model.forward(x)
        pred = np.argmax(h, axis=1)
        acc = 1.0 * np.where(pred == y)[0].size / h.shape[0]
        loss = util.softmax_cross_entropy(h, y)
        return loss, acc

    def update(self, x, y):
        self.model.zerograd()
        h = self.model.forward(x)
        pred = np.argmax(h, axis=1)
        acc = 1.0 * np.where(pred == y)[0].size / h.shape[0]
        prob = util.softmax(h)
        loss = util.cross_entropy(prob, y)
        
        dout = prob
        dout[np.arange(dout.shape[0]), y] -= 1

        self.model.backward(dout / dout.shape[0])
        self.model.update()

        return loss, acc


def check(arch):
    dr = 1e-5
    thres = 1e-4
    if arch == 'linear':
        print('linear model')
        print('check grad w.r.t input')
        ok = True
        for i in range(10):
            x = np.random.normal(size = (200, 100))
            dx = np.random.normal(size = (200, 100))
            W = np.random.normal(size = (100, 50))
            b = np.random.normal(size = (1, 50))
            y = np.random.normal(size = (200, 50))
            model = LinearLayer(100, 50)
            model.params['W'] = W
            model.params['b'] = b

            # calc backprop
            model.zerograd()
            model.forward(x)
            gradx = model.backward(y)
            bpgrad = np.sum(dx * gradx)
            numgrad = np.sum(y * (model.forward(x + dx*dr/2) - model.forward(x - dx*dr/2))) / dr
            if np.abs(bpgrad - numgrad) > thres:
                ok = False
            print('case %d (backprop, numerical): (%f, %f)'%(i+1,bpgrad,numgrad))
        print('ok' if ok else 'ng')

        print('check grad w.r.t bias')
        ok = True
        for i in range(10):
            x = np.random.normal(size = (200, 100))
            W = np.random.normal(size = (100, 50))
            b = np.random.normal(size = (1, 50))
            db = np.random.normal(size = (1, 50))
            y = np.random.normal(size = (200, 50))
            model = LinearLayer(100, 50)
            model.params['W'] = W
            model.params['b'] = b

            # calc backprop
            model.zerograd()
            model.forward(x)
            model.backward(y)
            if not(model.grads['b'].shape == (1,50)):
                ok = False
                print('the gradient of bias should be the same size to the bias')
            bpgrad = np.sum(db * model.grads['b'])
            pmodel = LinearLayer(100, 50)
            pmodel.params['W'] = W
            pmodel.params['b'] = b + db * dr/2
            nmodel = LinearLayer(100, 50)
            nmodel.params['W'] = W
            nmodel.params['b'] = b - db * dr/2
            numgrad = np.sum(y * (pmodel.forward(x) - nmodel.forward(x))) / dr
            if np.abs(bpgrad - numgrad) > thres:
                ok = False
            print('case %d (backprop, numerical): (%f, %f)'%(i+1,bpgrad,numgrad))
        print('ok' if ok else 'ng')

        print('check grad w.r.t weight')
        ok = True
        for i in range(10):
            x = np.random.normal(size = (200, 100))
            W = np.random.normal(size = (100, 50))
            dW = np.random.normal(size = (100, 50))
            b = np.random.normal(size = (1, 50))
            y = np.random.normal(size = (200, 50))
            model = LinearLayer(100, 50)
            model.params['W'] = W
            model.params['b'] = b

            # calc backprop
            model.zerograd()
            model.forward(x)
            model.backward(y)
            bpgrad = np.sum(dW * model.grads['W'])
            pmodel = LinearLayer(100, 50)
            pmodel.params['W'] = W + dW * dr/2
            pmodel.params['b'] = b
            nmodel = LinearLayer(100, 50)
            nmodel.params['W'] = W - dW * dr/2
            nmodel.params['b'] = b
            numgrad = np.sum(y * (pmodel.forward(x) - nmodel.forward(x))) / dr
            if np.abs(bpgrad - numgrad) > thres:
                ok = False
            print('case %d (backprop, numerical): (%f, %f)'%(i+1,bpgrad,numgrad))
        print('ok' if ok else 'ng')

    if arch == 'relu':
        print('relu model')
        print('check grad w.r.t input')
        ok = True
        for i in range(10):
            x = (0.5 + np.random.rand(200, 100)) * (np.random.randint(2, size = (200, 100)) * 2 - 1)
            dx = np.random.normal(size = (200, 100))
            y = np.random.normal(size = (200, 100))
            model = ReLULayer()

            # calc backprop
            model.zerograd()
            model.forward(x)
            gradx = model.backward(y)
            bpgrad = np.sum(dx * gradx)
            numgrad = np.sum(y * (model.forward(x + dx*dr/2) - model.forward(x - dx*dr/2))) / dr
            if np.abs(bpgrad - numgrad) > thres:
                ok = False
            print('case %d (backprop, numerical): (%f, %f)'%(i+1,bpgrad,numgrad))
        print('ok' if ok else 'ng')

    if arch == 'sequential':
        print('sequential model')
        print('check grad w.r.t input')
        ok = True
        for i in range(10):
            x = np.random.normal(size = (200, 100))
            dx = np.random.normal(size = (200, 100))
            y = np.random.normal(size = (200, 25))
            W1 = np.random.normal(size = (100, 50))
            b1 = np.random.normal(size = (1, 50))
            model1 = LinearLayer(100, 50)
            model1.params['W'] = W1
            model1.params['b'] = b1
            W2 = np.random.normal(size = (50, 25))
            b2 = np.random.normal(size = (1, 25))
            model2 = LinearLayer(50, 25)
            model2.params['W'] = W2
            model2.params['b'] = b2
            model = Sequential([model1,ReLULayer(),model2])

            # calc backprop
            model.zerograd()
            model.forward(x)
            gradx = model.backward(y)
            bpgrad = np.sum(dx * gradx)
            numgrad = np.sum(y * (model.forward(x + dx*dr/2) - model.forward(x - dx*dr/2))) / dr
            if np.abs(bpgrad - numgrad) > thres:
                ok = False
            print('case %d (backprop, numerical): (%f, %f)'%(i+1,bpgrad,numgrad))
        print('ok' if ok else 'ng')

    if arch == 'conv':
        print('conv model')
        print('check output value')
        for i in range(2):
            x = np.random.normal(size = (20, 3, 32, 32))
            W = np.random.normal(size = (64, 3, 3, 3))
            b = np.random.normal(size = (1, 64, 1, 1))
            model = Convsize3Layer(3, 64)
            model.params['W'] = W
            model.params['b'] = b
            posn = np.random.randint(20)
            posd = np.random.randint(64)
            out = model.forward(x)[posn,posd]
            posx = np.random.randint(1,31)
            posy = np.random.randint(1,31)
            print('case %d upper left (forward, numerical): (%f, %f)'%(i+1,x[posn,:,:2,:2].flatten().dot(W[posd,:,1:,1:].flatten())+b[0,posd,0,0],out[0,0]))
            print('case %d upper right (forward, numerical): (%f, %f)'%(i+1,x[posn,:,:2,-2:].flatten().dot(W[posd,:,1:,:2].flatten())+b[0,posd,0,0],out[0,-1]))
            print('case %d lower left (forward, numerical): (%f, %f)'%(i+1,x[posn,:,-2:,:2].flatten().dot(W[posd,:,:2,1:].flatten())+b[0,posd,0,0],out[-1,0]))
            print('case %d lower right (forward, numerical): (%f, %f)'%(i+1,x[posn,:,-2:,-2:].flatten().dot(W[posd,:,:2,:2].flatten())+b[0,posd,0,0],out[-1,-1]))
            print('case %d random (forward, numerical): (%f, %f)'%(i+1,x[posn,:,posx-1:posx+2,posy-1:posy+2].flatten().dot(W[posd].flatten())+b[0,posd,0,0],out[posx,posy]))

        print('check grad w.r.t input')
        ok = True
        for i in range(10):
            x = np.random.normal(size = (20, 3, 32, 32))
            dx = np.random.normal(size = (20, 3, 32, 32))
            y = np.random.normal(size = (20, 64, 32, 32))
            W = np.random.normal(size = (64, 3, 3, 3))
            b = np.random.normal(size = (1, 64, 1, 1))
            model = Convsize3Layer(3, 64)
            model.params['W'] = W
            model.params['b'] = b

            # calc backprop
            model.zerograd()
            model.forward(x)
            gradx = model.backward(y)
            bpgrad = np.sum(dx * gradx)
            numgrad = np.sum(y * (model.forward(x + dx*dr/2) - model.forward(x - dx*dr/2))) / dr
            if np.abs(bpgrad - numgrad) > thres:
                ok = False
            print('case %d (backprop, numerical): (%f, %f)'%(i+1,bpgrad,numgrad))
        print('ok' if ok else 'ng')
        
        print('check grad w.r.t bias')
        ok = True
        for i in range(10):
            x = np.random.normal(size = (20, 3, 32, 32))
            y = np.random.normal(size = (20, 64, 32, 32))
            W = np.random.normal(size = (64, 3, 3, 3))
            b = np.random.normal(size = (1, 64, 1, 1))
            db = np.random.normal(size = (1, 64, 1, 1))
            model = Convsize3Layer(3, 64)
            model.params['W'] = W
            model.params['b'] = b

            # calc backprop
            model.zerograd()
            model.forward(x)
            model.backward(y)
            bpgrad = np.sum(db * model.grads['b'])
            pmodel = Convsize3Layer(3, 64)
            pmodel.params['W'] = W
            pmodel.params['b'] = b + db * dr/2
            nmodel = Convsize3Layer(3, 64)
            nmodel.params['W'] = W
            nmodel.params['b'] = b - db * dr/2

            numgrad = np.sum(y * (pmodel.forward(x) - nmodel.forward(x))) / dr
            if np.abs(bpgrad - numgrad) > thres:
                ok = False
            print('case %d (backprop, numerical): (%f, %f)'%(i+1,bpgrad,numgrad))
        print('ok' if ok else 'ng')        

        print('check grad w.r.t weight')
        ok = True
        for i in range(10):
            x = np.random.normal(size = (20, 3, 32, 32))
            y = np.random.normal(size = (20, 64, 32, 32))
            W = np.random.normal(size = (64, 3, 3, 3))
            dW = np.random.normal(size = (64, 3, 3, 3))
            b = np.random.normal(size = (1, 64, 1, 1))
            model = Convsize3Layer(3, 64)
            model.params['W'] = W
            model.params['b'] = b

            # calc backprop
            model.zerograd()
            model.forward(x)
            model.backward(y)
            bpgrad = np.sum(dW * model.grads['W'])
            pmodel = Convsize3Layer(3, 64)
            pmodel.params['W'] = W + dW * dr/2
            pmodel.params['b'] = b
            nmodel = Convsize3Layer(3, 64)
            nmodel.params['W'] = W - dW * dr/2
            nmodel.params['b'] = b

            numgrad = np.sum(y * (pmodel.forward(x) - nmodel.forward(x))) / dr
            if np.abs(bpgrad - numgrad) > thres:
                ok = False
            print('case %d (backprop, numerical): (%f, %f)'%(i+1,bpgrad,numgrad))
        print('ok' if ok else 'ng')

    if arch == 'pooling':
        print('avgpooling model')
        print('check output value')
        for i in range(10):
            x = np.random.normal(size = (20, 3, 32, 32))
            model = AvgPoolingLayer(ksize=2)
            posn = np.random.randint(20)
            posd = np.random.randint(3)
            posx = np.random.randint(16)
            posy = np.random.randint(16)
            out = model.forward(x)
            print('case %d (forward, numerical): (%f, %f)'%(i+1,np.mean(x[posn,posd,posx*2:(posx+1)*2,posy*2:(posy+1)*2].flatten()),out[posn,posd,posx,posy]))
            
        print('check grad w.r.t input')
        ok = True
        for i in range(10):
            x = np.random.normal(size = (20, 3, 32, 32))
            dx = np.random.normal(size = (20, 3, 32, 32))
            y = np.random.normal(size = (20, 3, 16, 16))
            model = AvgPoolingLayer(ksize=2)

            # calc backprop
            model.zerograd()
            model.forward(x)
            gradx = model.backward(y)
            bpgrad = np.sum(dx * gradx)
            numgrad = np.sum(y * (model.forward(x + dx*dr/2) - model.forward(x - dx*dr/2))) / dr
            if np.abs(bpgrad - numgrad) > thres:
                ok = False
            print('case %d (backprop, numerical): (%f, %f)'%(i+1,bpgrad,numgrad))
        print('ok' if ok else 'ng')

        print('maxpooling model')
        print('check output value')
        for i in range(10):
            x = np.random.normal(size = (20, 3, 32, 32))
            model = MaxPoolingLayer(ksize=2)
            posn = np.random.randint(20)
            posd = np.random.randint(3)
            posx = np.random.randint(16)
            posy = np.random.randint(16)
            out = model.forward(x)
            print('case %d (forward, numerical): (%f, %f)'%(i+1,np.max(x[posn,posd,posx*2:(posx+1)*2,posy*2:(posy+1)*2].flatten()),out[posn,posd,posx,posy]))
            
        print('check grad w.r.t input')
        ok = True
        for i in range(10):
            x = np.random.normal(size = (20, 3, 32, 32))
            dx = np.random.normal(size = (20, 3, 32, 32))
            y = np.random.normal(size = (20, 3, 16, 16))
            model = MaxPoolingLayer(ksize=2)

            # calc backprop
            model.zerograd()
            model.forward(x)
            gradx = model.backward(y)
            bpgrad = np.sum(dx * gradx)
            numgrad = np.sum(y * (model.forward(x + dx*dr/2) - model.forward(x - dx*dr/2))) / dr
            if np.abs(bpgrad - numgrad) > thres:
                ok = False
            print('case %d (backprop, numerical): (%f, %f)'%(i+1,bpgrad,numgrad))
        print('ok' if ok else 'ng')

if __name__ == '__main__':
    arch = sys.argv[1]
    if arch == 'linear' or arch == 'relu' or arch == 'sequential' or arch == 'conv' or arch == 'pooling':
        check(arch)
    else:
        print('arch should be linear, relu, sequential, conv, pooling')
