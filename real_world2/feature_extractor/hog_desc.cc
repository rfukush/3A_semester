#include <math.h>
#include <Python.h>
#include <numpy/arrayobject.h>
#include <numpy/arrayscalars.h>

inline double round( double d )
{
    return floor( d + 0.5 );
}

// small value, used to avoid division by zero
#define eps 0.0001

// unit vectors used to compute gradient orientation
double uu[9] = {1.0000, 
		0.9397, 
		0.7660, 
		0.500, 
		0.1736, 
		-0.1736, 
		-0.5000, 
		-0.7660, 
		-0.9397};
double vv[9] = {0.0000, 
		0.3420, 
		0.6428, 
		0.8660, 
		0.9848, 
		0.9848, 
		0.8660, 
		0.6428, 
		0.3420};

static inline double min(double x, double y) { return (x <= y ? x : y); }
static inline double max(double x, double y) { return (x <= y ? y : x); }

static inline int min(int x, int y) { return (x <= y ? x : y); }
static inline int max(int x, int y) { return (x <= y ? y : x); }

// main function:
// takes a double color image and a bin size 
// returns HOG feature
//mxArray *process(const mxArray *mximage) {

static PyObject* process(PyObject* self, PyObject* args) {
  PyObject *image;
  int err = PyArg_ParseTuple(args,"O!",&PyArray_Type,&image);
  if(!err) return NULL;
  int type = PyArray_TYPE(image);
  if(!(type == NPY_DOUBLE))
    {
      PyErr_SetString(PyExc_TypeError, "array type should be double");
      return NULL;
    }
  npy_intp ndim = PyArray_NDIM(image);
  npy_intp *dims = PyArray_DIMS(image);
  if(!(ndim == 3))
    {
      PyErr_SetString(PyExc_TypeError, "array dimension should be 3");
      return NULL;
    }
  if(!(dims[2] == 3))
    {
      PyErr_SetString(PyExc_TypeError, "third array dimension should be 3");
      return NULL;
    }

  int sbin = 8;

  // memory for caching orientation histograms & their norms
  int blocks[2];
  blocks[0] = (int)round((double)dims[0]/(double)sbin);
  blocks[1] = (int)round((double)dims[1]/(double)sbin);
  double *hist = (double *)calloc(blocks[0]*blocks[1]*18, sizeof(double));
  double *norm = (double *)calloc(blocks[0]*blocks[1], sizeof(double));

  // memory for HOG features
  npy_intp out[3];
  out[0] = max(blocks[0]-2, 0);
  out[1] = max(blocks[1]-2, 0);
  out[2] = 27+4;
  PyObject *npfeat = PyArray_SimpleNew(3, out, type);

  
  int visible[2];
  visible[0] = blocks[0]*sbin;
  visible[1] = blocks[1]*sbin;
  
  for (int x = 1; x < visible[1]-1; x++) {
    for (int y = 1; y < visible[0]-1; y++) {
      // first color channel
      double dy = *((double *)PyArray_GETPTR3(image, min(y, dims[0]-2) + 1, min(x, dims[1]-2), 0)) - *((double *)PyArray_GETPTR3(image, min(y, dims[0]-2) - 1, min(x, dims[1]-2), 0));
      double dx = *((double *)PyArray_GETPTR3(image, min(y, dims[0]-2), min(x, dims[1]-2) + 1, 0)) - *((double *)PyArray_GETPTR3(image, min(y, dims[0]-2), min(x, dims[1]-2) - 1, 0));
      double v = dx*dx + dy*dy;

      // second color channel
      double dy2 = *((double *)PyArray_GETPTR3(image, min(y, dims[0]-2) + 1, min(x, dims[1]-2), 1)) - *((double *)PyArray_GETPTR3(image, min(y, dims[0]-2) - 1, min(x, dims[1]-2), 1));
      double dx2 = *((double *)PyArray_GETPTR3(image, min(y, dims[0]-2), min(x, dims[1]-2) + 1, 1)) - *((double *)PyArray_GETPTR3(image, min(y, dims[0]-2), min(x, dims[1]-2) - 1, 1));
      double v2 = dx2*dx2 + dy2*dy2;

      // third color channel
      double dy3 = *((double *)PyArray_GETPTR3(image, min(y, dims[0]-2) + 1, min(x, dims[1]-2), 2)) - *((double *)PyArray_GETPTR3(image, min(y, dims[0]-2) - 1, min(x, dims[1]-2), 2));
      double dx3 = *((double *)PyArray_GETPTR3(image, min(y, dims[0]-2), min(x, dims[1]-2) + 1, 2)) - *((double *)PyArray_GETPTR3(image, min(y, dims[0]-2), min(x, dims[1]-2) - 1, 2));
      double v3 = dx3*dx3 + dy3*dy3;

      // pick channel with strongest gradient
      if (v2 > v) {
	v = v2;
	dx = dx2;
	dy = dy2;
      } 
      if (v3 > v) {
	v = v3;
	dx = dx3;
	dy = dy3;
      }

      // snap to one of 18 orientations
      double best_dot = 0;
      int best_o = 0;
      for (int o = 0; o < 9; o++) {
	double dot = uu[o]*dx + vv[o]*dy;
	if (dot > best_dot) {
	  best_dot = dot;
	  best_o = o;
	} else if (-dot > best_dot) {
	  best_dot = -dot;
	  best_o = o+9;
	}
      }
      
      // add to 4 histograms around pixel using linear interpolation
      double xp = ((double)x+0.5)/(double)sbin - 0.5;
      double yp = ((double)y+0.5)/(double)sbin - 0.5;
      int ixp = (int)floor(xp);
      int iyp = (int)floor(yp);
      double vx0 = xp-ixp;
      double vy0 = yp-iyp;
      double vx1 = 1.0-vx0;
      double vy1 = 1.0-vy0;
      v = sqrt(v);

      if (ixp >= 0 && iyp >= 0) {
	*(hist + ixp*blocks[0] + iyp + best_o*blocks[0]*blocks[1]) += 
	  vx1*vy1*v;
      }

      if (ixp+1 < blocks[1] && iyp >= 0) {
	*(hist + (ixp+1)*blocks[0] + iyp + best_o*blocks[0]*blocks[1]) += 
	  vx0*vy1*v;
      }

      if (ixp >= 0 && iyp+1 < blocks[0]) {
	*(hist + ixp*blocks[0] + (iyp+1) + best_o*blocks[0]*blocks[1]) += 
	  vx1*vy0*v;
      }

      if (ixp+1 < blocks[1] && iyp+1 < blocks[0]) {
	*(hist + (ixp+1)*blocks[0] + (iyp+1) + best_o*blocks[0]*blocks[1]) += 
	  vx0*vy0*v;
      }
    }
  }

  // compute energy in each block by summing over orientations
  for (int o = 0; o < 9; o++) {
    double *src1 = hist + o*blocks[0]*blocks[1];
    double *src2 = hist + (o+9)*blocks[0]*blocks[1];
    double *dst = norm;
    double *end = norm + blocks[1]*blocks[0];
    while (dst < end) {
      *(dst++) += (*src1 + *src2) * (*src1 + *src2);
      src1++;
      src2++;
    }
  }

  // compute features
  for (int x = 0; x < out[1]; x++) {
    for (int y = 0; y < out[0]; y++) {
      double *src, *p, n1, n2, n3, n4;

      p = norm + (x+1)*blocks[0] + y+1;
      n1 = 1.0 / sqrt(*p + *(p+1) + *(p+blocks[0]) + *(p+blocks[0]+1) + eps);
      p = norm + (x+1)*blocks[0] + y;
      n2 = 1.0 / sqrt(*p + *(p+1) + *(p+blocks[0]) + *(p+blocks[0]+1) + eps);
      p = norm + x*blocks[0] + y+1;
      n3 = 1.0 / sqrt(*p + *(p+1) + *(p+blocks[0]) + *(p+blocks[0]+1) + eps);
      p = norm + x*blocks[0] + y;      
      n4 = 1.0 / sqrt(*p + *(p+1) + *(p+blocks[0]) + *(p+blocks[0]+1) + eps);

      double t1 = 0;
      double t2 = 0;
      double t3 = 0;
      double t4 = 0;

      // contrast-sensitive features
      src = hist + (x+1)*blocks[0] + (y+1);
      for (int o = 0; o < 18; o++) {
	double h1 = min(*src * n1, 0.2);
	double h2 = min(*src * n2, 0.2);
	double h3 = min(*src * n3, 0.2);
	double h4 = min(*src * n4, 0.2);
	*((double *)PyArray_GETPTR3(npfeat, y, x, o)) = 0.5 * (h1 + h2 + h3 + h4);
	t1 += h1;
	t2 += h2;
	t3 += h3;
	t4 += h4;
	src += blocks[0]*blocks[1];
      }

      // contrast-insensitive features
      src = hist + (x+1)*blocks[0] + (y+1);
      for (int o = 0; o < 9; o++) {
        double sum = *src + *(src + 9*blocks[0]*blocks[1]);
        double h1 = min(sum * n1, 0.2);
        double h2 = min(sum * n2, 0.2);
        double h3 = min(sum * n3, 0.2);
        double h4 = min(sum * n4, 0.2);
	*((double *)PyArray_GETPTR3(npfeat, y, x, o + 18)) = 0.5 * (h1 + h2 + h3 + h4);
        src += blocks[0]*blocks[1];
      }

      // texture features
      *((double *)PyArray_GETPTR3(npfeat, y, x, 27)) = 0.2357 * t1;
      *((double *)PyArray_GETPTR3(npfeat, y, x, 28)) = 0.2357 * t2;
      *((double *)PyArray_GETPTR3(npfeat, y, x, 29)) = 0.2357 * t3;
      *((double *)PyArray_GETPTR3(npfeat, y, x, 30)) = 0.2357 * t4;
    }
  }

  free(hist);
  free(norm);
  return npfeat;
}

static PyMethodDef methods[] = {
  {
    "hog_desc", process, METH_VARARGS, ""},{NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC inithog_desc(void)
{
  (void)Py_InitModule("hog_desc", methods);
  import_array();
}
