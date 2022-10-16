import numpy as np

obin = 18
ksize = 8
eps = 1e-6
orientations = np.array([[1.0000, 0.9397, 0.7660, 0.500, 0.1736, -0.1736, -0.5000, -0.7660, -0.9397], [0.0000, 0.3420, 0.6428, 0.8660, 0.9848, 0.9848, 0.8660, 0.6428, 0.3420]])

def hog_easy(img):
    feat = img.astype(np.float32)

    # TODO1 calculation of the image gradient
    dx = feat[1:-1,2:,:] - feat[1:-1,:-2,:]
    dy = feat[2:,1:-1,:] - feat[:-2,1:-1,:]

    # calculation of the energy of the gradient
    v = np.sqrt(dx*dx + dy*dy + eps)

    # calculation of the max of the gradient with respect to the color channel
    maxv = np.max(v, axis = 2)
    maxcolor = np.argmax(v, axis = 2)
    edges = np.zeros((dx.shape[0], dx.shape[1], 2), dtype=np.float32)
    for y in range(edges.shape[0]):
        for x in range(edges.shape[1]):
            edges[y,x,0] = dx[y,x,maxcolor[y,x]]
            edges[y,x,1] = dy[y,x,maxcolor[y,x]]

    # TODO2 calculation of the orientation of the gradient
    iprod = np.dot(edges, orientations)
    iprod = np.concatenate([iprod,-iprod],axis=2)
    maxorientationind = np.argmax(iprod, axis=2)

    # calculation of the histogram of the gradient
    orientationcount=np.zeros((edges.shape[0]//ksize, edges.shape[1]//ksize, obin), dtype=np.float32)
    for y in range(ksize*orientationcount.shape[0]):
        for x in range(ksize*orientationcount.shape[1]):
            orientationcount[y//ksize, x//ksize, maxorientationind[y,x]] += maxv[y,x]

    # calculation of the contrast sensitive feature
    contrastsensitive = orientationcount

    # TODO3 calculation of the contrast insensitive feature
    contrastinsensitive = orientationcount[:,:,:9] + orientationcount[:,:,9:]

    # calculation of the gradient energy feature
    norm = np.sum(orientationcount ** 2, axis=2)
    norm_pad = np.pad(norm, [(1, 1)], 'constant')

    # TODO4 calculation of the sum of gradient energy
    norm_sum = (norm_pad[:-1,:-1] + norm_pad[:-1,1:] + norm_pad[1:,:-1] + norm_pad[1:,1:] + eps)/4
    gradientenergy = np.zeros((orientationcount.shape[0], orientationcount.shape[1], 4), dtype=np.float32)
    gradientenergy[:,:,0] = norm/norm_sum[:-1,:-1]
    gradientenergy[:,:,1] = norm/norm_sum[:-1,1:]
    gradientenergy[:,:,2] = norm/norm_sum[1:,:-1]
    gradientenergy[:,:,3] = norm/norm_sum[1:,1:]

    # concatenation of all the feature
    return np.concatenate([contrastsensitive,contrastinsensitive,gradientenergy], axis=2)

