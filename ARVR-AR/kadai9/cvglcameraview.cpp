/* 2008.12.16 知能機械情報学演習 Media Interface */
/* Updated 2010                               */
/* Masamichi Shimosaka  simosaka@ics.t.u-tokyo.ac.jp */
/* Kunihiro Nishimura   kuni@cyber.t.u-tokyo.ac.jp */
/* Modified by Toshiyuki Numata in 2019 numata@cyber.t.u-tokyo.ac.jp */

#include <opencv2/opencv.hpp>

#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <pthread.h>

#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#define KEY_ESC 27

#define CHESS_ROW_NUM 10
#define CHESS_COL_NUM 7
#define CHESS_ROW_DX 20
#define CHESS_COL_DY 20

/* 2^n GLでは、2の累乗のみテクスチャを許可 */
#define IMAGE_W  512
#define IMAGE_H  512

typedef struct{
 double dx;
 double dy;
 CvSize patternSize;
} ChessBoard;

int graphicsWinWidth = 640;
int graphicsWinHeight = 480;

double distance = 3.0;
double theta = 0.0;
double twist = 0.0;
double elevation = 0.0;
double azimuth = 0.0;

double xAngle = 0;
double yAngle = 0;

int xStart = 0;
int yStart = 0;

int mouseFlag = GL_FALSE;

int isCVinitialized = 0;

double transGL[3];

double rotGL[4];

/* resized captured image 2^n */
IplImage* resized_frame;
IplImage* frame_copy;

/* キャプチャするイメージの縦横比 */
double aspect_rate;

GLuint texture[1];

const char* captureWinName = "camera view";

void store2DCoordinates( CvMat* cornerPoints, CvPoint2D32f* points,
			 ChessBoard chess, int imageID );

void store3DCoordinates( CvMat* objectPoints, ChessBoard chess, int imageID );


void loadIntrinsicParams(const char* fileName, CvMat* intrinsic,
			 CvMat* distortion);

void getCameraPosition(CvMat* rotation, CvMat* translation, double* cameraPosition);

void getCameraOriVec(CvMat* rotation, double* cameraOriVec);

void convertCv2Gl(double* cvPosInMM, double* glPosInM);


void calibrate ( CvMat* cornerPoints, CvMat* objectPoints,
		 CvMat* intrinsic, CvMat* distortion,
		 CvMat* rotation, CvMat* translation );

void updateTexture(IplImage *image);

void initTexture();

/* 3軸の描画 */
void drawXYZAxes()
{
    glLineWidth(1);
    glBegin(GL_LINES);
    glColor3d(0, 1, 0);//x軸・・・緑
    glVertex2d(0, 0);
    glVertex2d(0.5, 0);

    glColor3d(1, 0, 0);//y軸・・・赤
    glVertex2d(0, 0);
    glVertex2d(0, 0.5);

    glColor3d(0, 0, 1);//z軸・・・青
    glVertex3d(0, 0, 0);
    glVertex3d(0, 0, 0.5);
    glEnd();
}

/* チェッカーボードの描画  */
void drawBoard(double unit, int xsize, int ysize)
{
    int i,j;
    xsize++;
    ysize++;

    glColor3d(1, 1, 1);
    glLineWidth(0.5);
    glBegin(GL_POLYGON);
    glVertex3d(-unit, 0, -unit);
    glVertex3d(xsize * unit + unit, 0, -unit);
    glVertex3d(xsize * unit + unit, 0, ysize * unit + unit);
    glVertex3d(-unit, 0, ysize * unit + unit);
    glEnd();

    glColor3d(0, 0, 0);
    glLineWidth(1);
    glBegin(GL_LINES);
    glVertex3d(-unit, 0, -unit);
    glVertex3d(xsize * unit + unit, 0, -unit);
    glVertex3d(xsize * unit + unit, 0, ysize * unit + unit);
    glVertex3d(-unit, 0, ysize * unit + unit);
    glEnd();

    glLineWidth(0.5);
    for( i = 0; i < xsize; i++){
	for( j = 0; j < ysize; j++){
	    double offsetx = unit * i;
	    double offsety = unit * j;
	    double xend = unit * (i + 1);
	    double yend = unit * (j + 1);

	    if((i + j) % 2){
		glColor3d(1, 1, 1);
	    } else {
		glColor3d(0, 0, 0);
	    }
	    glBegin(GL_POLYGON);
	    glVertex3d(offsetx, 0, offsety);
	    glVertex3d(xend, 0, offsety);
	    glVertex3d(xend, 0, yend);
	    glVertex3d(offsetx, 0, yend);
	    glEnd();
	}
    }
}

