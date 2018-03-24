#include "v4lFFGL.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>             /* getopt_long() */
#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>
#include <libv4lconvert.h>

#include <pthread.h>


#define CLEAR(x) memset(&(x), 0, sizeof(x))


#include "error_image.h"


enum io_method {
    IO_METHOD_READ,
    IO_METHOD_MMAP,
    IO_METHOD_USERPTR,
};

struct buffer {
    void   *start;
    size_t  length;
};

class video4LinuxFreeFrameGLData {

public:

    char             dev_name[256];
    enum io_method   io;
    int              fd;
    struct buffer    *buffers;
    unsigned int     n_buffers;
    int              force_format;

    unsigned char *  _glbuffer[2];
    GLuint draw_buffer;
    GLuint read_buffer;
    GLuint textureIndex;
    GLuint errorTextureIndex;
    int width;
    int height;
    struct v4lconvert_data *m_convertData;
    struct v4l2_format m_capSrcFormat;
    struct v4l2_format m_capDestFormat;

    pthread_t thread;
    pthread_mutex_t mutex;
    bool stop;

    video4LinuxFreeFrameGLData(){

        sprintf(dev_name, " ");
        // Using MMAP method: apparently the most robust.
        io = IO_METHOD_MMAP;
        fd = -1;
        force_format = true;
        draw_buffer = 0;
        read_buffer = 1;
        textureIndex = 0;
        errorTextureIndex = 0;
        width = 0;
        height = 0;
        mutex = PTHREAD_MUTEX_INITIALIZER;
        stop = true;
        _glbuffer[0] = 0;
        _glbuffer[1] = 0;
    }

};


void *update_thread(void *c);

#define FFPARAM_DEVICE 0

int errno_exit(const char *s)
{
    fprintf(stderr, "v4l2 - %s error %d, %s\n", s, errno, strerror(errno));
//    exit(EXIT_FAILURE);
    return errno;
}

int xioctl(int fh, int request, void *arg)
{
    int r;

    do {
        r = ioctl(fh, request, arg);
    } while (-1 == r && EINTR == errno);

    return r;
}


void process_image(const void *p, int size, video4LinuxFreeFrameGLData *current)
{

    pthread_mutex_lock( &(current->mutex) );
    GLuint tmp = current->read_buffer;
    current->read_buffer = current->draw_buffer;
    current->draw_buffer = tmp;
    pthread_mutex_unlock( &(current->mutex) );

    if ( current->m_capSrcFormat.fmt.pix.pixelformat != V4L2_PIX_FMT_YUYV)
    {
        int s = 0;
        s = v4lconvert_convert(current->m_convertData, &(current->m_capSrcFormat),
                               &(current->m_capDestFormat),
                               (unsigned char *)p, size,
                               (unsigned char *)current->_glbuffer[current->draw_buffer],
                current->m_capDestFormat.fmt.pix.sizeimage);

        if ( s == -1 || s != int(current->m_capDestFormat.fmt.pix.sizeimage) )
            fprintf(stderr, "%s", v4lconvert_get_error_message(current->m_convertData));

    }
    // custom YUV to RGB conversion (
    else {
        int y;
        int cr;
        int cb;
        int i, j;

        double r;
        double g;
        double b;
        unsigned char* rgb_image = (unsigned char *)current->_glbuffer[current->draw_buffer];
        unsigned char* yuyv_image = (unsigned char *)p;

        for (i = 0, j = 0; i < current->width * current->height * 3; i+=6, j+=4) {
            //first pixel
            y = yuyv_image[j];
            cb = yuyv_image[j+1];
            cr = yuyv_image[j+3];

            r = y + (1.4065 * (cr - 128));
            g = y - (0.3455 * (cb - 128)) - (0.7169 * (cr - 128));
            b = y + (1.7790 * (cb - 128));

            //This prevents colour distortions in your rgb image
            if (r < 0) r = 0;
            else if (r > 255) r = 255;
            if (g < 0) g = 0;
            else if (g > 255) g = 255;
            if (b < 0) b = 0;
            else if (b > 255) b = 255;

            rgb_image[i] = (unsigned char)r;
            rgb_image[i+1] = (unsigned char)g;
            rgb_image[i+2] = (unsigned char)b;

            //second pixel
            y = yuyv_image[j+2];
            cb = yuyv_image[j+1];
            cr = yuyv_image[j+3];

            r = y + (1.4065 * (cr - 128));
            g = y - (0.3455 * (cb - 128)) - (0.7169 * (cr - 128));
            b = y + (1.7790 * (cb - 128));

            if (r < 0) r = 0;
            else if (r > 255) r = 255;
            if (g < 0) g = 0;
            else if (g > 255) g = 255;
            if (b < 0) b = 0;
            else if (b > 255) b = 255;

            rgb_image[i+3] = (unsigned char)r;
            rgb_image[i+4] = (unsigned char)g;
            rgb_image[i+5] = (unsigned char)b;
        }
    }

}

