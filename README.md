# vaapi-opencl-interop

## source code

compute-runtime https://github.com/intel/compute-runtime

intel-graphics-compiler https://github.com/intel/intel-graphics-compiler

libva https://github.com/intel/libva

gmmlib https://github.com/intel/gmmlib

media-driver https://github.com/intel/media-driver

## install tools
```bash
sudo apt install cmake automake autoconf libtool libdrm-dev xorg xorg-dev openbox libx11-dev libgl1-mesa-glx libgl1-mesa-dev
```

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

## Sync between VAAPI and OpenCL

clEnqueueAcquireVA_APIMediaSurfacesINTEL
```
iHD_drv_video.so!drmIoctl(int fd, unsigned long request, void * arg) (/home/fresh/data/work/intel_opencl_linux/source/media-driver/media_driver/linux/common/os/i915/xf86drm.c:180)
iHD_drv_video.so!mos_gem_bo_wait(mos_linux_bo * bo, int64_t timeout_ns) (/home/fresh/data/work/intel_opencl_linux/source/media-driver/media_driver/linux/common/os/i915/mos_bufmgr.c:2313)
iHD_drv_video.so!DdiMedia_SyncSurface(VADriverContextP ctx, VASurfaceID render_target) (/home/fresh/data/work/intel_opencl_linux/source/media-driver/media_driver/linux/common/ddi/media_libva.cpp:3367)
libva.so.2!vaSyncSurface(VADisplay dpy, VASurfaceID render_target) (/home/fresh/data/work/intel_opencl_linux/source/libva/va/va.c:1539)
libigdrcl.so!OCLRT::VASharingFunctions::syncSurface(OCLRT::VASharingFunctions * const this, VASurfaceID vaSurface) (/home/fresh/data/work/intel_opencl_linux/source/compute-runtime/runtime/sharings/va/va_sharing_functions.h:42)
libigdrcl.so!OCLRT::VASurface::synchronizeObject(OCLRT::VASurface * const this, OCLRT::UpdateData & updateData) (/home/fresh/data/work/intel_opencl_linux/source/compute-runtime/runtime/sharings/va/va_surface.cpp:90)
libigdrcl.so!OCLRT::SharingHandler::synchronizeHandler(OCLRT::SharingHandler * const this, OCLRT::UpdateData & updateData) (/home/fresh/data/work/intel_opencl_linux/source/compute-runtime/runtime/sharings/sharing.cpp:42)
libigdrcl.so!OCLRT::SharingHandler::acquire(OCLRT::SharingHandler * const this, OCLRT::MemObj * memObj) (/home/fresh/data/work/intel_opencl_linux/source/compute-runtime/runtime/sharings/sharing.cpp:24)
libigdrcl.so!OCLRT::CommandQueue::enqueueAcquireSharedObjects(OCLRT::CommandQueue * const this, cl_uint numObjects, const cl_mem * memObjects, cl_uint numEventsInWaitList, const cl_event * eventWaitList, cl_event * oclEvent, cl_uint cmdType) (/home/fresh/data/work/intel_opencl_linux/source/compute-runtime/runtime/command_queue/command_queue.cpp:219)
libigdrcl.so!clEnqueueAcquireVA_APIMediaSurfacesINTEL(cl_command_queue commandQueue, cl_uint numObjects, const cl_mem * memObjects, cl_uint numEventsInWaitList, const cl_event * eventWaitList, cl_event * event) (/home/fresh/data/work/intel_opencl_linux/source/compute-runtime/runtime/sharings/va/cl_va_api.cpp:94)
oclProcessDecodeOutput(VADisplay vaDpy, VASurfaceID * vaSurfID) (/home/fresh/data/work/intel_opencl_linux/source/vaapi-opencl-interop/interop/vaocl.c:279)
main(int argc, char ** argv) (/home/fresh/data/work/intel_opencl_linux/source/vaapi-opencl-interop/interop/vaocl.c:508)
```