/* カメラの描画 */
void drawCamera(double t[3], double r[4])
{
    glPushMatrix();
    /* カメラ位置への移動 */
    /* 課題 */
    /* 0.0を適切に移動するように変更する */
    glTranslated(t[0], t[1], t[2]);
    /* カメラ位置の回転（一部） */
    glRotated(r[1]*90.0, 0.0,1.0,0.0);
    glRotated(r[2]*180.0, 0.0,0.0,1.0);

    glColor3d(1, 0, 1);
    glLineWidth(2);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBegin(GL_POLYGON);
    glVertex3d(0, 0, 0);
    glVertex3d(0.025, 0.025, 0.07);
    glVertex3d(0.025, -0.025, 0.07);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex3d(0, 0, 0);
    glVertex3d(0.025, 0.025, 0.07);
    glVertex3d(-0.025, 0.025, 0.07);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex3d(0, 0, 0);
    glVertex3d(-0.025, 0.025, 0.07);
    glVertex3d(-0.025, -0.025, 0.07);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex3d(0, 0, 0);
    glVertex3d(-0.025, -0.025, 0.07);
    glVertex3d(0.025, -0.025, 0.07);
    glEnd();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glPopMatrix();
}

/* カメラからキャプチャされたテクスチャの描画部分 */
void drawCameraTexture(double t[3], double r[4]){
    float p_size = 0.15;
    glPushMatrix();
    /* カメラ位置への移動 */
    /* 課題 */
    /* 適切なパラメータへ変更する */
    glTranslated(t[0], t[1], t[2]);
    /* カメラ位置の回転（一部） */
    /* 課題 */
    /* 適切なパラメータへ変更する */
    glRotated(r[1]*90.0, 0.0,1.0,0.0);
    /* 課題 */
    /* 適切なパラメータへ変更する */
    glRotated(r[2]*180.0, 0.0,0.0,1.0);

    //テクスチャを貼り付ける
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    {
	/* 課題 */
	/* テクスチャの張り方（軸）の違い、アスペクト比なども考えること */
	glTexCoord2d(0.0, 0.0);
	glVertex3d( 1.0*p_size, 0.0, 1.0*p_size);
	glTexCoord2d(0.0, 1.0);
	glVertex3d( 1.0*p_size, 0.0, -1.0*p_size);
	glTexCoord2d(1.0, 1.0);
	glVertex3d(-1.0*p_size, 0.0, -1.0*p_size);
	glTexCoord2d(1.0, 0.0);
	glVertex3d(-1.0*p_size, 0.0, 1.0*p_size);
        /* glVertex3dのなかの位置を考える */


    }
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}

/* 描画関数 */
void display(void)
{
  float aspect = 1.0f * (float)graphicsWinWidth / (float)graphicsWinHeight;
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, aspect, 0.01f, 10.f);
    gluLookAt(0.9, 0.8, 0.8,  0, 0, 0,  0, 1, 0);

    glRotated(xAngle,1,0,0);
    glRotated(yAngle,0,1,0);
    glRotated(180, 0,1,0);
    /* 3軸の描画 */
    drawXYZAxes();
    /* チェッカーボードの描画 */
    drawBoard(CHESS_ROW_DX/1000.0f, CHESS_ROW_NUM, CHESS_COL_NUM);

    /* キャプチャしたテクスチャの描画 */
    drawCameraTexture(transGL, rotGL);
    /* カメラの描画 */
    drawCamera(transGL, rotGL);
    glutSwapBuffers();
}


/* アイドル時 */
void idle(void)
{
    /* テクスチャのアップデート */
    updateTexture(frame_copy);
    glutPostRedisplay();
}

/* キーボードの設定 */
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

/* 初期化 */
void initialize()
{

  float aspect = 1.0f * (float)graphicsWinWidth / (float)graphicsWinHeight;

    glutInitWindowPosition(0, 0);
    glutInitWindowSize( graphicsWinWidth, graphicsWinHeight);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutCreateWindow("graphics window");

    glClearColor (0.0, 0.0, 0.0, 1.0);

    glutKeyboardFunc(keyboardFunction);
    glutMouseFunc(mouseFunction);
    glutMotionFunc(motionFunction);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, aspect, 1.0, 30.0);
    glMatrixMode(GL_MODELVIEW);

    glutDisplayFunc(display);
    glutIdleFunc(idle);

    /* テクスチャの初期化 */
    initTexture();
}