int read_frame(video4LinuxFreeFrameGLData *current)
{
    struct v4l2_buffer buf;
    unsigned int i;

    switch (current->io) {
    case IO_METHOD_READ:
        if (-1 == read(current->fd, current->buffers[0].start, current->buffers[0].length)) {
            switch (errno) {
            case EAGAIN:
                // retry
                return 0;
            case EIO:
                /* Could ignore EIO, see spec. */
                /* fall through */
            default:
                current->stop = true;
                return errno_exit("read");
            }
        }

        process_image(current->buffers[0].start, current->buffers[0].length, current);
        break;

    case IO_METHOD_MMAP:
        CLEAR(buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        if (-1 == xioctl(current->fd, VIDIOC_DQBUF, &buf)) {
            switch (errno) {
            case EAGAIN:
                // retry
                return 0;
            case EIO:
                /* Could ignore EIO, see spec. */
                /* fall through */
            default:
                current->stop = true;
                return errno_exit("VIDIOC_DQBUF");
            }
        }

        process_image(current->buffers[buf.index].start, buf.bytesused, current);

        if (-1 == xioctl(current->fd, VIDIOC_QBUF, &buf)) {
            current->stop = true;
            return errno_exit("VIDIOC_QBUF");
        }

        break;

    case IO_METHOD_USERPTR:
        CLEAR(buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_USERPTR;

        if (-1 == xioctl(current->fd, VIDIOC_DQBUF, &buf)) {
            switch (errno) {
            case EAGAIN:
                // retry
                return 0;
            case EIO:
                /* Could ignore EIO, see spec. */
                /* fall through */
            default:
                current->stop = true;
                return errno_exit("VIDIOC_DQBUF");
            }
        }

        for (i = 0; i < current->n_buffers; ++i)
            if (buf.m.userptr == (unsigned long)current->buffers[i].start
                    && buf.length == current->buffers[i].length)
                break;


        process_image((void *)buf.m.userptr, buf.bytesused, current);

        if (-1 == xioctl(current->fd, VIDIOC_QBUF, &buf)){
            current->stop = true;
            return errno_exit("VIDIOC_QBUF");
        }

        break;
    }

    return 1;
}

void update(video4LinuxFreeFrameGLData *current)
{

    for (;;)
    {
        fd_set fds;
        struct timeval tv;
        int r;

        FD_ZERO(&fds);
        FD_SET(current->fd, &fds);

        /* Timeout. */
        tv.tv_sec = 2;
        tv.tv_usec = 0;

        r = select(current->fd + 1, &fds, NULL, NULL, &tv);

        if (-1 == r) {
            if (EINTR == errno)
                continue;
            errno_exit("select");
        }

        if (0 == r) {
            fprintf(stderr, "v4l2 - select timeout\n");
            break;
        }

        if (read_frame(current))
            break;
        /* EAGAIN - continue select loop. */
    }


}


bool stop_capturing(video4LinuxFreeFrameGLData *current)
{
    enum v4l2_buf_type type;

    switch (current->io) {
    case IO_METHOD_READ:
        /* Nothing to do. */
        break;

    case IO_METHOD_MMAP:
    case IO_METHOD_USERPTR:
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (-1 == xioctl(current->fd, VIDIOC_STREAMOFF, &type))
            return false;
        break;
    }

    fprintf(stderr, "v4l2 - Stop Capturing %s\n", current->dev_name);

    return true;
}

bool start_capturing(video4LinuxFreeFrameGLData *current)
{
    unsigned int i;
    enum v4l2_buf_type type;

    switch (current->io) {
    case IO_METHOD_READ:
        /* Nothing to do. */
        break;

    case IO_METHOD_MMAP:
        for (i = 0; i < current->n_buffers; ++i) {
            struct v4l2_buffer buf;

            CLEAR(buf);
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = i;

            if (-1 == xioctl(current->fd, VIDIOC_QBUF, &buf))
                errno_exit("VIDIOC_QBUF");
        }
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (-1 == xioctl(current->fd, VIDIOC_STREAMON, &type))
            return false;
        break;

    case IO_METHOD_USERPTR:
        for (i = 0; i < current->n_buffers; ++i) {
            struct v4l2_buffer buf;

            CLEAR(buf);
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_USERPTR;
            buf.index = i;
            buf.m.userptr = (unsigned long)current->buffers[i].start;
            buf.length = current->buffers[i].length;

            if (-1 == xioctl(current->fd, VIDIOC_QBUF, &buf))
                return false;
        }
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (-1 == xioctl(current->fd, VIDIOC_STREAMON, &type))
            return false;
        break;
    }

    return true;
}

bool uninit_device(video4LinuxFreeFrameGLData *current)
{
    unsigned int i;

    if (!current || -1 == current->fd)
        return false;

    switch (current->io) {
    case IO_METHOD_READ:
        free(current->buffers[0].start);
        break;

    case IO_METHOD_MMAP:
        for (i = 0; i < current->n_buffers; ++i)
            if (-1 == munmap(current->buffers[i].start, current->buffers[i].length))
                return false;
        break;

    case IO_METHOD_USERPTR:
        for (i = 0; i < current->n_buffers; ++i)
            free(current->buffers[i].start);
        break;
    }

    free(current->buffers);
    free(current->_glbuffer[0]);
    free(current->_glbuffer[1]);

    v4lconvert_destroy(current->m_convertData);

    fprintf(stderr, "v4l2 - Released device %s\n", current->dev_name);

    return true;
}

bool init_read(unsigned int buffer_size, video4LinuxFreeFrameGLData *current)
{
    current->buffers = (buffer *) calloc(1, sizeof(*(current->buffers)));

    if (!current->buffers) {
        fprintf(stderr, "v4l2 - Out of memory\n");
        return false;
    }

    current->buffers[0].length = buffer_size;
    current->buffers[0].start = malloc(buffer_size);

    if (!current->buffers[0].start) {
        fprintf(stderr, "v4l2 - Out of memory\n");
        return false;
    }

    return true;
}

bool init_mmap(video4LinuxFreeFrameGLData *current)
{
    struct v4l2_requestbuffers req;

    CLEAR(req);

    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl(current->fd, VIDIOC_REQBUFS, &req)) {
        if (EINVAL == errno) {
            fprintf(stderr, "v4l2 - %s does not support memory mapping\n", current->dev_name);
        }
        return false;
    }

    if (req.count < 2) {
        fprintf(stderr, "v4l2 - Insufficient buffer memory on %s\n", current->dev_name);
        return false;
    }

    current->buffers = (buffer *) calloc(req.count, sizeof(*(current->buffers)));

    if (!current->buffers) {
        fprintf(stderr, "v4l2 - Out of memory\n");
        return false;
    }

    for (current->n_buffers = 0; current->n_buffers < req.count; ++current->n_buffers) {
        struct v4l2_buffer buf;

        CLEAR(buf);

        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = current->n_buffers;

        if (-1 == xioctl(current->fd, VIDIOC_QUERYBUF, &buf))
            return false;

        current->buffers[current->n_buffers].length = buf.length;
        current->buffers[current->n_buffers].start =
                mmap(NULL /* start anywhere */,
                     buf.length,
                     PROT_READ | PROT_WRITE /* required */,
                     MAP_SHARED /* recommended */,
                     current->fd, buf.m.offset);

        if (MAP_FAILED == current->buffers[current->n_buffers].start)
            return false;
        // TODO unmap and free if not exit
    }

    return true;
}

bool init_userp(unsigned int buffer_size, video4LinuxFreeFrameGLData *current)
{
    struct v4l2_requestbuffers req;

    CLEAR(req);

    req.count  = 4;
    req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_USERPTR;

    if (-1 == xioctl(current->fd, VIDIOC_REQBUFS, &req)) {
        if (EINVAL == errno) {
            fprintf(stderr, "v4l2 - %s does not support user pointer i/o\n", current->dev_name);
        }

        return false;
    }

    current->buffers = (buffer *) calloc(4, sizeof(*(current->buffers)));

    if (!current->buffers) {
        fprintf(stderr, "v4l2 - Out of memory\n");
        return false;
    }

    for (current->n_buffers = 0; current->n_buffers < 4; ++current->n_buffers) {
        current->buffers[current->n_buffers].length = buffer_size;
        current->buffers[current->n_buffers].start = malloc(buffer_size);

        if (!current->buffers[current->n_buffers].start) {
            fprintf(stderr, "v4l2 - Out of memory\n");
            return false;
        }
    }

//    fprintf(stderr, "init userp %d x %d \n", 4, buffer_size);

    return true;
}

bool init_device(video4LinuxFreeFrameGLData *current)
{
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
    unsigned int min;

    if (-1 == current->fd)
        return false;

    if (-1 == xioctl(current->fd, VIDIOC_QUERYCAP, &cap)) {
        if (EINVAL == errno) {
            fprintf(stderr, "v4l2 - %s is no Video4Linux device\n", current->dev_name);
        }
        return false;
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        fprintf(stderr, "v4l2 - %s is no video capture device\n", current->dev_name);
        return false;
    }

    switch (current->io) {
    case IO_METHOD_READ:
        if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
            fprintf(stderr, "v4l2 - %s does not support read i/o\n", current->dev_name);
            return false;
        }
        break;

    case IO_METHOD_MMAP:
    case IO_METHOD_USERPTR:
        if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
            fprintf(stderr, "v4l2 - %s does not support streaming i/o\n",  current->dev_name);
            return false;
        }
        break;
    }


    /* Select video input, video standard and tune here. */


