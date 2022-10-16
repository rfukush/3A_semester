from distutils.core import setup, Extension
from numpy.distutils.misc_util import get_numpy_include_dirs

setup(
    package_dir={":"},
    packages=[],
    ext_modules=[
        Extension('hog_desc',
                  sources=[
                      'hog_desc.cc'
                      ],
                  include_dirs=[] + get_numpy_include_dirs(),
                  library_dirs=[],
                  libraries=[],
                  extra_compile_args=[],
                  extra_link_args=[]
                  )
        ]
    )