/* OpenCVでカメラからキャプチャし、チェッカーボードを認識するスレッド */
void* captureThread(void* args)
{
    CvCapture* capture = NULL;

    IplImage* frame;

    IplImage* gray;
    ChessBoard chess;
    int pointsPerScene;
    CvPoint2D32f* points;
    int pointsNum[1];
    CvMat* cornerPoints;
    CvMat* objectPoints;
    CvMat pointsNumMat;

    CvMat* intrinsic = cvCreateMat( 3, 3, CV_64F );
    CvMat* distortion = cvCreateMat( 4, 1, CV_64F );
    CvMat* rotation = cvCreateMat( 3, 3, CV_64F );
    CvMat* translation = cvCreateMat( 3, 1, CV_64F );

    loadIntrinsicParams("intrinsic_param.txt", intrinsic, distortion );

    /* カメラからキャプチャする部分 */
    capture = cvCreateCameraCapture(0);

    if(capture == NULL){
	fprintf(stderr, "ERROR: Could not open Camera Device\n");
	exit(1);
    }

    /* frame = queryFrameOverIP(capture); */
    frame = cvQueryFrame(capture);

    if(frame == NULL){
	fprintf(stderr, "ERROR: Could not query frame\n");
	exit(1);
    }

    frame_copy = cvCreateImage(cvGetSize(frame),
			       frame->depth, 3);

    gray = cvCreateImage(cvGetSize(frame_copy), frame_copy->depth, 1);

    /* calculate rate */
    aspect_rate = (double)frame_copy->width / (double)frame_copy->height;
    resized_frame = cvCreateImage(cvSize(IMAGE_W, IMAGE_H),8,3);

//    cvNamedWindow(captureWinName, CV_WINDOW_AUTOSIZE);
    cvMoveWindow(captureWinName, graphicsWinWidth + 10, 0);

    chess.dx = CHESS_ROW_DX;
    chess.dy = CHESS_COL_DY;
    chess.patternSize.width = CHESS_ROW_NUM;
    chess.patternSize.height = CHESS_COL_NUM;

    pointsPerScene = chess.patternSize.width * chess.patternSize.height;
    cornerPoints = cvCreateMat(pointsPerScene, 2, CV_32F);
    objectPoints = cvCreateMat(pointsPerScene, 3, CV_32F);

    pointsNum[0] = pointsPerScene;
    pointsNumMat = cvMat(1, 1, CV_32S, pointsNum);

    points = (CvPoint2D32f*)malloc( sizeof(CvPoint2D32f) * pointsPerScene );

    //flag of initialization
    isCVinitialized = 1;

    while(1){
	int allPointsFound = 0;
	int detectedPointsNum;
        frame = cvQueryFrame(capture);
	if( !frame ) {
	    fprintf(stderr, "could not query frame\n");
	    exit(1);
	}

	cvResize(frame, frame_copy, CV_INTER_NN);
	cvCvtColor(frame_copy, gray, CV_BGR2GRAY);

	if( cvFindChessboardCorners( gray, chess.patternSize, points,
				     &detectedPointsNum,
				     CV_CALIB_CB_ADAPTIVE_THRESH ) ){
	    cvFindCornerSubPix(gray, points, detectedPointsNum,
			       cvSize(5, 5), cvSize(-1, -1),
			       cvTermCriteria(CV_TERMCRIT_ITER, 100, 0.1));
	    allPointsFound = 1;
	} else {
	    allPointsFound = 0;
	}

	cvDrawChessboardCorners( frame_copy, chess.patternSize, points,
				 detectedPointsNum, allPointsFound );

	if( allPointsFound ){
	    double cameraPosition[3];
	    double cameraOriVec[4];
	    store2DCoordinates( cornerPoints, points, chess, 0 );
	    store3DCoordinates( objectPoints, chess, 0 );
	    calibrate( cornerPoints, objectPoints,
		       intrinsic, distortion, rotation, translation );

	    getCameraPosition(rotation, translation, cameraPosition);
	    printf("cam pos relative to chess board: %.1f, %.1f, %.1f\n",
		   cameraPosition[0],
		   cameraPosition[1],
		   cameraPosition[2]);
	    convertCv2Gl(cameraPosition, transGL);
	    getCameraOriVec(rotation, rotGL);
	}

	cvShowImage( captureWinName, frame_copy);
	if(cvWaitKey(10) == KEY_ESC){
          exit(1);
        }
    }
    free(points);
    cvReleaseMat(&intrinsic);
    cvReleaseMat(&distortion);
    cvReleaseMat(&rotation);
    cvReleaseMat(&translation);

    cvReleaseMat(&cornerPoints);
    cvReleaseMat(&objectPoints);

    cvDestroyWindow(captureWinName);
    cvReleaseImage(&frame_copy);
    cvReleaseImage(&gray);
    /*releaseIpBasedCapture(capture);*/
    cvReleaseCapture(&capture);
}