//    CLEAR(cropcap);

//    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

//    if (0 == xioctl(current->fd, VIDIOC_CROPCAP, &cropcap)) {
//        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//        crop.c = cropcap.defrect; /* reset to default */

//        if (-1 == xioctl(current->fd, VIDIOC_S_CROP, &crop)) {
//            switch (errno) {
//            case EINVAL:
//                /* Cropping not supported. */
//                break;
//            default:
//                /* Errors ignored. */
//                break;
//            }
//        }
//    } else {
//        /* Errors ignored. */
//    }


    CLEAR(fmt);

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (current->force_format) {
        fmt.fmt.pix.width       = 1024;
        fmt.fmt.pix.height      = 768;
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
        fmt.fmt.pix.field       = V4L2_FIELD_NONE;
        // try to get those parameters from the camera
        if (-1 == xioctl(current->fd, VIDIOC_S_FMT, &fmt))
            return false;
        /* Note VIDIOC_S_FMT may change pixelformat, width and height depending on camera */
    }
    else {
        /* Preserve original settings as set by v4l2-ctl for example */
        if (-1 == xioctl(current->fd, VIDIOC_G_FMT, &fmt))
            return false;
    }

    /* Buggy driver paranoia. */
    min = fmt.fmt.pix.width * 2;
    if (fmt.fmt.pix.bytesperline < min)
        fmt.fmt.pix.bytesperline = min;
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if (fmt.fmt.pix.sizeimage < min)
        fmt.fmt.pix.sizeimage = min;
    v4lconvert_fixup_fmt(&(current->m_capSrcFormat));
    v4lconvert_fixup_fmt(&(current->m_capDestFormat));

    // remember width, height and format of the input
    current->width = fmt.fmt.pix.width;
    current->height = fmt.fmt.pix.height;
    current->m_capSrcFormat = fmt;

    // format for destination is the same, except for the PIXELS
    current->m_capDestFormat = fmt;
    current->m_capDestFormat.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;

    current->m_convertData = v4lconvert_create(current->fd);
