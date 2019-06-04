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