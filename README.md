# vaapi-opencl-interop

## source code

compute-runtime https://github.com/intel/compute-runtime

intel-graphics-compiler https://github.com/intel/intel-graphics-compiler

libva https://github.com/intel/libva

gmmlib https://github.com/intel/gmmlib

media-driver https://github.com/intel/media-driver

## dependencies
download intel-graphics-compiler offical release binaries

https://github.com/intel/intel-graphics-compiler/releases
```bash
sudo dpkg -i ./intel-igc-core_1.0-0_amd64.deb
sudo dpkg -i ./intel-igc-media_1.0-0_amd64.deb
sudo dpkg -i ./intel-igc-opencl-devel_1.0-0_amd64.deb
sudo dpkg -i ./intel-igc-opencl_1.0-0_amd64.deb
sudo ldconfig
```

## build
```bash
# build compute-runtime
mkdir build & cd build & mkdir neo & cd neo
cmake ../../source/compute-runtime
make -j8
sudo make install
sudo ln -s /usr/lib/x86_64-linux-gnu/libOpenCL.so.1 /usr/lib/libOpenCL.so

# build interop sample app
cd build & mkdir interop & cd interop
cmake ../../source/vaapi-opencl-interop/interop
make
sudo make install
```

## run test
```bash
export LIBVA_DRIVER_NAME=iHD
cd build/interop
./vaocl
```
output
```
libva info: VA-API version 1.5.0
libva info: va_getDriverName() returns 0
libva info: User requested driver 'iHD'
libva info: Trying to open /usr/local/lib/dri/iHD_drv_video.so
libva info: Found init function __vaDriverInit_1_5
libva info: va_openDriver() returns 0
INFO: platform nubmer = 1
INFO: Intel(R) OpenCL HD Graphics
INFO: OpenCL 2.1 
execute sucessfully.
```
