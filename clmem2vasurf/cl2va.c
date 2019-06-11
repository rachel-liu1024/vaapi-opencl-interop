#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <va/va.h>
#include <va/va_drmcommon.h>
#include "va_display.h"

#include <CL/cl.h>
#include <CL/cl_va_api_media_sharing_intel.h>
#include <CL/cl_ext_intel.h>

#define CL_MEM_ALLOCATION_HANDLE_INTEL 0x10050

#define CL_STR_LEN 1024

#define USE_CL_NV12 1

#define CHECK_VASTATUS(va_status, func)                                        \
    if (va_status != VA_STATUS_SUCCESS)                                        \
    {                                                                          \
        fprintf(stderr, "%s:%s (%d) failed,exit\n", __func__, func, __LINE__); \
        exit(1);                                                               \
    }

#define CHECK_OCL_ERROR(err, msg)     \
    if ((err) < 0)                    \
    {                                 \
        printf("ERROR: %s\n", (msg)); \
        return -1;                    \
    }

#define CHECK_NULL_AND_RETURN(ptr) \
    if ((ptr) == NULL)             \
        return -1;

struct clMemInfo
{
    cl_mem_object_type type;
    cl_mem_flags flags;
    size_t sz;
    void *p;
    cl_uint count;
    cl_uint ref;
    cl_context ctx;
    int handle;
};

struct clImageInfo
{
    cl_image_format fmt;
    size_t esz;
    size_t pitch;
    size_t slicep;
    size_t width;
    size_t height;
    size_t depth;
};

clGetDeviceIDsFromVA_APIMediaAdapterINTEL_fn clGetDeviceIDsFromVA = NULL;
clCreateFromVA_APIMediaSurfaceINTEL_fn clCreateFromVA = NULL;
clEnqueueAcquireVA_APIMediaSurfacesINTEL_fn clEnqueueAcquireVA = NULL;
clEnqueueReleaseVA_APIMediaSurfacesINTEL_fn clEnqueueReleaseVA = NULL;

int getVASharingFunc(cl_platform_id platform)
{
    size_t len = 0;
    clGetPlatformInfo(platform, CL_PLATFORM_EXTENSIONS, 0, NULL, &len);

    char *str = (char *)malloc(len);
    CHECK_NULL_AND_RETURN(str);
    clGetPlatformInfo(platform, CL_PLATFORM_EXTENSIONS, len, str, NULL);
    //printf("%s\n", str);

    clGetDeviceIDsFromVA = (clGetDeviceIDsFromVA_APIMediaAdapterINTEL_fn)clGetExtensionFunctionAddressForPlatform(
        platform, "clGetDeviceIDsFromVA_APIMediaAdapterINTEL");
    CHECK_NULL_AND_RETURN(clGetDeviceIDsFromVA);

    clCreateFromVA = (clCreateFromVA_APIMediaSurfaceINTEL_fn)clGetExtensionFunctionAddressForPlatform(
        platform, "clCreateFromVA_APIMediaSurfaceINTEL");
    CHECK_NULL_AND_RETURN(clGetDeviceIDsFromVA);

    clEnqueueAcquireVA = (clEnqueueAcquireVA_APIMediaSurfacesINTEL_fn)clGetExtensionFunctionAddressForPlatform(
        platform, "clEnqueueAcquireVA_APIMediaSurfacesINTEL");
    CHECK_NULL_AND_RETURN(clGetDeviceIDsFromVA);

    clEnqueueReleaseVA = (clEnqueueReleaseVA_APIMediaSurfacesINTEL_fn)clGetExtensionFunctionAddressForPlatform(
        platform, "clEnqueueReleaseVA_APIMediaSurfacesINTEL");
    CHECK_NULL_AND_RETURN(clGetDeviceIDsFromVA);

    free(str);

    return 0;
}