//    current->m_convertData


    int err = 0;
    err = v4lconvert_try_format(current->m_convertData, &(current->m_capDestFormat), &(current->m_capSrcFormat));
    if (err == -1) {

        fprintf(stderr, "v4l2 - Failed to create converter.\n");
        return false;
    }

//    fprintf(stderr, "v4lconvert_try_format passed\n");

    // Allocate OpenGL buffers
    current->_glbuffer[0] = (unsigned char *) malloc(current->m_capDestFormat.fmt.pix.sizeimage);
    current->_glbuffer[1] = (unsigned char *) malloc(current->m_capDestFormat.fmt.pix.sizeimage);

    // initialize
    switch (current->io) {
    case IO_METHOD_READ:
        init_read(fmt.fmt.pix.sizeimage, current);
//        fprintf(stderr, "v4l2 - using IO_METHOD_READ\n");
        break;

    case IO_METHOD_USERPTR:
        init_userp(fmt.fmt.pix.sizeimage,current);
//        fprintf(stderr, "v4l2 - using IO_METHOD_USERPTR\n");
        break;

    default:
    case IO_METHOD_MMAP:
        init_mmap(current);
//        fprintf(stderr, "v4l2 - using IO_METHOD_MMAP\n");
        break;
    }


    fprintf(stderr, "v4l2 - Device %s initialized ; %d x %d ", current->dev_name, current->width, current->height);
    fprintf(stderr, " %c%c%c%c \n",
            current->m_capSrcFormat.fmt.pix.pixelformat & 0xFF, (current->m_capSrcFormat.fmt.pix.pixelformat >> 8) & 0xFF,
            (current->m_capSrcFormat.fmt.pix.pixelformat >> 16) & 0xFF, (current->m_capSrcFormat.fmt.pix.pixelformat >> 24) & 0xFF);
