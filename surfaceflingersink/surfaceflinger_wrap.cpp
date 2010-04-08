/* GStreamer
 * Copyright (C) <2009> Prajnashi S <prajnashi@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
//#define ENABLE_GST_PLAYER_LOG
//



#include <cutils/log.h>
#include "surfaceflinger_wrap.h"


#define GST_PLAYER_INFO(format, ...) \
    test_print_log(__FILE__, __FUNCTION__,  __LINE__, format, ## __VA_ARGS__)

#define GST_PLAYER_ERROR(format, ...) \
    test_print_log(__FILE__, __FUNCTION__,  __LINE__, format, ## __VA_ARGS__)


const char* g_sLogPrefix = "surfacetest ";

void test_print_log(
        const char* file, 
        const char* func, 
        int line, 
        const char* format, ...)
{
    va_list var;
    const char* file_name;

    printf("%s: [%d] %d, %s(): ", 
        g_sLogPrefix, gettid(), line, func);

    va_start(var, format);
    vprintf(format, var);
    va_end(var);

    printf("\n");
    
    fflush(stdout);
}

namespace android {

/*
typedef struct 
{
    sp<MemoryHeapBase> frame_heap;
    sp<ISurface> isurface;
    sp<Surface> surface;
    int32_t hor_stride;
    int32_t ver_stride;
    uint32_t width;
    uint32_t height;
    PixelFormat format;
    int frame_offset[2];
    int buf_index;
} VideoFlingerDevice;
*/
/* max frame buffers */
#define  MAX_FRAME_BUFFERS     2

static int videoflinger_device_create_new_surface(VideoFlingerDevice* videodev);

/* 
 * The only purpose of class "MediaPlayer" is to call Surface::getISurface()
 * in frameworks/base/include/ui/Surface.h, which is private function and accessed
 * by friend class MediaPlayer.
 *
 * We define a fake one to cheat compiler
 */
class MediaPlayer
{
public:
    static sp<ISurface> getSurface(sp<Surface> surface)
    {
        return surface->getISurface();
    };
    static sp<ISurface> getSurface(sp<SurfaceControl> control)
    {
        return control->getISurface();
    };    
};


VideoFlingerDeviceHandle videoflinger_device_create(void* isurface)
{
    VideoFlingerDevice *videodev = NULL;

    GST_PLAYER_INFO("Enter\n");
    videodev = new VideoFlingerDevice;
    if (videodev == NULL)
    {
        return NULL;
    }
    videodev->frame_heap.clear();
    videodev->isurface = (ISurface*)isurface;
    videodev->control.clear();
    videodev->surface.clear();
    videodev->client.clear();
    videodev->format = -1;
    videodev->width = 0;
    videodev->height = 0;
    videodev->hor_stride = 0;
    videodev->ver_stride = 0;
    videodev->buf_index = 0;
    for ( int i = 0; i<MAX_FRAME_BUFFERS; i++)
    {
        videodev->frame_offset[i] = 0;
    }

    GST_PLAYER_INFO("Leave\n");
    return (VideoFlingerDeviceHandle)videodev;    
}



int videoflinger_device_create_new_surface(VideoFlingerDevice* videodev)
{
    DisplayInfo dinfo;
    status_t state;

    GST_PLAYER_INFO("Enter\n");

    /* release privious surface */
    videodev->surface.clear();
    videodev->isurface.clear();

    /* Create a new Surface object with 320x240 */
    sp<SurfaceComposerClient> videoClient = new SurfaceComposerClient;
    if (videoClient.get() == NULL)
    {
        GST_PLAYER_ERROR("Fail to create SurfaceComposerClient\n");
        return -1;
    }
    videodev->client = videoClient;
    state = videoClient->getDisplayInfo(0, &dinfo);
    if(state != NO_ERROR)
    {
        GST_PLAYER_ERROR("Fail to create display info\n");
        return -1;
    }
    GST_PLAYER_INFO(
            "width: %d, height: %d, orientation: %d, fps: %f, "
            "density: %f, xdpi: %f, ydpi: %f\n",
            dinfo.w,
            dinfo.h,
            dinfo.orientation,
            dinfo.fps,
            dinfo.density,
            dinfo.xdpi,
            dinfo.ydpi);

    int width = dinfo.w;
    int height = dinfo.h;
    sp<SurfaceControl> control;

    control = videoClient->createSurface(
            getpid(), 0, width, height, PIXEL_FORMAT_RGB_565, 
            ISurfaceComposer::ePushBuffers);
    if(control == NULL)
    {
        GST_PLAYER_ERROR("Fail to create SurfaceControl\n");
        return -1;
    }
    videodev->control = control;

    /* set Surface toppest z-order, this will bypass all isurface created 
     * in java side and make sure this surface displaied in toppest */
    videoClient->openTransaction();
    control->setLayer(INT_MAX);
    control->show(INT_MAX);
    videoClient->closeTransaction();

    /* get surface */
    videodev->surface = control->getSurface();
    if(videodev->surface == NULL)
    {
        GST_PLAYER_ERROR("Fail to create Surface\n");
        return -1;
    }

    /* get ISurface interface */
    //videodev->isurface = MediaPlayer::getSurface(videodev->surface);
    videodev->isurface = MediaPlayer::getSurface(videodev->control);

    /* Smart pointer videoClient shall be deleted automatically
     * when function exists */
    GST_PLAYER_INFO("Leave\n");


    return 0;
}

