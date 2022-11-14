/*
*   videocapture.c
*		---- OpenGL program example for capturing by using OpenCV
*
*       	December 10, 2009 	programmed by TANIKAWA Tomohiro
*                                       changed by NISHIMURA Kunihiro
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
/* OpenGL */
#include <GL/glut.h>
/* OpenCV */
#include <cv.h>
#include <highgui.h>

#define SIZE_X 	640
#define SIZE_Y 	480

int 		win_width =SIZE_X, win_height =SIZE_Y;
int 		cap_width =SIZE_X, cap_height =SIZE_Y;
IplImage 	*frame;         /* OpenCV画像構造体 */
CvCapture* 	capture = 0;	/* Capture構造体 */
int 		dev_index = 0;	/* カメラのインデックス */

void my_init(int argc, char **argv);
int my_exit(int e);

/* キー入力への対応 */
void keyboard(unsigned char c, int x, int y)
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

/* アイドル状態での処理 */
void idle(void)
{
    if (capture) {
	frame = cvQueryFrame(capture); /* カメラ画像の取得 */
	if (frame->origin==0) {
	    cvFlip(frame, frame, 0 ); /* OpenCVとOpenGLでは座標が異なるため変換 */
	}
	cvCvtColor(frame, frame, CV_BGR2RGB); /* 色データの入れ替え */
    }
    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );

    glutPostRedisplay();
}

/* 画像の書き出し */
void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); /* テクスチャデータの格納方式 */
/*    glPixelZoom(1.0, 1.0);
      glRasterPos2i(-cap_width/2, -cap_height/2); */
    glPixelZoom((GLdouble)(win_width-2)/(GLdouble)cap_width,
		(GLdouble)(win_height-2)/(GLdouble)cap_height);
    glRasterPos2i(-win_width/2+1, -win_height/2+1); /* 原点指定 */
    glDrawPixels(cap_width, cap_height, GL_RGB, GL_UNSIGNED_BYTE,
		 frame->imageData);

    glFlush();
    glutSwapBuffers();
}

/* ウィンドウサイズ変更時の処理 */
void reshape(int w, int h)
{
/*    win_width = SIZE_X; win_height = SIZE_Y;*/
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

/* OpenCV関連の初期化 */
void my_init(int argc, char **argv)
{
    /* カメラからのキャプチャ準備 */
    capture = cvCreateCameraCapture(dev_index);
    if (capture) {
	/* カメラ画像の取得 */
	frame = cvQueryFrame(capture);
	/* カメラ画面サイズの取得 */
	cap_width = cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH);
	cap_height = cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT);
    } else {
	fprintf(stderr, "Found No Camera\n");
	exit(-1);
    }
    /* 画像データの上下反転 */
    if (frame->origin==0) {
	cvFlip(frame, frame, 0);
    }
    /* BGR配列からRGB配列への変換 */
    cvCvtColor(frame, frame, CV_BGR2RGB);

    win_width = cap_width;
    win_height = cap_height;
}

int my_exit(int e)
{
    /* キャプチャの終了処理 */
    if (capture) cvReleaseCapture(&capture);

    exit(e);
}

/* メイン関数 */
int main(int argc, char **argv)
{
    char	title[128];

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);

    my_init(argc, argv);

    glutInitWindowSize(win_width, win_height);
    sprintf(title, "OpenCV Video Texture - (%dx%d)", cap_width, cap_height);
    glutCreateWindow(title);
/*    glutFullScreen();*/

    glClearColor(0.0, 0.0, 0.0, 0.0);

/* OpenGLコールバック関数の指定 */
	glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);
    glutKeyboardFunc(keyboard);

    glutMainLoop();
    return 0;
}
