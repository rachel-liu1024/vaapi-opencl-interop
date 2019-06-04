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
#include "va_display.h"

#include <CL/cl.h>
#include <CL/cl_va_api_media_sharing_intel.h>

#define CL_STR_LEN 1024

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

int main(int argc, char **argv)
{
    VASurfaceID surface_id;
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

    cl_image_format imgFormat = {};
    imgFormat.image_channel_order = CL_R;
    imgFormat.image_channel_data_type = CL_UNORM_INT8;
    cl_mem outSurf = clCreateImage2D(context, CL_MEM_WRITE_ONLY, &imgFormat, 640, 480, 0, NULL, &err);
    CHECK_OCL_ERROR(err, "clCreateImage2D failed");

    va_status = vaCreateSurfaces(
        va_dpy,
        VA_RT_FORMAT_YUV420, 320, 240,
        &surface_id, 1,
        NULL, 0);
    CHECK_VASTATUS(va_status, "vaCreateSurfaces");

    vaDestroySurfaces(va_dpy, &surface_id, 1);

    vaTerminate(va_dpy);
    va_close_display(va_dpy);

    printf("INFO: execution exit.\n");

    return 0;
}