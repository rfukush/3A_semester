#include <opencv2/opencv.hpp>

#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <pthread.h>

#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#define KEY_ESC 27

int graphicsWinWidth = 640;
int graphicsWinHeight = 480;

double xAngle = 0;
double yAngle = 0;

int xStart = 0;
int yStart = 0;

int mouseFlag = GL_FALSE;

const char* captureWinName = "camera view";

void drawTeapot(){
    glColor3f(1, 0, 1);
    glutWireTeapot(.10);
}


void display(void)
{
    float aspect = 1.0f * graphicsWinWidth / graphicsWinHeight;
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, aspect, 0.01f, 10.f);
    gluLookAt(0.9, 0.8, 0.8,  0, 0, 0,  0, 1, 0);
        
    glRotated(xAngle,1,0,0);
    glRotated(yAngle,0,1,0);

    drawTeapot();
    glutSwapBuffers();
}

void idle(void)
{
    glutPostRedisplay();
}

void keyboardFunction( unsigned char key, int x, int y )
{
    if(key == KEY_ESC){
	exit(0);
    }
    glutPostRedisplay();
}

void mouseFunction( int button, int state, int x, int y )
{
    if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN){
	xStart = x;
	yStart = y;
	mouseFlag = GL_TRUE;
    } else{
	mouseFlag = GL_FALSE;
    }
}

void motionFunction( int x, int y )
{
    int xdis;
    int ydis;
    double a = 0.25;

    if(mouseFlag ==GL_FALSE){
	return;
    }

    xdis = x - xStart;
    ydis = y - yStart;
    xAngle += a * ydis;
    yAngle += a * xdis;
    xStart = x;
    yStart = y;
    
    glutPostRedisplay();
}

void initialize()
{

    float aspect = 1.0f * graphicsWinWidth / graphicsWinHeight;
    
    glutInitWindowPosition(0, 0);
    glutInitWindowSize( graphicsWinWidth, graphicsWinHeight);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutCreateWindow("graphics window");

    glClearColor (0.0, 0.0, 0.0, 1.0);

    glutKeyboardFunc(keyboardFunction);
    glutMouseFunc(mouseFunction);
    glutMotionFunc(motionFunction);
    
    glutDisplayFunc(display);
    glutIdleFunc(idle);
}

void* captureThread(void* args)
{
    CvCapture* capture = NULL;
    IplImage* frame;
    IplImage* frame_copy;

    capture = cvCreateCameraCapture( 0 );

    if(capture == NULL){
	fprintf(stderr, "ERROR: Could not open Camera Device\n");
	exit(1);
    }
    
    frame = cvQueryFrame(capture); 
    
    if(frame == NULL){
	fprintf(stderr, "ERROR: Could not query frame\n");
	exit(1);
    }
    
    frame_copy = cvCloneImage(frame);
    
    cvNamedWindow(captureWinName, CV_WINDOW_AUTOSIZE);
    cvMoveWindow(captureWinName, graphicsWinWidth + 10, 0);

    while(1){
	frame = cvQueryFrame( capture ); 
	if( !frame ) {
	    fprintf(stderr, "could not query frame\n");
	    exit(1);
	}
	
	cvCopy( frame, frame_copy, NULL );
	cvShowImage( captureWinName, frame_copy);
	if(cvWaitKey(10) == KEY_ESC){
	    exit(1);
	}
    }
    cvDestroyWindow(captureWinName);
    cvReleaseImage(&frame_copy);
    cvReleaseCapture(&capture);
}

int main(int argc, char** argv)
{
    pthread_t thread;
    
    glutInit(&argc, argv);
    initialize();
    
    pthread_create(&thread, NULL, captureThread, (void*)NULL);
    glutMainLoop();
    pthread_join(thread, NULL);
    
    return 0;
}