int videoflinger_device_release(VideoFlingerDeviceHandle handle)
{
    GST_PLAYER_INFO("Enter");
    
    if (handle == NULL)
    {
        return -1;
    }
    
    /* unregister frame buffer */
    videoflinger_device_unregister_framebuffers(handle); 

    /* release ISurface & Surface */
    VideoFlingerDevice *videodev = (VideoFlingerDevice*)handle;
    videodev->isurface.clear();
    videodev->surface.clear();
    videodev->control.clear();
    videodev->client.clear();

    /* delete device */
    delete videodev;

    GST_PLAYER_INFO("Leave");
    return 0;
}

int videoflinger_device_register_framebuffers(VideoFlingerDeviceHandle handle, 
    int w, int h, VIDEO_FLINGER_PIXEL_FORMAT format)
{
    int surface_format = 0;

    GST_PLAYER_INFO("Enter");
    if (handle == NULL)
    {
        GST_PLAYER_ERROR("videodev is NULL");
        return -1;
    }
   
    /* TODO: Now, only PIXEL_FORMAT_RGB_565 is supported. Change here to support
     * more pixel type
     */
    if (format !=  VIDEO_FLINGER_RGB_565 )
    {
        GST_PLAYER_ERROR("Unsupport format: %d", format);
        return -1;
    }
    surface_format = PIXEL_FORMAT_RGB_565;
   
    VideoFlingerDevice *videodev = (VideoFlingerDevice*)handle;
    /* unregister previous buffers */
    if (videodev->frame_heap.get())
    {
        videoflinger_device_unregister_framebuffers(handle);
    }

    /* reset framebuffers */
    videodev->format = surface_format;
    videodev->width = (w + 1) & -2;
    videodev->height = (h + 1) & -2;
    videodev->hor_stride = videodev->width;
    videodev->ver_stride =  videodev->height;
    
    /* create isurface internally, if no ISurface interface input */
    if (videodev->isurface.get() == NULL)
    {
        videoflinger_device_create_new_surface(videodev);
    }

    /* use double buffer in post */
    int frameSize = videodev->width * videodev->height * 2;
    GST_PLAYER_INFO( 
        "format=%d, width=%d, height=%d, hor_stride=%d, ver_stride=%d, frameSize=%d\n",
        videodev->format,
        videodev->width,
        videodev->height,
        videodev->hor_stride,
        videodev->ver_stride,
        frameSize);

    /* create frame buffer heap base */
    videodev->frame_heap = new MemoryHeapBase(frameSize * MAX_FRAME_BUFFERS);
    if (videodev->frame_heap->heapID() < 0) 
    {
        GST_PLAYER_ERROR("Error creating frame buffer heap!");
        return -1;
    }

    /* create frame buffer heap and register with surfaceflinger */
    ISurface::BufferHeap buffers(
        videodev->width,
        videodev->height,
        videodev->hor_stride,
        videodev->ver_stride,
        videodev->format,
        0,
        0,
        videodev->frame_heap);

    status_t status;
    status = videodev->isurface->registerBuffers(buffers);
    if (status < 0 )
    {
        GST_PLAYER_ERROR("Cannot register frame buffer, state=%d", status);
        videodev->frame_heap.clear();
        return -1;
    }

    for ( int i = 0; i<MAX_FRAME_BUFFERS; i++)
    {
        videodev->frame_offset[i] = i*frameSize;
    }
    videodev->buf_index = 0;
    GST_PLAYER_INFO("Leave");

    return 0;
}

void videoflinger_device_unregister_framebuffers(VideoFlingerDeviceHandle handle)
{
    GST_PLAYER_INFO("Enter");

    if (handle == NULL)
    {
        return;
    }

    VideoFlingerDevice* videodev = (VideoFlingerDevice*)handle;
    if (videodev->frame_heap.get())
    {
        GST_PLAYER_INFO("Unregister frame buffers.  videodev->isurface = %p", videodev->isurface.get());
        
        /* release ISurface */
        GST_PLAYER_INFO("Unregister frame buffer");
        videodev->isurface->unregisterBuffers();

        /* release MemoryHeapBase */
        GST_PLAYER_INFO("Clear frame buffers.");
        videodev->frame_heap.clear();

        /* reset offset */
        for (int i = 0; i<MAX_FRAME_BUFFERS; i++)
        {
            videodev->frame_offset[i] = 0;
        }
            
        videodev->format = -1;
        videodev->width = 0;
        videodev->height = 0;
        videodev->hor_stride = 0;
        videodev->ver_stride = 0;
        videodev->buf_index = 0;
    }

    GST_PLAYER_INFO("Leave");
}

void videoflinger_device_post(VideoFlingerDeviceHandle handle, void * buf, int bufsize)
{
    // GST_PLAYER_INFO("Enter");
    
    if (handle == NULL)
    {
        return;
    }

    VideoFlingerDevice* videodev = (VideoFlingerDevice*)handle;
    
    if (++videodev->buf_index == MAX_FRAME_BUFFERS) 
        videodev->buf_index = 0;
   
    memcpy (static_cast<unsigned char *>(videodev->frame_heap->base()) + videodev->frame_offset[videodev->buf_index],  buf, bufsize);

    GST_PLAYER_INFO ("Post buffer[%d].\n", videodev->buf_index);
    videodev->isurface->postBuffer(videodev->frame_offset[videodev->buf_index]);

    // GST_PLAYER_INFO("Leave");
}


};
