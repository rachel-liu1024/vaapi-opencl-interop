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

#define CHECK_VASTATUS(va_status, func)                                        \
    if (va_status != VA_STATUS_SUCCESS)                                        \
    {                                                                          \
        fprintf(stderr, "%s:%s (%d) failed,exit\n", __func__, func, __LINE__); \
        exit(1);                                                               \
    }

#define CHECK_NULL_AND_RETURN(ptr) \
    if ((ptr) == NULL)             \
        return -1;

#define CHECK_OCL_ERROR(err, msg)     \
    if ((err) < 0)                    \
    {                                 \
        printf("ERROR: %s\n", (msg)); \
        return -1;                    \
    }

static unsigned char mpeg2_clip[] = {
    0x00, 0x00, 0x01, 0xb3, 0x01, 0x00, 0x10, 0x13, 0xff, 0xff, 0xe0, 0x18, 0x00, 0x00, 0x01, 0xb5,
    0x14, 0x8a, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0xb8, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x0f, 0xff, 0xf8, 0x00, 0x00, 0x01, 0xb5, 0x8f, 0xff, 0xf3, 0x41, 0x80, 0x00,
    0x00, 0x01, 0x01, 0x13, 0xe1, 0x00, 0x15, 0x81, 0x54, 0xe0, 0x2a, 0x05, 0x43, 0x00, 0x2d, 0x60,
    0x18, 0x01, 0x4e, 0x82, 0xb9, 0x58, 0xb1, 0x83, 0x49, 0xa4, 0xa0, 0x2e, 0x05, 0x80, 0x4b, 0x7a,
    0x00, 0x01, 0x38, 0x20, 0x80, 0xe8, 0x05, 0xff, 0x60, 0x18, 0xe0, 0x1d, 0x80, 0x98, 0x01, 0xf8,
    0x06, 0x00, 0x54, 0x02, 0xc0, 0x18, 0x14, 0x03, 0xb2, 0x92, 0x80, 0xc0, 0x18, 0x94, 0x42, 0x2c,
    0xb2, 0x11, 0x64, 0xa0, 0x12, 0x5e, 0x78, 0x03, 0x3c, 0x01, 0x80, 0x0e, 0x80, 0x18, 0x80, 0x6b,
    0xca, 0x4e, 0x01, 0x0f, 0xe4, 0x32, 0xc9, 0xbf, 0x01, 0x42, 0x69, 0x43, 0x50, 0x4b, 0x01, 0xc9,
    0x45, 0x80, 0x50, 0x01, 0x38, 0x65, 0xe8, 0x01, 0x03, 0xf3, 0xc0, 0x76, 0x00, 0xe0, 0x03, 0x20,
    0x28, 0x18, 0x01, 0xa9, 0x34, 0x04, 0xc5, 0xe0, 0x0b, 0x0b, 0x04, 0x20, 0x06, 0xc0, 0x89, 0xff,
    0x60, 0x12, 0x12, 0x8a, 0x2c, 0x34, 0x11, 0xff, 0xf6, 0xe2, 0x40, 0xc0, 0x30, 0x1b, 0x7a, 0x01,
    0xa9, 0x0d, 0x00, 0xac, 0x64};

/* hardcoded here without a bitstream parser helper
 * please see picture mpeg2-I.jpg for bitstream details
 */
static VAPictureParameterBufferMPEG2 pic_param = {
    horizontal_size : 16,
    vertical_size : 16,
    forward_reference_picture : 0xffffffff,
    backward_reference_picture : 0xffffffff,
    picture_coding_type : 1,
    f_code : 0xffff,
    {
        {
            intra_dc_precision : 0,
            picture_structure : 3,
            top_field_first : 0,
            frame_pred_frame_dct : 1,
            concealment_motion_vectors : 0,
            q_scale_type : 0,
            intra_vlc_format : 0,
            alternate_scan : 0,
            repeat_first_field : 0,
            progressive_frame : 1,
            is_first_field : 1
        },
    }
};

