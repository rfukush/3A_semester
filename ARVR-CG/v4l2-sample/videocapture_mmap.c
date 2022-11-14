/*
 *  V4L2 video capture example
 *
 *  This program can be used and distributed without restrictions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <getopt.h>		/* getopt_long() */

#include <fcntl.h>		/* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <asm/types.h>		/* for videodev2.h */

#include <linux/videodev2.h>

#define CLEAR(x) memset (&(x), 0, sizeof (x))

#include "videocapture_mmap.h"

char *dev_name = NULL;
int fd = -1, width = 640, height = 480;
buffer *buffers = NULL;
unsigned int n_buffers = 0;

///
///
///

void read_frame(void)
{
    struct v4l2_buffer buf;

    CLEAR(buf);

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_DQBUF, &buf) == -1) {
	perror("VIDIOC_DQBUF");
	exit(EXIT_FAILURE);
    }
    assert(buf.index < n_buffers);

    process_image(buffers[buf.index].start);

    if (ioctl(fd, VIDIOC_QBUF, &buf) == -1) {
	perror("VIDIOC_QBUF");
	exit(EXIT_FAILURE);
    }
}

///
///
///

void start_capturing(void)
{
    unsigned int i;
    enum v4l2_buf_type type;

    for (i = 0; i < n_buffers; ++i) {
	struct v4l2_buffer buf;

	CLEAR(buf);

	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	buf.index = i;

	if (ioctl(fd, VIDIOC_QBUF, &buf) == -1) {
	    perror("VIDIOC_QBUF");
	    exit(EXIT_FAILURE);
	}
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (ioctl(fd, VIDIOC_STREAMON, &type) == -1) {
	perror("VIDIOC_STREAMON");
	exit(EXIT_FAILURE);
    }
}


void stop_capturing(void)
{
    enum v4l2_buf_type type;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (ioctl(fd, VIDIOC_STREAMOFF, &type) == -1) {
	perror("VIDIOC_STREAMOFF");
	exit(EXIT_FAILURE);
    }

}


///
///
///

void init_mmap(void)
{
    struct v4l2_requestbuffers req;

    CLEAR(req);

    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_REQBUFS, &req) == -1) {
	perror("VIDIOC_REQBUFS");
	exit(EXIT_FAILURE);
    }

    if (req.count < 2) {
	fprintf(stderr, "Insufficient buffer memory on %s\n", dev_name);
	exit(EXIT_FAILURE);
    }

    buffers = calloc(req.count, sizeof(*buffers));

    if (!buffers) {
	fprintf(stderr, "Out of memory\n");
	exit(EXIT_FAILURE);
    }

    for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
	struct v4l2_buffer buf;

	CLEAR(buf);

	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	buf.index = n_buffers;

	if (ioctl(fd, VIDIOC_QUERYBUF, &buf) == -1) {
	    perror("VIDIOC_QUERYBUF");
	    exit(EXIT_FAILURE);
	}

	buffers[n_buffers].length = buf.length;
	buffers[n_buffers].start = mmap(NULL /* start anywhere */ ,
					buf.length, PROT_READ | PROT_WRITE
					/* required */ ,
					MAP_SHARED /* recommended */ ,
					fd, buf.m.offset);

	if (buffers[n_buffers].start == MAP_FAILED) {
	    perror("mmap");
	    exit(EXIT_FAILURE);
	}
    }
}

void uninit_mmap(void)
{
    unsigned int i;

    for (i = 0; i < n_buffers; ++i) {
	if (munmap(buffers[i].start, buffers[i].length) == -1) {
	    perror("munmap");
	    exit(EXIT_FAILURE);
	}
    }
}


///
///
///

void init_device(void)
{
    struct v4l2_capability cap;
    struct v4l2_format fmt;

    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == -1) {
	if (EINVAL == errno) {
	    fprintf(stderr, "%s is no V4L2 device\n", dev_name);
	}
	perror("VIDIOC_QUERYCAP");
	exit(EXIT_FAILURE);
    }

    fprintf(stderr, "video capabilities\n");
    fprintf(stderr, "cap.driver        =  %s\n", cap.driver);
    fprintf(stderr, "cap.card          =  %s\n", cap.card);
    fprintf(stderr, "cap.buf_info      =  %s\n", cap.bus_info);
    fprintf(stderr, "cap.version       =  %d\n", cap.version);
    fprintf(stderr, "cap.capabilities  =  0x%08x ", cap.capabilities);
    if (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)
	fprintf(stderr, " VIDEO_CAPTURE");
    if (cap.capabilities & V4L2_CAP_VIDEO_OUTPUT)
	fprintf(stderr, " VIDEO_OUTPUT");
    if (cap.capabilities & V4L2_CAP_VIDEO_OVERLAY)
	fprintf(stderr, " VIDEO_OVERLAY");
    if (cap.capabilities & V4L2_CAP_VBI_CAPTURE)
	fprintf(stderr, " VBI_CAPTURE");
    if (cap.capabilities & V4L2_CAP_VBI_OUTPUT)
	fprintf(stderr, " VBI_OUTPUT");
    if (cap.capabilities & V4L2_CAP_SLICED_VBI_CAPTURE)
	fprintf(stderr, " SLICED_VBI_CAPTURE");
    if (cap.capabilities & V4L2_CAP_SLICED_VBI_OUTPUT)
	fprintf(stderr, " VBI_SLICED_OUTPUT");
    if (cap.capabilities & V4L2_CAP_RDS_CAPTURE)
	fprintf(stderr, " RDS_CAPTURE");
    if (cap.capabilities & V4L2_CAP_VIDEO_OUTPUT_OVERLAY)
	fprintf(stderr, " VIDEO_OUTPUT_OVERLAY");
    if (cap.capabilities & V4L2_CAP_TUNER)
	fprintf(stderr, " TUNER");
    if (cap.capabilities & V4L2_CAP_AUDIO)
	fprintf(stderr, " AUDIO");
    if (cap.capabilities & V4L2_CAP_RADIO)
	fprintf(stderr, " RADIO");
    if (cap.capabilities & V4L2_CAP_READWRITE)
	fprintf(stderr, " READWRITE");
    if (cap.capabilities & V4L2_CAP_ASYNCIO)
	fprintf(stderr, " ASYNCIO");
    if (cap.capabilities & V4L2_CAP_STREAMING)
	fprintf(stderr, " STREAMING");
    fprintf(stderr, "\n");

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
	fprintf(stderr, "%s is no video capture device\n", dev_name);
	exit(EXIT_FAILURE);
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
	fprintf(stderr, "%s does not support streaming i/o\n", dev_name);
	exit(EXIT_FAILURE);
    }

    CLEAR(fmt);

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = width;
    fmt.fmt.pix.height = height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

    if (ioctl(fd, VIDIOC_S_FMT, &fmt) == -1) {
	perror("VIDIOC_S_FMT");
	exit(EXIT_FAILURE);
    }

    init_mmap();
}


void uninit_device(void)
{
    uninit_mmap();
    free(buffers);
}