int getMemInfo(cl_mem m, struct clMemInfo* info)
{
    cl_int err;

    cl_mem_object_type type;
    err = clGetMemObjectInfo(m, CL_MEM_TYPE, sizeof(type), &type, NULL);
    CHECK_OCL_ERROR(err, "clGetMemObjectInfo - CL_MEM_TYPE failed");
    printf("MemINFO: CL_MEM_TYPE = 0x%x\n", type);

    cl_mem_flags flags;
    err = clGetMemObjectInfo(m, CL_MEM_FLAGS, sizeof(flags), &flags, NULL);
    CHECK_OCL_ERROR(err, "clGetMemObjectInfo - CL_MEM_FLAGS failed");
    printf("MemINFO: CL_MEM_FLAGS = 0x%lx\n", flags);

    size_t sz;
    err = clGetMemObjectInfo(m, CL_MEM_SIZE, sizeof(sz), &sz, NULL);
    CHECK_OCL_ERROR(err, "clGetMemObjectInfo - CL_MEM_SIZE failed");
    printf("MemINFO: CL_MEM_SIZE = %d\n", sz);

    void *p;
    err = clGetMemObjectInfo(m, CL_MEM_HOST_PTR, sizeof(p), &p, NULL);
    CHECK_OCL_ERROR(err, "clGetMemObjectInfo - CL_MEM_HOST_PTR failed");
    printf("MemINFO: CL_MEM_HOST_PTR = 0x%x\n", p);

    cl_uint count;
    err = clGetMemObjectInfo(m, CL_MEM_MAP_COUNT, sizeof(count), &count, NULL);
    CHECK_OCL_ERROR(err, "clGetMemObjectInfo - CL_MEM_MAP_COUNT failed");
    printf("MemINFO: CL_MEM_MAP_COUNT = %d\n", count);

    cl_uint ref;
    err = clGetMemObjectInfo(m, CL_MEM_REFERENCE_COUNT, sizeof(ref), &ref, NULL);
    CHECK_OCL_ERROR(err, "clGetMemObjectInfo - CL_MEM_REFERENCE_COUNT failed");
    printf("MemINFO: CL_MEM_REFERENCE_COUNT = %d\n", ref);

    cl_context ctx;
    err = clGetMemObjectInfo(m, CL_MEM_CONTEXT, sizeof(ctx), &ctx, NULL);
    CHECK_OCL_ERROR(err, "clGetMemObjectInfo - CL_MEM_CONTEXT failed");
    printf("MemINFO: CL_MEM_CONTEXT = 0x%x\n", ctx);

    int handle;
    err = clGetMemObjectInfo(m, CL_MEM_ALLOCATION_HANDLE_INTEL, sizeof(handle), &handle, NULL);
    CHECK_OCL_ERROR(err, "clGetMemObjectInfo - CL_MEM_ALLOCATION_HANDLE_INTEL failed");
    printf("MemINFO: CL_MEM_ALLOCATION_HANDLE_INTEL = 0x%lx\n", handle);

    info->type = type;
    info->flags = flags;
    info->sz = sz;
    info->p = p;
    info->count = count;
    info->ref = ref;
    info->ctx = ctx;
    info->handle = handle;

    return 0;
}

