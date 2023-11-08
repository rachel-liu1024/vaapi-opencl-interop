/*
 * Copyright (c) 2012 Intel Corporation. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <va/va.h>
#include <va/va_drm.h>
#include "va_display.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>

extern  const char *g_display_name;
extern const char *g_drm_device_name;
int g_drm_fd = 0;


VADisplay
va_open_display_drm(void)
{
    VADisplay va_dpy = NULL;
    unsigned int i;
    if(!g_drm_device_name)
        g_drm_device_name = "/dev/dri/renderD128";//card0 failed in clGetDeviceIDsFromVA_APIMediaAdapterINTEL

    g_drm_fd = open(g_drm_device_name, O_RDWR);
    if (g_drm_fd < 0)
    {
        printf("Cannot open %s: %d, %s.\n", g_drm_device_name, errno, strerror(errno));
        return -1;
    }
    va_dpy = vaGetDisplayDRM(g_drm_fd);
    int          major_ver, minor_ver;
    VAStatus    vaStatus = VA_STATUS_ERROR_UNKNOWN;

    vaStatus = vaInitialize(va_dpy, &major_ver, &minor_ver);
    if (vaStatus != VA_STATUS_SUCCESS) {
        fprintf(stderr,"Error: Failed vaInitialize(): in %s %s(line %d)\n", __FILE__, __func__, __LINE__);
        return -1;
    }
    printf("libva version: %d.%d\n", major_ver, minor_ver);

    const char  *driver = NULL;
    driver = vaQueryVendorString(va_dpy);
    printf("VAAPI Init complete; Driver version : %d\n", driver);


    if (!va_dpy)  {
        fprintf(stderr, "error: failed to initialize display");
        if (g_display_name)
            fprintf(stderr, " '%s'", g_display_name);
        fprintf(stderr, "\n");
        exit(1);
    }
    return va_dpy;
}

void
va_close_display_drm(VADisplay va_dpy)
{
    if (!va_dpy)
        return;

    vaTerminate(va_dpy);
    va_dpy = NULL;
    close(g_drm_fd);
    g_drm_fd = -1;
}

const VADisplayHooks va_display_hooks_drm = {
    "drm",
    va_open_display_drm,
    va_close_display_drm,
    NULL
};
