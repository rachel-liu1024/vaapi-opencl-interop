
clCreateImage2D

```
libigdrcl.so!NEO::Drm::ioctl(NEO::Drm * const this, unsigned long request, void * arg) (/home/fresh/data/work/intel_opencl_compute/source/compute-runtime-19.18.12932/runtime/os_interface/linux/drm_neo.cpp:30)
libigdrcl.so!NEO::DrmMemoryManager::allocateGraphicsMemoryForImageImpl(NEO::DrmMemoryManager * const this, const NEO::MemoryManager::AllocationData & allocationData, std::unique_ptr<NEO::Gmm, std::default_delete<NEO::Gmm> > gmm) (/home/fresh/data/work/intel_opencl_compute/source/compute-runtime-19.18.12932/runtime/os_interface/linux/drm_memory_manager.cpp:339)
libigdrcl.so!NEO::MemoryManager::allocateGraphicsMemoryForImage(NEO::MemoryManager * const this, const NEO::MemoryManager::AllocationData & allocationData) (/home/fresh/data/work/intel_opencl_compute/source/compute-runtime-19.18.12932/runtime/memory_manager/memory_manager.cpp:369)
libigdrcl.so!NEO::MemoryManager::allocateGraphicsMemory(NEO::MemoryManager * const this, const NEO::MemoryManager::AllocationData & allocationData) (/home/fresh/data/work/intel_opencl_compute/source/compute-runtime-19.18.12932/runtime/memory_manager/memory_manager.cpp:332)
libigdrcl.so!NEO::MemoryManager::allocateGraphicsMemoryInPreferredPool(NEO::MemoryManager * const this, const NEO::AllocationProperties & properties, const void * hostPtr) (/home/fresh/data/work/intel_opencl_compute/source/compute-runtime-19.18.12932/runtime/memory_manager/memory_manager.cpp:323)
libigdrcl.so!NEO::MemoryManager::allocateGraphicsMemoryWithProperties(NEO::MemoryManager * const this, const NEO::AllocationProperties & properties) (/home/fresh/data/work/intel_opencl_compute/source/compute-runtime-19.18.12932/runtime/memory_manager/memory_manager.h:72)
libigdrcl.so!NEO::Image::create(NEO::Context * context, cl_mem_flags flags, const NEO::SurfaceFormatInfo * surfaceFormat, const cl_image_desc * imageDesc, const void * hostPtr, cl_int & errcodeRet) (/home/fresh/data/work/intel_opencl_compute/source/compute-runtime-19.18.12932/runtime/mem_obj/image.cpp:247)
libigdrcl.so!NEO::Image::validateAndCreateImage(NEO::Context * context, cl_mem_flags flags, const cl_image_format * imageFormat, const cl_image_desc * imageDesc, const void * hostPtr, cl_int & errcodeRet) (/home/fresh/data/work/intel_opencl_compute/source/compute-runtime-19.18.12932/runtime/mem_obj/image.cpp:1026)
libigdrcl.so!clCreateImage2D(cl_context context, cl_mem_flags flags, const cl_image_format * imageFormat, size_t imageWidth, size_t imageHeight, size_t imageRowPitch, void * hostPtr, cl_int * errcodeRet) (/home/fresh/data/work/intel_opencl_compute/source/compute-runtime-19.18.12932/runtime/api/api.cpp:764)
libOpenCL.so.1!clCreateImage2D(cl_context context, cl_mem_flags flags, const cl_image_format * image_format, size_t image_width, size_t image_height, size_t image_row_pitch, void * host_ptr, cl_int * errcode_ret) (/home/fresh/data/work/intel_opencl_compute/source/OpenCL-ICD-Loader/loader/icd_dispatch.c:1499)
oclProcessDecodeOutput(VADisplay vaDpy, VAContextID context_id, VASurfaceID * vaSurfID) (/home/fresh/data/work/intel_opencl_compute/source/vaapi-opencl-interop/interop/vaocl.c:289)
main(int argc, char ** argv) (/home/fresh/data/work/intel_opencl_compute/source/vaapi-opencl-interop/interop/vaocl.c:542)
```