int getImageInfo(cl_mem m, struct clImageInfo* info)
{
    cl_int err;

    cl_image_format fmt;
    err = clGetImageInfo(m, CL_IMAGE_FORMAT, sizeof(fmt), &fmt, NULL);
    CHECK_OCL_ERROR(err, "clGetImageInfo - CL_IMAGE_FORMAT failed");
    printf("ImageINFO: CL_IMAGE_FORMAT = 0x%x\n", fmt);

    size_t esz;
    err = clGetImageInfo(m, CL_IMAGE_ELEMENT_SIZE, sizeof(esz), &esz, NULL);
    CHECK_OCL_ERROR(err, "clGetImageInfo - CL_IMAGE_ELEMENT_SIZE failed");
    printf("ImageINFO: CL_IMAGE_ELEMENT_SIZE = %d\n", esz);

    size_t pitch;
    err = clGetImageInfo(m, CL_IMAGE_ROW_PITCH, sizeof(pitch), &pitch, NULL);
    CHECK_OCL_ERROR(err, "clGetImageInfo - CL_IMAGE_ROW_PITCH failed");
    printf("ImageINFO: CL_IMAGE_ROW_PITCH = %d\n", pitch);

    size_t slicep;
    err = clGetImageInfo(m, CL_IMAGE_SLICE_PITCH, sizeof(slicep), &slicep, NULL);
    CHECK_OCL_ERROR(err, "clGetImageInfo - CL_IMAGE_SLICE_PITCH failed");
    printf("ImageINFO: CL_IMAGE_SLICE_PITCH = %d\n", slicep);

    size_t width;
    err = clGetImageInfo(m, CL_IMAGE_WIDTH, sizeof(width), &width, NULL);
    CHECK_OCL_ERROR(err, "clGetImageInfo - CL_IMAGE_WIDTH failed");
    printf("ImageINFO: CL_IMAGE_WIDTH = %d\n", width);

    size_t height;
    err = clGetImageInfo(m, CL_IMAGE_HEIGHT, sizeof(height), &height, NULL);
    CHECK_OCL_ERROR(err, "clGetImageInfo - CL_IMAGE_HEIGHT failed");
    printf("ImageINFO: CL_IMAGE_HEIGHT = %d\n", height);

    size_t depth;
    err = clGetImageInfo(m, CL_IMAGE_DEPTH, sizeof(depth), &depth, NULL);
    CHECK_OCL_ERROR(err, "clGetImageInfo - CL_IMAGE_DEPTH failed");
    printf("ImageINFO: CL_IMAGE_DEPTH = %d\n", depth);

    info->fmt = fmt;
    info->esz = esz;
    info->pitch = pitch;
    info->slicep = slicep;
    info->width = width;
    info->height = height;
    info->depth = depth;

    return 0;
}

