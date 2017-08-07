# Intercept the RTLD to make library substitutions

## To build on Titan/Chester
```
module switch PrgEnv-pgi PrgEnv-gnu
module load boost
module load cmake3

mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/foo/bar -DCMAKE_BUILD_TYPE=RELEASE cmake ..
make
make install
```

## To use
If we want to intercept the dynamic loading of any library entry containing `libfoo.so` with `/bar/libbar.so` we can do the following
```
export DL_INTERCEPT=libfoo.so:/bar/libbar.so
LD_AUDIT='/install/prefix/libdl-intercept.so' a.out
```
multiple libraries can be intercepted by seperating the replacement pairs with a comma:
```
export DL_INTERCEPT=libfoo.so:/bar/libbar.so, libbaz.so:/raz/libraz.so
```