int main(int argc, char** argv)
{
  int cnt = 0;
    pthread_t thread;

    glutInit(&argc, argv);
    initialize();

    /* 課題 */
    /* カメラキャプチャのスレッドを起動する */
    pthread_create(&thread, NULL, captureThread, (void*)NULL);

    //flag of initalization
    while(!isCVinitialized){
      cvWaitKey(10);
      cnt++;
      if(cnt>500){
        printf("Initialization Error\n");
        exit(-1);
      }
    }
    glutMainLoop();

    pthread_join(thread, NULL);

    return 0;
}



void store2DCoordinates( CvMat* cornerPoints, CvPoint2D32f* points,
                       ChessBoard chess, int imageID )
{
    int i = 0;
    int pointsPerScene = chess.patternSize.width * chess.patternSize.height;
    int offset = imageID * pointsPerScene;

    for ( i = 0; i < pointsPerScene; i++ ) {
	cvmSet(cornerPoints, offset + i, 0, points[i].x);
	cvmSet(cornerPoints, offset + i, 1, points[i].y);
    }
}

void store3DCoordinates( CvMat* objectPoints,ChessBoard chess, int imageID )
{
    int i;
    int pointsPerScene = chess.patternSize.width * chess.patternSize.height;

    for( i = 0; i < pointsPerScene; i++){
	double dx = chess.dx;
	double dy = chess.dy;

	int rowNum = chess.patternSize.width;
	int k = i % rowNum;
	int l = i / rowNum;
	int offset = pointsPerScene * imageID;
	cvmSet(objectPoints, offset + i, 0, k*dx );
	cvmSet(objectPoints, offset + i, 1, l*dy );
	cvmSet(objectPoints, offset + i, 2, 0.0 );
    }
}


void loadIntrinsicParams(const char* fileName, CvMat* intrinsic,
			 CvMat* distortion)
{
    FILE* file;
    int i, j;

    if((file = fopen(fileName, "r")) == NULL){
	fprintf(stderr, "file open error\n");
	return;
    }

    for( i = 0; i < 3; i++){
	for( j = 0; j < 3; j++){
	    double value = 0;
	    fscanf( file, "%lf", &value );
	    cvmSet( intrinsic, i, j, value );
	}
    }

    for( i = 0; i < 4; i++){
	cvmSet( distortion, i, 0,  0.0 );
    }

    fclose(file);
}

void getCameraPosition(CvMat* rotation, CvMat* translation,
		       double* cameraPosition)
{
    int rr;
    int cc;

    for(rr = 0; rr < 3; rr++){
	cameraPosition[rr] = 0.0;
	for(cc = 0; cc < 3; cc++){
	    cameraPosition[rr] -=
		cvmGet(rotation, cc, rr) * cvmGet(translation, cc, 0);
	}
    }

    //inverting z axis value
    cameraPosition[2] *= -1;
}

void getCameraOriVec(CvMat* rotation, double* cameraOriVec)
{
    int rr;
    for(rr=0; rr<3; rr++){
	printf("%f, ", cvmGet(rotation, 0, rr));
	cameraOriVec[rr] = cvmGet(rotation, 0, rr);
    }
}

void convertCv2Gl(double* cvPosInMM, double* glPosInM)
{
    glPosInM[0] = cvPosInMM[0] / 1000.;
    glPosInM[1] = cvPosInMM[2] / 1000.;
    glPosInM[2] = cvPosInMM[1] / 1000.;
}

void calibrate ( CvMat* cornerPoints, CvMat* objectPoints,
		 CvMat* intrinsic, CvMat* distortion,
		 CvMat* rotation, CvMat* translation )
{

    CvMat* rotVector = cvCreateMat( 3, 1, CV_64F );
    cvFindExtrinsicCameraParams2( objectPoints, cornerPoints,
				  intrinsic, distortion,
				  rotVector, translation, 0 );

    cvRodrigues2( rotVector, rotation, 0 );

    cvReleaseMat( &rotVector );
}

/* テクスチャのアップデート */
void updateTexture(IplImage *image){
    cvResize(image, resized_frame, CV_INTER_NN);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
		 resized_frame->width,
		 resized_frame->height,
		 0, GL_BGR_EXT, GL_UNSIGNED_BYTE,
		 resized_frame->imageData);
}

/* テクスチャの初期化 */
void initTexture(){
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glGenTextures(1, &texture[0]);
    glBindTexture(GL_TEXTURE_2D, texture[0]);

    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}

