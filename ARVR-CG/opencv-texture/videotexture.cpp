/*
*   videotexture.c
*		---- OpenGL program example for capturing by using OpenCV
*
*       	December 11, 2009 	programmed by TANIKAWA Tomohiro
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

#define SIZE_X 		640
#define SIZE_Y 		480
#define PI 		3.1415926535897932
#define DEG 		(2*PI/360.)
#define FOV_Y 		45
#define NEAR_CLIP	10.0
#define FAR_CLIP 	10000.0
#define DISTANCE_0	579.41

int 		win_width =SIZE_X, win_height =SIZE_Y;
GLfloat 	distance = DISTANCE_0, pitch = 0.0, yaw = 0.0;
GLint		mouse_button = -1;
GLint		mouse_x = 0, mouse_y =0;

GLuint		tex_index;
GLsizei 	tex_width, tex_height;

IplImage 	*frame;			/* OpenCV画像構造体 */
CvCapture* 	capture = 0;		/* Capture構造体 */
int 		dev_index = 0;		/* カメラのインデックス */
int 		cap_width =SIZE_X, cap_height =SIZE_Y;

/* OpenGLコールバック関数 */
void keyboard(unsigned char c, int x, int y);
void display(void);
void reshape(int w, int h);
void mouse(int button, int status, int x, int y);
void motion(int x, int y);
void idle(void);
void my_init(int argc, char **argv);
void my_exit(int e);

/* テクスチャのサイズは2のn乗 */
void define_texturesize(GLsizei *w0, GLsizei *h0, GLsizei *w1, GLsizei *h1)
{
    int i = 0;
    GLsizei prev;

    while(*w0 > (prev = (int)pow(2, i))) i++; *w1 = prev;
    i = 0;
    while(*h0 > (prev = (int)pow(2, i))) i++; *h1 = prev;
}

/* キー入力への対応 */
void keyboard(unsigned char c, int x, int y)
{
    switch(c){
      case 32: /* Space */
	  glutPostRedisplay();
	break;
      case 27: /* Escape */
	  my_exit(1);
	break;
    }
}

/* マウスのボタン入力への対応 */
void mouse(int button, int state, int x, int y)
{
    mouse_button = button;
    mouse_x = x; mouse_y = y;
    if(state == GLUT_UP) mouse_button = -1;

    glutPostRedisplay();
}

/* マウスの移動への反応 */
void motion(int x, int y)
{
    switch(mouse_button){
      case GLUT_LEFT_BUTTON:
	if( x==mouse_x && y==mouse_y ) return;
	yaw -= (GLfloat)( x - mouse_x ) /10.0;
	pitch -= (GLfloat)( y - mouse_y ) /10.0;
	break;
      case GLUT_RIGHT_BUTTON:
	if( y==mouse_y ) return;
	if( y < mouse_y ) distance += (GLfloat)(mouse_y - y);
	else distance -= (GLfloat)(y - mouse_y);
	if( distance < NEAR_CLIP ) distance = NEAR_CLIP;
	if( distance > FAR_CLIP ) distance = FAR_CLIP;
	break;
    }
    mouse_x = x;
    mouse_y = y;

    glutPostRedisplay();
}

/* アイドル状態での処理 */
void idle(void)
{
    if (capture) { 
	frame = cvQueryFrame(capture); /* カメラ画像の取得 */
	if (frame->origin==0) {
	    cvFlip(frame, frame, 0);
	}
	cvCvtColor(frame, frame, CV_BGR2RGB);
    }

    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
    glBindTexture( GL_TEXTURE_2D, tex_index );
    glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, frame->width, frame->height,
                     GL_RGB, GL_UNSIGNED_BYTE, frame->imageData ); /* 画像の更新 */
    glutPostRedisplay();
}

/* 画像の書き出し */
void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();
    gluLookAt(0.0, 0.0, distance, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
    glRotatef( -pitch, 1.0, 0.0, 0.0);
    glRotatef( -yaw, 0.0, 1.0, 0.0);

/* テクスチャの環境設定 */
    glColor3f(1.0, 1.0, 1.0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture( GL_TEXTURE_2D, tex_index );

/* ポリゴンへのテクスチャ貼り付け */
    glPushMatrix();
    glBegin(GL_TRIANGLE_FAN);
    glTexCoord2f(0.0, 0.0);
    glVertex3f(-cap_width/2, -cap_height/2, 0.0);
    glTexCoord2f((float)cap_width/tex_width, 0.0);
    glVertex3f(cap_width/2, -cap_height/2, 0.0);
    glTexCoord2f((float)cap_width/tex_width, (float)cap_height/tex_height);
    glVertex3f(cap_width/2, cap_height/2, 0.0);
    glTexCoord2f(0.0, (float)cap_height/tex_height);
    glVertex3f(-cap_width/2, cap_height/2, 0.0);
    glEnd();
    glPopMatrix();

    glutSwapBuffers();
}

/* ウィンドウサイズ変更時の処理 */
void reshape(int w, int h)
{
    win_width =  w; win_height =  h;
    glViewport(0, 0, win_width, win_height);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(FOV_Y, (GLfloat)w/(GLfloat)h, NEAR_CLIP, FAR_CLIP);
    
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

/* ウィンドウのサイズ指定と最適なカメラ距離の算出 */
    win_width = cap_width;
    win_height = cap_height;
    distance = (cap_height/2.0) / tan(FOV_Y/2.0 * DEG);
}

/* OpenGLのテクスチャ関連の初期化 */
void my_initGL(void )
{
    define_texturesize( &cap_width, &cap_height, &tex_width, &tex_height );
    
    glGenTextures( 1, &tex_index );
    glBindTexture( GL_TEXTURE_2D, tex_index );

    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );

    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, tex_width, tex_height, 0,
                  GL_RGB, GL_UNSIGNED_BYTE, 0 );

    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
    glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, cap_width, cap_height,
                     GL_RGB, GL_UNSIGNED_BYTE, frame->imageData );

    glEnable( GL_TEXTURE_2D );
    glEnable( GL_DEPTH_TEST );
    glClearColor( 0.0, 0.0, 0.0, 0.0 );
    glClear( GL_COLOR_BUFFER_BIT );
}

void my_exit(int e)
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
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH );

    my_init(argc, argv);

    glutInitWindowSize(win_width, win_height);
    sprintf(title, "OpenCV Video Texture - (%dx%d)", cap_width, cap_height);
    glutCreateWindow(title);
/*    glutFullScreen();*/
    my_initGL();

    glClearColor(0.0, 0.0, 0.0, 0.0);

/* OpenGLコールバック関数の指定 */
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);

    glutMainLoop();
    return 0;
}