/* see MPEG2 spec65 for the defines of matrix */
static VAIQMatrixBufferMPEG2 iq_matrix = {
    load_intra_quantiser_matrix : 1,
    load_non_intra_quantiser_matrix : 1,
    load_chroma_intra_quantiser_matrix : 0,
    load_chroma_non_intra_quantiser_matrix : 0,
    intra_quantiser_matrix : {
        8, 16, 16, 19, 16, 19, 22, 22,
        22, 22, 22, 22, 26, 24, 26, 27,
        27, 27, 26, 26, 26, 26, 27, 27,
        27, 29, 29, 29, 34, 34, 34, 29,
        29, 29, 27, 27, 29, 29, 32, 32,
        34, 34, 37, 38, 37, 35, 35, 34,
        35, 38, 38, 40, 40, 40, 48, 48,
        46, 46, 56, 56, 58, 69, 69, 83},
    non_intra_quantiser_matrix : {16},
    chroma_intra_quantiser_matrix : {0},
    chroma_non_intra_quantiser_matrix : {0}
};

static VASliceParameterBufferMPEG2 slice_param = {
    slice_data_size : 150,
    slice_data_offset : 0,
    slice_data_flag : 0,
    macroblock_offset : 38, /* 4byte + 6bits=38bits */
    slice_horizontal_position : 0,
    slice_vertical_position : 0,
    quantiser_scale_code : 2,
    intra_slice_flag : 0
};

#define CLIP_WIDTH 16
#define CLIP_HEIGHT 16

#define WIN_WIDTH (CLIP_WIDTH << 1)
#define WIN_HEIGHT (CLIP_HEIGHT << 1)

#define _CRT_SECURE_NO_WARNINGS
#define PROGRAM_FILE "convert.cl"
#define KERNEL_FUNC "scale"

#define CL_STR_LEN 1024

unsigned char output_ref[] = {
    0x13, 0x14, 0x13, 0x0e, 0x11, 0x17, 0x1a, 0x20, 0x25, 0x21, 0x1e, 0x1b, 0x11, 0x16, 0x1a, 0x1b, 
    0x13, 0x11, 0x14, 0x16, 0x1b, 0x20, 0x1e, 0x1e, 0x20, 0x23, 0x29, 0x2b, 0x1f, 0x18, 0x15, 0x1d, 
    0x13, 0x10, 0x17, 0x1c, 0x1d, 0x1e, 0x1d, 0x1d, 0x1e, 0x21, 0x28, 0x31, 0x2a, 0x19, 0x10, 0x20, 
    0x11, 0x0f, 0x19, 0x1f, 0x1c, 0x1b, 0x1b, 0x1c, 0x1c, 0x1d, 0x20, 0x2b, 0x2c, 0x1a, 0x0d, 0x22, 
    0x0f, 0x0e, 0x19, 0x20, 0x1e, 0x1d, 0x1c, 0x1c, 0x1c, 0x1d, 0x20, 0x28, 0x2e, 0x22, 0x14, 0x29, 
    0x0d, 0x11, 0x1a, 0x1c, 0x19, 0x1a, 0x1c, 0x1f, 0x1c, 0x18, 0x18, 0x1f, 0x2b, 0x2b, 0x1d, 0x2a, 
    0x0d, 0x15, 0x1d, 0x18, 0x14, 0x18, 0x1d, 0x22, 0x1f, 0x15, 0x14, 0x1a, 0x26, 0x2e, 0x20, 0x25, 
    0x0e, 0x18, 0x1e, 0x19, 0x1a, 0x20, 0x21, 0x22, 0x28, 0x1f, 0x22, 0x26, 0x2c, 0x33, 0x24, 0x25, 
    0x0e, 0x1e, 0x23, 0x1e, 0x1d, 0x1d, 0x19, 0x17, 0x1f, 0x1a, 0x1b, 0x24, 0x2b, 0x33, 0x27, 0x23, 
    0x0e, 0x1b, 0x1e, 0x1b, 0x1d, 0x1c, 0x17, 0x19, 0x1e, 0x1d, 0x1c, 0x1e, 0x20, 0x29, 0x25, 0x24, 
    0x0f, 0x1d, 0x1f, 0x1b, 0x1d, 0x1b, 0x16, 0x17, 0x13, 0x17, 0x19, 0x19, 0x1b, 0x24, 0x23, 0x23, 
    0x09, 0x17, 0x1d, 0x17, 0x13, 0x14, 0x12, 0x0e, 0x12, 0x16, 0x19, 0x1a, 0x1c, 0x20, 0x1f, 0x1e, 
    0x0d, 0x13, 0x1c, 0x1d, 0x18, 0x1a, 0x1c, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1d, 0x20, 0x25, 0x27, 
    0x0c, 0x09, 0x12, 0x1c, 0x1a, 0x1a, 0x1e, 0x1d, 0x1b, 0x1c, 0x1c, 0x1c, 0x20, 0x22, 0x2f, 0x34, 
    0x0e, 0x10, 0x16, 0x1c, 0x1d, 0x1a, 0x1a, 0x1d, 0x1d, 0x1e, 0x1e, 0x1c, 0x22, 0x25, 0x31, 0x33, 
    0x12, 0x21, 0x25, 0x21, 0x20, 0x1b, 0x17, 0x19, 0x19, 0x1a, 0x19, 0x18, 0x24, 0x2a, 0x35, 0x31, 
};