//    fprintf(stderr, " to  %c%c%c%c \n",
//            current->m_capDestFormat.fmt.pix.pixelformat & 0xFF, (current->m_capDestFormat.fmt.pix.pixelformat >> 8) & 0xFF,
//            (current->m_capDestFormat.fmt.pix.pixelformat >> 16) & 0xFF, (current->m_capDestFormat.fmt.pix.pixelformat >> 24) & 0xFF);

    return true;
}

bool close_device(video4LinuxFreeFrameGLData *current)
{
    if (-1 == close(current->fd))
        return false;

    current->fd = -1;

    fprintf(stderr, "v4l2 - Closed device %s\n", current->dev_name);

    return true;
}

bool open_device(video4LinuxFreeFrameGLData *current)
{
    struct stat st;

    if (-1 == stat(current->dev_name, &st)) {
        fprintf(stderr, "v4l2 - Cannot identify '%s': %d, %s\n",  current->dev_name, errno, strerror(errno));
        return false;
    }

    if (!S_ISCHR(st.st_mode)) {
        fprintf(stderr, "v4l2 - %s is no device\n", current->dev_name);
        return false;
    }

    current->fd = open(current->dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

    if (-1 == current->fd) {
        fprintf(stderr, "v4l2 - Cannot open '%s': %d, %s\n", current->dev_name, errno, strerror(errno));
        return false;
    }
    return true;
}


void *update_thread(void *c)
{
    video4LinuxFreeFrameGLData *current = (video4LinuxFreeFrameGLData *) c;

    for(;;) {

        update(current);

        if (current->stop)
            break;

    }

    stop_capturing(current);
    uninit_device(current);
    close_device(current);

    return 0;
}

GLuint displayList = 0;

////////////////////////////////////////////////////////////////////////////////////////////////////
//  Plugin information
////////////////////////////////////////////////////////////////////////////////////////////////////

static CFFGLPluginInfo PluginInfo (
        video4LinuxFreeFrameGL::CreateInstance,	// Create method
        "FFGLV4L",								// Plugin unique ID
        "Video4Linux",                          // Plugin name
        1,                                      // API major version number
        600,                                    // API minor version number
        1,										// Plugin major version number
        000,									// Plugin minor version number
        FF_SOURCE,                              // Plugin type
        "Display Video4linux input (e.g.webcams)",            // Plugin description
        "by Bruno Herbelin"                     // About
        );


////////////////////////////////////////////////////////////////////////////////////////////////////
//  Constructor and destructor
////////////////////////////////////////////////////////////////////////////////////////////////////

video4LinuxFreeFrameGL::video4LinuxFreeFrameGL()
    : CFreeFrameGLPlugin()
{
    data = new video4LinuxFreeFrameGLData;

    // Input properties
    SetMinInputs(0);
    SetMaxInputs(0);

    // Parameters
    SetParamInfo(FFPARAM_DEVICE, "Device", FF_TYPE_TEXT, "auto");
}

video4LinuxFreeFrameGL::~video4LinuxFreeFrameGL()
{
    delete data;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//  Methods
////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef FF_FAIL
// FFGL 1.5
DWORD   video4LinuxFreeFrameGL::InitGL(const FFGLViewportStruct *vp)
#else
// FFGL 1.6
FFResult video4LinuxFreeFrameGL::InitGL(const FFGLViewportStruct *vp)
#endif
{

    glEnable(GL_TEXTURE);
    glActiveTexture(GL_TEXTURE0);

    glGenTextures(1, &(data->errorTextureIndex));
    glBindTexture(GL_TEXTURE_2D, data->errorTextureIndex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, error_image.width, error_image.height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE,(GLvoid*) error_image.pixel_data);


    if (displayList == 0) {
        displayList = glGenLists(1);
        glNewList(displayList, GL_COMPILE);
            glColor4f(1.f, 1.f, 1.f, 1.f);
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();
            glBegin(GL_QUADS);
            //lower left
            glTexCoord2d(0.0, 0.0);
            glVertex2f(-1,-1);
            //upper left
            glTexCoord2d(0.0, 1.0);
            glVertex2f(-1,1);
            //upper right
            glTexCoord2d(1.0, 1.0);
            glVertex2f(1,1);
            //lower right
            glTexCoord2d(1.0, 0.0);
            glVertex2f(1,-1);
            glEnd();
        glEndList();
    }

    return FF_SUCCESS;
}


#ifdef FF_FAIL
// FFGL 1.5
DWORD   video4LinuxFreeFrameGL::DeInitGL()
#else
// FFGL 1.6
FFResult video4LinuxFreeFrameGL::DeInitGL()
#endif
{

    if (! data->stop) {
        data->stop = true;
        pthread_join( data->thread, NULL );

        stop_capturing(data);
        uninit_device(data);
        close_device(data);
    }

    return FF_SUCCESS;
}

#ifdef FF_FAIL
// FFGL 1.5
DWORD   video4LinuxFreeFrameGL::SetTime(double time)
#else
// FFGL 1.6
FFResult video4LinuxFreeFrameGL::SetTime(double time)
#endif
{
    m_curTime = time;
    return FF_SUCCESS;
}

#ifdef FF_FAIL
// FFGL 1.5
DWORD	video4LinuxFreeFrameGL::ProcessOpenGL(ProcessOpenGLStruct* pGL)
#else
// FFGL 1.6
FFResult video4LinuxFreeFrameGL::ProcessOpenGL(ProcessOpenGLStruct *pGL)
#endif
{
    if (!pGL)
        return FF_FAIL;

    glClear(GL_COLOR_BUFFER_BIT );

    glEnable(GL_TEXTURE_2D);

    if (!data->stop) {

        glBindTexture(GL_TEXTURE_2D, data->textureIndex);

        pthread_mutex_lock( &(data->mutex) );
        // get a new frame

        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, data->width, data->height, GL_RGB, GL_UNSIGNED_BYTE, data->_glbuffer[data->read_buffer]);
//        fprintf(stderr, "%d %d image", data->width, data->height);

        pthread_mutex_unlock( &(data->mutex) );

    } else {

        glBindTexture(GL_TEXTURE_2D, data->errorTextureIndex);

    }

    glCallList(displayList);

    //unbind the texture
    glBindTexture(GL_TEXTURE_2D, 0);

    //disable texturemapping
    glDisable(GL_TEXTURE_2D);

    return FF_SUCCESS;
}

#ifdef FF_FAIL
// FFGL 1.5
DWORD	video4LinuxFreeFrameGL::SetTextParameter(unsigned int index, const char *value)
#else
// FFGL 1.6
FFResult video4LinuxFreeFrameGL::SetTextParameter(unsigned int index, const char *value)
#endif
{
    if (index == FFPARAM_DEVICE) {

        if ( strcmp(value, data->dev_name) == 0 ) {

            fprintf(stderr,"v4l2 - Already using device %s\n", data->dev_name);
            return FF_SUCCESS;
        }

        // stop if already running
        if (! data->stop) {
            data->stop = true;
            pthread_join( data->thread, NULL );

            stop_capturing(data);
            uninit_device(data);
            close_device(data);
        }


        if ( strcmp(value, "auto") == 0 ) {
            bool ok = false;
            int device_count = 0;

            for (;;) {
                if (device_count>9)
                    break;

                sprintf(data->dev_name, "/dev/video%d", device_count);
                if (open_device(data) ) {
                    if (init_device(data)) {
                        ok = true;
                        break;
                    }
                    close_device(data);
                }
                device_count++;
            }

            if ( !ok ){
                fprintf(stderr,"v4l2 - No available device found\n");
                return FF_SUCCESS;
            }
        }
        else {
            sprintf(data->dev_name, "%s", value);
            if (!open_device(data)) {
                fprintf(stderr,"v4l2 - Failed to open %s\n", value);
                return FF_SUCCESS;
            }
            if (!init_device(data)) {
                fprintf(stderr,"v4l2 - Failed to initialize %s\n", value);
                close_device(data);
                return FF_SUCCESS;
            }
        }


        fprintf(stderr,"v4l2 - Using device %s\n", data->dev_name);


        if ( start_capturing(data) ) {

            if (data->textureIndex != 0)
                glDeleteTextures(1, &(data->textureIndex));

            glEnable(GL_TEXTURE_2D);
            glGenTextures(1, &(data->textureIndex));
            glBindTexture(GL_TEXTURE_2D, data->textureIndex);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

            data->read_buffer = 0;
            data->draw_buffer = 0;
            update( data );

//            fprintf(stderr, "init texture %d x %d \n", data->width, data->height);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, data->width, data->height, 0, GL_RGB, GL_UNSIGNED_BYTE, data->_glbuffer[data->read_buffer]);

            // start capturing thread
            data->stop = false;
            data->draw_buffer = 1;
            int rc = pthread_create( &(data->thread), NULL, &update_thread, (void *) data);
            if( rc != 0 )
                fprintf(stderr,"v4l2 - Thread creation failed: %d\n", rc);

        }
        else {
            printf("v4l2 - Failed to open %s\n", value);
            data->stop = true;
        }

        return FF_SUCCESS;

    }

    return FF_FAIL;
}


char* video4LinuxFreeFrameGL::GetTextParameter(unsigned int index)
{
    if (index == FFPARAM_DEVICE)
        return data->dev_name;

    return (char *)FF_FAIL;
}


