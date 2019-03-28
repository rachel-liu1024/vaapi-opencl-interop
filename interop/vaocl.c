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
//#include <CL/va_ext.h>

#define CHECK_VASTATUS(va_status, func)                                        \
    if (va_status != VA_STATUS_SUCCESS)                                        \
    {                                                                          \
        fprintf(stderr, "%s:%s (%d) failed,exit\n", __func__, func, __LINE__); \
        exit(1);                                                               \
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
#define PROGRAM_FILE "matvec.cl"
#define KERNEL_FUNC "matvec_mult"

int compute()
{
    /* Host/device data structures */
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_int i, err;

    /* Program/kernel data structures */
    cl_program program;
    FILE *program_handle;
    char *program_buffer, *program_log;
    size_t program_size, log_size;
    cl_kernel kernel;

    /* Data and buffers */
    float mat[16], vec[4], result[4];
    float correct[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    cl_mem mat_buff, vec_buff, res_buff;
    size_t work_units_per_kernel;

    /* Initialize data to be processed by the kernel */
    for (i = 0; i < 16; i++)
    {
        mat[i] = i * 2.0f;
    }
    for (i = 0; i < 4; i++)
    {
        vec[i] = i * 3.0f;
        correct[0] += mat[i] * vec[i];
        correct[1] += mat[i + 4] * vec[i];
        correct[2] += mat[i + 8] * vec[i];
        correct[3] += mat[i + 12] * vec[i];
    }

    /* Identify a platform */
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err < 0)
    {
        perror("Couldn't find any platforms");
        exit(1);
    }

    /* Access a device */
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err < 0)
    {
        perror("Couldn't find any devices");
        exit(1);
    }

    /* Create the context */
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err < 0)
    {
        perror("Couldn't create a context");
        exit(1);
    }

    size_t len = 0; 
    clGetPlatformInfo(platform, CL_PLATFORM_EXTENSIONS, 0, NULL, &len);

    // allocate buffer to store extensions names
    char *str = (char *)malloc(len);

    //get string with extension names
    clGetPlatformInfo(platform, CL_PLATFORM_EXTENSIONS, len, str, NULL);
    printf("%s\n", str);

    /* Read program file and place content into buffer */
    program_handle = fopen(PROGRAM_FILE, "r");
    if (program_handle == NULL)
    {
        perror("Couldn't find the program file");
        exit(1);
    }
    fseek(program_handle, 0, SEEK_END);
    program_size = ftell(program_handle);
    rewind(program_handle);
    program_buffer = (char *)malloc(program_size + 1);
    memset(program_buffer, 0, program_size + 1);
    //program_buffer[program_size] = '\0';
    fread(program_buffer, sizeof(char), program_size, program_handle);
    fclose(program_handle);

    /* Create program from file */
    program = clCreateProgramWithSource(context, 1,
                                        (const char **)&program_buffer, &program_size, &err);
    if (err < 0)
    {
        perror("Couldn't create the program");
        exit(1);
    }
    free(program_buffer);

    /* Build program */
    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if (err < 0)
    {

        /* Find size of log and print to std output */
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG,
                              0, NULL, &log_size);
        program_log = (char *)malloc(log_size + 1);
        memset(program_log, 0, log_size + 1);
        //program_log[log_size] = '\0';
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG,
                              log_size + 1, program_log, NULL);
        printf("%s\n", program_log);
        free(program_log);
        exit(1);
    }

    /* Create kernel for the mat_vec_mult function */
    kernel = clCreateKernel(program, KERNEL_FUNC, &err);
    if (err < 0)
    {
        perror("Couldn't create the kernel");
        exit(1);
    }

    /* Create CL buffers to hold input and output data */
    mat_buff = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float) * 16, mat, &err);
    if (err < 0)
    {
        perror("Couldn't create a buffer object");
        exit(1);
    }
    vec_buff = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float) * 4, vec, NULL);
    res_buff = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                              sizeof(float) * 4, NULL, NULL);

    /* Create kernel arguments from the CL buffers */
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &mat_buff);
    if (err < 0)
    {
        perror("Couldn't set the kernel argument");
        exit(1);
    }
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &vec_buff);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), &res_buff);

    /* Create a CL command queue for the device*/
    queue = clCreateCommandQueue(context, device, 0, &err);
    if (err < 0)
    {
        perror("Couldn't create the command queue");
        exit(1);
    }

    /* Enqueue the command queue to the device */
    work_units_per_kernel = 4; /* 4 work-units per kernel */
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &work_units_per_kernel,
                                 NULL, 0, NULL, NULL);
    if (err < 0)
    {
        perror("Couldn't enqueue the kernel execution command");
        exit(1);
    }

    /* Read the result */
    err = clEnqueueReadBuffer(queue, res_buff, CL_TRUE, 0, sizeof(float) * 4,
                              result, 0, NULL, NULL);
    if (err < 0)
    {
        perror("Couldn't enqueue the read buffer command");
        exit(1);
    }

    /* Test the result */
    if ((result[0] == correct[0]) && (result[1] == correct[1]) && (result[2] == correct[2]) && (result[3] == correct[3]))
    {
        printf("Matrix-vector multiplication successful.\n");
    }
    else
    {
        printf("Matrix-vector multiplication unsuccessful.\n");
    }

    /* Deallocate resources */
    clReleaseMemObject(mat_buff);
    clReleaseMemObject(vec_buff);
    clReleaseMemObject(res_buff);
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

    va_status = vaEndPicture(va_dpy, context_id);
    CHECK_VASTATUS(va_status, "vaEndPicture");

    va_status = vaSyncSurface(va_dpy, surface_id);
    CHECK_VASTATUS(va_status, "vaSyncSurface");

    if (1)
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

    compute();

    vaDestroySurfaces(va_dpy, &surface_id, 1);
    vaDestroyConfig(va_dpy, config_id);
    vaDestroyContext(va_dpy, context_id);

    vaTerminate(va_dpy);
    va_close_display(va_dpy);

    printf("done.\n");
    return 0;
}