clGetDeviceIDsFromVA_APIMediaAdapterINTEL_fn clGetDeviceIDsFromVA = NULL;
clCreateFromVA_APIMediaSurfaceINTEL_fn clCreateFromVA = NULL;
clEnqueueAcquireVA_APIMediaSurfacesINTEL_fn clEnqueueAcquireVA = NULL;
clEnqueueReleaseVA_APIMediaSurfacesINTEL_fn clEnqueueReleaseVA = NULL;

static int enableReadWriteKernel = 1;

char kernel_code[] =
    "__kernel void scale(__read_write image2d_t image) \
{ \
    int2 coord = (int2)(get_global_id(0), get_global_id(1)); \
    float4 pixel = read_imagef(image, coord); \
    float4 pixel2 = pixel/4; \
    write_imagef(image, coord, pixel2); \
}";

char kernel_code2[] =
    "__kernel void scale(__read_only image2d_t image, __write_only image2d_t result) \
{ \
    int2 coord = (int2)(get_global_id(0), get_global_id(1)); \
    float4 pixel = read_imagef(image, coord); \
    float4 pixel2 = pixel/4; \
    write_imagef(result, coord, pixel2); \
}";

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

int oclProcessDecodeOutput(VADisplay vaDpy, VAContextID context_id, VASurfaceID *vaSurfID)
{
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

    err = clGetDeviceIDsFromVA(platform, CL_VA_API_DISPLAY_INTEL, vaDpy,
                               CL_PREFERRED_DEVICES_FOR_VA_API_INTEL, NULL, NULL, &device_num);
    CHECK_OCL_ERROR(err, "clGetDeviceIDsFromVA failed");

    device_list = (cl_device_id *)malloc(sizeof(cl_device_id) * device_num);
    CHECK_NULL_AND_RETURN(device_list);

    err = clGetDeviceIDsFromVA(platform, CL_VA_API_DISPLAY_INTEL, vaDpy,
                               CL_PREFERRED_DEVICES_FOR_VA_API_INTEL, device_num, device_list, NULL);
    CHECK_OCL_ERROR(err, "Can’t get OpenCL device for media sharing");

    // use the first device by default
    device = device_list[0];

    cl_context_properties props[] =
        {
            CL_CONTEXT_VA_API_DISPLAY_INTEL, (cl_context_properties)vaDpy,
            //CL_CONTEXT_INTEROP_USER_SYNC, CL_FALSE, // driver handle sync
            CL_CONTEXT_INTEROP_USER_SYNC, CL_TRUE, // app handle sync
            0
        };

    context = clCreateContext(props, device_num, device_list, NULL, NULL, &err);
    CHECK_OCL_ERROR(err, "clCreateContext failed");

    char* kernel_source = (enableReadWriteKernel) ? kernel_code : kernel_code2;
    char *program_buffer, *program_log;
    size_t program_size, log_size;
    program_size = strlen(kernel_source) + 1;
    program_buffer = (char *)malloc(program_size);
    memset(program_buffer, 0, program_size + 1);
    memcpy(program_buffer, kernel_source, program_size);
    program = clCreateProgramWithSource(context, 1, (const char **)&program_buffer, &program_size, &err);
    CHECK_OCL_ERROR(err, "clCreateProgramWithSource failed");
    free(program_buffer);

    // NOTE: in order to use __read_write qualifier for image2d_t in kernel, need to specifiy build
    // option as OpenCL2.0 or clBuildProgram will fail
    char build_options[] = "-cl-std=CL2.0";
    err = clBuildProgram(program, 1, &device, build_options, NULL, NULL);
    if (err < 0)
    {
        err = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        CHECK_OCL_ERROR(err, "clGetProgramBuildInfo failed");

        program_log = (char *)malloc(log_size + 1);
        memset(program_log, 0, log_size + 1);
        err = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size + 1, program_log, NULL);
        CHECK_OCL_ERROR(err, "clGetProgramBuildInfo failed");

        printf("%s\n", program_log);
        free(program_log);
        return -1;
    }

    cl_image_format imgFormat = {};
    imgFormat.image_channel_order = CL_R;
    imgFormat.image_channel_data_type = CL_UNORM_INT8;
    cl_mem outSurf = clCreateImage2D(context, CL_MEM_WRITE_ONLY, &imgFormat, CLIP_WIDTH, CLIP_HEIGHT, 0, NULL, &err);
    CHECK_OCL_ERROR(err, "clCreateImage2D failed");

    queue = clCreateCommandQueue(context, device, 0, &err);
    CHECK_OCL_ERROR(err, "clCreateCommandQueue failed");

    cl_mem sharedSurfY = clCreateFromVA(context, CL_MEM_READ_WRITE, vaSurfID, 0, &err);
    CHECK_OCL_ERROR(err, "clCreateFromVA failed");

    err = clEnqueueAcquireVA(queue, 1, &sharedSurfY, 0, NULL, NULL);
    CHECK_OCL_ERROR(err, "clEnqueueAcquireVA failed");

    kernel = clCreateKernel(program, KERNEL_FUNC, &err);
    CHECK_OCL_ERROR(err, "clCreateKernel failed");

    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &sharedSurfY);
    CHECK_OCL_ERROR(err, "clSetKernelArg failed");

    if (!enableReadWriteKernel) 
    {
        err = clSetKernelArg(kernel, 1, sizeof(cl_mem), &outSurf);
        CHECK_OCL_ERROR(err, "clSetKernelArg failed");
    }
    
    if(1)
    {
        VAStatus va_status;
        va_status = vaEndPicture(vaDpy, context_id);
        CHECK_VASTATUS(va_status, "vaEndPicture");
    }
    
    size_t global_size[2] = {CLIP_WIDTH, CLIP_HEIGHT};
    err = clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global_size, NULL, 0, NULL, NULL);
    CHECK_OCL_ERROR(err, "clEnqueueNDRangeKernel failed");

    // Wait until the queued kernel is completed by the device
    err = clFinish(queue);
    CHECK_OCL_ERROR(err, "clFinish failed");

    size_t origin[3] = {0, 0, 0};
    size_t region[3] = {CLIP_WIDTH, CLIP_HEIGHT, 1};
    unsigned char hostMem[CLIP_WIDTH * CLIP_HEIGHT] = {};
    cl_mem readSurf = (enableReadWriteKernel) ? sharedSurfY : outSurf;
    err = clEnqueueReadImage(queue, readSurf, CL_TRUE, origin, region, 0, 0, (void *)&hostMem[0], 0, NULL, NULL);
    CHECK_OCL_ERROR(err, "clEnqueueReadImage failed");

    // validate output
    char mismatch_count = 0;
    for(size_t y = 0; y < CLIP_HEIGHT; y++)
    {
        for(size_t x = 0; x < CLIP_WIDTH; x++)
        {
            mismatch_count += (hostMem[y * CLIP_WIDTH + x] != output_ref[y * CLIP_WIDTH + x]);
            printf("0x%02x, ", hostMem[y * CLIP_WIDTH + x]);
        }
        printf("\n");
    }

    if (mismatch_count) 
    {
        printf("ERROR: execution failed!!! mismatch count = %d\n", mismatch_count);
    }
    else
    {
        printf("INFO: execution success...\n");
    }

    err = clEnqueueReleaseVA(queue, 1, &sharedSurfY, 0, NULL, NULL);
    CHECK_OCL_ERROR(err, "clEnqueueReleaseVA failed");

    free(device_list);
    clReleaseMemObject(outSurf);
    clReleaseKernel(kernel);
    clReleaseCommandQueue(queue);
    clReleaseProgram(program);
    clReleaseContext(context);

    return 0;
}

