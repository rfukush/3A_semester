/*
*   videocapture_gl.c
*		---- OpenGL program example for capturing V4L2 Video  
*
*       	October 24, 2009 	customized by TANIKAWA Tomohiro
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>		/* getopt_long() */
#include <fcntl.h>		/* low-level i/o */
#include <errno.h>
#include <GL/glut.h>

#include "videocapture.h"

#define SIZEX 640
#define SIZEY 480

int win_width =SIZEX, win_height =SIZEY;
GLubyte *image =NULL;
GLsizei image_width, image_height, image_depth;
GLenum	image_format;

void my_init(int argc, char **argv);
int my_exit(int e);

//
// open and close devices
//

void open_device(void)
{
    fprintf(stderr, "Opening device '%s'\n", dev_name);
    fd = open(dev_name, O_RDWR, 0);

    if (fd == -1) {
	fprintf(stderr, "Cannot open '%s': %d, %s\n",
		dev_name, errno, strerror(errno));
	exit(EXIT_FAILURE);
    }
}

void close_device(void)
{
    if (close(fd) == -1) {
	perror("close");
	exit(EXIT_FAILURE);
    }
    fd = -1;
}

void convert_rgb(void){
    int i;

    for (i = 0; i < width * height; i += 2) {
	unsigned char y;
	signed char u, v;
	double r, g, b;
	y = ((char *) buffers[0].start)[i * 2];
	u = ((char *) buffers[0].start)[i * 2 + 1] - 128;
	v = ((char *) buffers[0].start)[i * 2 + 3] - 128;

	r = y + 1.40200 * v;
	g = y - 0.71414 * v - 0.34414 * u;
	b = y + 1.77200 * u;
	r = (r-16) * 255 / 219; if(r<0) r=0; if(r>255) r=255;
	g = (g-16) * 255 / 219; if(g<0) g=0; if(g>255) g=255;
	b = (b-16) * 255 / 219; if(b<0) b=0; if(b>255) b=255;
	image[i * 3 + 0] = (GLubyte)r;
	image[i * 3 + 1] = (GLubyte)g;
	image[i * 3 + 2] = (GLubyte)b;

	y = ((char *) buffers[0].start)[(i + 1) * 2];

	r = y + 1.40200 * v; 
	g = y - 0.71414 * v - 0.34414 * u; 
	b = y + 1.77200 * u; 
	r = (r-16) * 255 / 219; if(r<0) r=0; if(r>255) r=255;
	g = (g-16) * 255 / 219; if(g<0) g=0; if(g>255) g=255;
	b = (b-16) * 255 / 219; if(b<0) b=0; if(b>255) b=255;
	image[(i + 1) * 3 + 0] = (GLubyte)r;
	image[(i + 1) * 3 + 1] = (GLubyte)g;
	image[(i + 1) * 3 + 2] = (GLubyte)b;
    }
}

void process_image(const void *p)
{
    fputc('.', stderr);
    fflush(stderr);
}

static void usage(FILE * fp, int argc, char **argv)
{
    fprintf(fp,
	    "Usage: %s [options]\n\n"
	    "Options:\n"
	    "-d | --device name   Video device name [/dev/video]\n"
	    "-e | --height name   Video window heigtht [640]\n"
	    "-w | --width name    Video window width [480]\n"
	    "-h | --help          Print this message\n" "", argv[0]);
}

static const char short_options[] = "d:hmru";

static const struct option long_options[] = {
    {"device", required_argument, NULL, 'd'},
    {"width", required_argument, NULL, 'w'},
    {"height", required_argument, NULL, 'e'},
    {"help", no_argument, NULL, 'h'},
    {0, 0, 0, 0}
};

void
keyboard(unsigned char c, int x, int y)
{
    switch(c){
      case 32:
	glutPostRedisplay();
	break;
      case 27:
	my_exit(1);
	break;
    }
}

void
idle(void)
{
    read_frame();
    convert_rgb();
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glutPostRedisplay();
}

void
display(void)
{
    glClear(GL_COLOR_BUFFER_BIT);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelZoom(1.0, -1.0);
    glRasterPos2i(-image_width/2, image_height/2);
/*    glPixelZoom((GLdouble)(win_width-2)/(GLdouble)image_width, 
		-(GLdouble)(win_height-2)/(GLdouble)image_height);
    glRasterPos2i(-win_width/2+1, win_height/2-1); */
    glDrawPixels(image_width, image_height, image_format, GL_UNSIGNED_BYTE, image);

    glFlush();
    glutSwapBuffers();
}

void
reshape(int w, int h)
{
/*    win_width = SIZEX; win_height = SIZEY;*/
    win_width =  w; win_height =  h;
    glViewport(0, 0, win_width, win_height);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-(GLdouble)win_width/2.0, (GLdouble)win_width/2.0, 
	       -(GLdouble)win_height/2.0, (GLdouble)win_height/2.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glutPostRedisplay();
}

void
my_init(int argc, char **argv)
{
    dev_name = "/dev/video0";
    unsigned int count;

    for (;;) {
	int index;
	int c;

	c = getopt_long(argc, argv, short_options, long_options, &index);

	if (c == -1)
	    break;
	switch (c) {
	case 0:		/* getopt_long() flag */
	    break;
	case 'w':
	    width = atoi(optarg);
	    break;
	case 'e':
	    height = atoi(optarg);
	    break;
	case 'd':
	    dev_name = optarg;
	    break;
	case 'h':
	    usage(stdout, argc, argv);
	    exit(EXIT_SUCCESS);
	default:
	    usage(stderr, argc, argv);
	    exit(EXIT_FAILURE);
	}
    }

    open_device();
    init_device();
    start_capturing();

    count = 10;
    while (count-- > 0) {
	read_frame();
    }
    image = (GLubyte *)malloc(width * height * 3 * sizeof(GLubyte));
    convert_rgb();

    image_width = width;
    image_height = height;
    image_depth = 3;
    if(image_depth >= 4) image_format = GL_RGBA;
    else if(image_depth == 3) image_format = GL_RGB;
    else image_format = GL_LUMINANCE;

    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
 
    win_width = width+2;
    win_height = height+2;
}

int
my_exit(int e)
{
    stop_capturing();
    uninit_device();
    close_device();

    exit(e);
}

int
main(int argc, char **argv)
{
    char	title[128];

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);

    my_init(argc, argv);

    glutInitWindowSize(win_width, win_height);
    sprintf(title, "Video Capture - %s(%dx%d)", dev_name, width, height);
    glutCreateWindow(title);
/*    glutFullScreen();*/

    glClearColor(0.0, 0.0, 0.0, 0.0);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);
    glutKeyboardFunc(keyboard);

    glutMainLoop();
    return 0;
}