int main(int argc, char **argv)
{
    int major_ver, minor_ver;
    VADisplay va_dpy;
    VAStatus va_status;

    va_init_display_args(&argc, argv);

    va_dpy = va_open_display();
    va_status = vaInitialize(va_dpy, &major_ver, &minor_ver);
    assert(va_status == VA_STATUS_SUCCESS);

    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    cl_int err;

    cl_uint num_platforms = 0;
    err = clGetPlatformIDs(0, NULL, &num_platforms);
    CHECK_OCL_ERROR(err, "clGetPlatformIDs failed");
    printf("INFO: platform nubmer = %d\n", num_platforms);

    err = clGetPlatformIDs(1, &platform, NULL);
    CHECK_OCL_ERROR(err, "clGetPlatformIDs failed");

    char platform_info[CL_STR_LEN] = {};
    err = clGetPlatformInfo(platform, CL_PLATFORM_NAME, CL_STR_LEN, platform_info, NULL);
    CHECK_OCL_ERROR(err, "clGetPlatformInfo - CL_PLATFORM_NAME failed");
    printf("INFO: %s\n", platform_info);

    char platform_version[CL_STR_LEN] = {};
    err = clGetPlatformInfo(platform, CL_PLATFORM_VERSION, CL_STR_LEN, platform_version, NULL);
    CHECK_OCL_ERROR(err, "clGetPlatformInfo - CL_PLATFORM_VERSION failed");
    printf("INFO: %s\n", platform_version);

    if (getVASharingFunc(platform) != 0)
    {
        printf("ERROR: cannot get CL VA sharing extension interface!\n");
        return -1;
    }

    cl_device_id *device_list = NULL;
    cl_uint device_num = 0;

    err = clGetDeviceIDsFromVA(platform, CL_VA_API_DISPLAY_INTEL, va_dpy,
                               CL_PREFERRED_DEVICES_FOR_VA_API_INTEL, NULL, NULL, &device_num);
    CHECK_OCL_ERROR(err, "clGetDeviceIDsFromVA failed");

    device_list = (cl_device_id *)malloc(sizeof(cl_device_id) * device_num);
    CHECK_NULL_AND_RETURN(device_list);

    err = clGetDeviceIDsFromVA(platform, CL_VA_API_DISPLAY_INTEL, va_dpy,
                               CL_PREFERRED_DEVICES_FOR_VA_API_INTEL, device_num, device_list, NULL);
    CHECK_OCL_ERROR(err, "Can’t get OpenCL device for media sharing");

    // use the first device by default
    device = device_list[0];

    cl_context_properties props[] =
    {
        CL_CONTEXT_VA_API_DISPLAY_INTEL, (cl_context_properties)va_dpy,
        //CL_CONTEXT_INTEROP_USER_SYNC, CL_FALSE, // driver handle sync
        CL_CONTEXT_INTEROP_USER_SYNC, CL_TRUE, // app handle sync
        0
    };

    context = clCreateContext(props, device_num, device_list, NULL, NULL, &err);
    CHECK_OCL_ERROR(err, "clCreateContext failed");

    cl_mem img2d;
    struct clMemInfo memInfo = {};
    struct clImageInfo imgInfo = {};
    size_t width = 640;
    size_t height = 480;

#if USE_CL_NV12
    cl_image_format imgFormat = {};
    imgFormat.image_channel_order = CL_NV12_INTEL;
    imgFormat.image_channel_data_type = CL_UNORM_INT8;

    cl_image_desc imgDesc = {};
    imgDesc.image_type = CL_MEM_OBJECT_IMAGE2D;
    imgDesc.image_width = width;
    imgDesc.image_height = height;
    imgDesc.image_array_size = 0;
    imgDesc.image_row_pitch = 0;
    imgDesc.image_slice_pitch = 0;
    imgDesc.num_mip_levels = 0;
    imgDesc.num_samples = 0;
    imgDesc.mem_object = NULL;

    // create a CL_NV12_IMAGE
    img2d = clCreateImage(context, CL_MEM_READ_ONLY | CL_MEM_HOST_NO_ACCESS | CL_MEM_ACCESS_FLAGS_UNRESTRICTED_INTEL,
        &imgFormat, &imgDesc, NULL, err);
    CHECK_OCL_ERROR(err, "clCreateImage2D failed");

    getMemInfo(img2d, &memInfo);
    getImageInfo(img2d, &imgInfo);
#else
    /* since OCL doesn't have NV12 format, need allocate normal (one channel only) 2D image 
    that is big enough to hold NV12 surface Y and UV Planes */
    size_t nv12height = height * 3 / 2; 
    cl_image_format imgFormat = {};
    imgFormat.image_channel_order = CL_R;
    imgFormat.image_channel_data_type = CL_UNORM_INT8;

    img2d = clCreateImage2D(context, CL_MEM_WRITE_ONLY, &imgFormat, width, nv12height, 0, NULL, &err);
    CHECK_OCL_ERROR(err, "clCreateImage2D failed");

    getMemInfo(img2d, &memInfo);
    getImageInfo(img2d, &imgInfo);
#endif

    VASurfaceID surfExt;
    VASurfaceAttrib attrib[2] = {};
    VASurfaceAttribExternalBuffers extBuf = {};

    extBuf.pixel_format = VA_FOURCC_NV12;
    extBuf.width = imgInfo.width;
#if USE_CL_NV12
    extBuf.height = imgInfo.height;
#elif
    extBuf.height = height; // use real NV12 height instead of OCL queried height
#endif
    extBuf.pitches[0] = imgInfo.pitch;
    extBuf.buffers = &memInfo.handle;
    extBuf.num_buffers = 1;
    extBuf.flags = 0;
    extBuf.private_data = NULL;

    attrib[0].type = VASurfaceAttribExternalBufferDescriptor;
    attrib[0].flags = VA_SURFACE_ATTRIB_SETTABLE;
    attrib[0].value.type = VAGenericValueTypePointer;
    attrib[0].value.value.p = &extBuf;
    attrib[1].type = VASurfaceAttribMemoryType;
    attrib[1].flags = VA_SURFACE_ATTRIB_SETTABLE;
    attrib[1].value.type = VAGenericValueTypeInteger;
    attrib[1].value.value.i = VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME;

    va_status = vaCreateSurfaces(
        va_dpy,
        extBuf.pixel_format, 
        extBuf.width, 
        extBuf.height,
        &surfExt, 1,
        attrib, 2);
    CHECK_VASTATUS(va_status, "vaCreateSurfaces");

    vaDestroySurfaces(va_dpy, &surfExt, 1);

    vaTerminate(va_dpy);
    va_close_display(va_dpy);

    printf("INFO: execution exit.\n");

    return 0;
}