int main(int argc, char **argv)
{
    VAEntrypoint entrypoints[5];
    int num_entrypoints, vld_entrypoint;
    VAConfigAttrib attrib;
    VAConfigID config_id;
    VASurfaceID surface_id;
    VAContextID context_id;
    VABufferID pic_param_buf, iqmatrix_buf, slice_param_buf, slice_data_buf;
    int major_ver, minor_ver;
    VADisplay va_dpy;
    VAStatus va_status;
    int putsurface = 1;

    va_init_display_args(&argc, argv);

    if (argc > 1)
        putsurface = 1;

    va_dpy = va_open_display();
    va_status = vaInitialize(va_dpy, &major_ver, &minor_ver);
    assert(va_status == VA_STATUS_SUCCESS);

    va_status = vaQueryConfigEntrypoints(va_dpy, VAProfileMPEG2Main, entrypoints,
                                         &num_entrypoints);
    CHECK_VASTATUS(va_status, "vaQueryConfigEntrypoints");

    for (vld_entrypoint = 0; vld_entrypoint < num_entrypoints; vld_entrypoint++)
    {
        if (entrypoints[vld_entrypoint] == VAEntrypointVLD)
            break;
    }
    if (vld_entrypoint == num_entrypoints)
    {
        /* not find VLD entry point */
        assert(0);
    }

    /* Assuming finding VLD, find out the format for the render target */
    attrib.type = VAConfigAttribRTFormat;
    vaGetConfigAttributes(va_dpy, VAProfileMPEG2Main, VAEntrypointVLD,
                          &attrib, 1);
    if ((attrib.value & VA_RT_FORMAT_YUV420) == 0)
    {
        /* not find desired YUV420 RT format */
        assert(0);
    }

    va_status = vaCreateConfig(va_dpy, VAProfileMPEG2Main, VAEntrypointVLD,
                               &attrib, 1, &config_id);
    CHECK_VASTATUS(va_status, "vaQueryConfigEntrypoints");

    va_status = vaCreateSurfaces(
        va_dpy,
        VA_RT_FORMAT_YUV420, CLIP_WIDTH, CLIP_HEIGHT,
        &surface_id, 1,
        NULL, 0);
    CHECK_VASTATUS(va_status, "vaCreateSurfaces");

    /* Create a context for this decode pipe */
    va_status = vaCreateContext(va_dpy, config_id,
                                CLIP_WIDTH,
                                ((CLIP_HEIGHT + 15) / 16) * 16,
                                VA_PROGRESSIVE,
                                &surface_id,
                                1,
                                &context_id);
    CHECK_VASTATUS(va_status, "vaCreateContext");

    va_status = vaCreateBuffer(va_dpy, context_id,
                               VAPictureParameterBufferType,
                               sizeof(VAPictureParameterBufferMPEG2),
                               1, &pic_param,
                               &pic_param_buf);
    CHECK_VASTATUS(va_status, "vaCreateBuffer");

    va_status = vaCreateBuffer(va_dpy, context_id,
                               VAIQMatrixBufferType,
                               sizeof(VAIQMatrixBufferMPEG2),
                               1, &iq_matrix,
                               &iqmatrix_buf);
    CHECK_VASTATUS(va_status, "vaCreateBuffer");

    va_status = vaCreateBuffer(va_dpy, context_id,
                               VASliceParameterBufferType,
                               sizeof(VASliceParameterBufferMPEG2),
                               1,
                               &slice_param, &slice_param_buf);
    CHECK_VASTATUS(va_status, "vaCreateBuffer");

    va_status = vaCreateBuffer(va_dpy, context_id,
                               VASliceDataBufferType,
                               0xc4 - 0x2f + 1,
                               1,
                               mpeg2_clip + 0x2f,
                               &slice_data_buf);
    CHECK_VASTATUS(va_status, "vaCreateBuffer");

    va_status = vaBeginPicture(va_dpy, context_id, surface_id);
    CHECK_VASTATUS(va_status, "vaBeginPicture");

    va_status = vaRenderPicture(va_dpy, context_id, &pic_param_buf, 1);
    CHECK_VASTATUS(va_status, "vaRenderPicture");

    va_status = vaRenderPicture(va_dpy, context_id, &iqmatrix_buf, 1);
    CHECK_VASTATUS(va_status, "vaRenderPicture");

    va_status = vaRenderPicture(va_dpy, context_id, &slice_param_buf, 1);
    CHECK_VASTATUS(va_status, "vaRenderPicture");

    va_status = vaRenderPicture(va_dpy, context_id, &slice_data_buf, 1);
    CHECK_VASTATUS(va_status, "vaRenderPicture");

    //va_status = vaEndPicture(va_dpy, context_id);
    //CHECK_VASTATUS(va_status, "vaEndPicture");

    if (0) 
    {
        va_status = vaSyncSurface(va_dpy, surface_id);
        CHECK_VASTATUS(va_status, "vaSyncSurface");
    }

    if (0)
    {
        uint8_t *buffer = NULL;
        VAImage image = {};
        VASurfaceStatus surf_status = VASurfaceSkipped;

        for (;;)
        {
            vaQuerySurfaceStatus(va_dpy, surface_id, &surf_status);
            if (surf_status != VASurfaceRendering &&
                surf_status != VASurfaceDisplaying)
                break;
        }

        if (surf_status != VASurfaceReady)
        {
            printf("Surface is not ready by vaQueryStatusSurface");
            return -1;
        }

        va_status = vaDeriveImage(va_dpy, surface_id, &image);
        CHECK_VASTATUS(va_status, "vaDeriveImage");

        va_status = vaMapBuffer(va_dpy, image.buf, (void **)&buffer);
        CHECK_VASTATUS(va_status, "vaDeriveImage");

        va_status = vaUnmapBuffer(va_dpy, image.buf);
        CHECK_VASTATUS(va_status, "vaUnmapBuffer");

        va_status = vaDestroyImage(va_dpy, image.image_id);
        CHECK_VASTATUS(va_status, "vaDestroyImage");
    }

    if (0)
    {
        VARectangle src_rect, dst_rect;

        src_rect.x = 0;
        src_rect.y = 0;
        src_rect.width = CLIP_WIDTH;
        src_rect.height = CLIP_HEIGHT;

        dst_rect.x = 0;
        dst_rect.y = 0;
        dst_rect.width = WIN_WIDTH;
        dst_rect.height = WIN_HEIGHT;

        va_status = va_put_surface(va_dpy, surface_id, &src_rect, &dst_rect);
        CHECK_VASTATUS(va_status, "vaPutSurface");
    }

    if (oclProcessDecodeOutput(va_dpy, context_id, &surface_id) != 0)
    {
        printf("ERROR: OCL execution failed\n");
        return -1;
    }

    vaDestroySurfaces(va_dpy, &surface_id, 1);
    vaDestroyConfig(va_dpy, config_id);
    vaDestroyContext(va_dpy, context_id);

    vaTerminate(va_dpy);
    va_close_display(va_dpy);

    printf("INFO: execution exit.\n");
    return 0;
}
