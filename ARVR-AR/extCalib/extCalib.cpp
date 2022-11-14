/*
 * extCalib.c
 * written by m. shimosaka, y. sagawa, t. mori
 *
 */

#include <stdio.h>
#include <ctype.h> 
#include <opencv2/opencv.hpp>

#define ESCAPE 27 
#define RETURN 10
#define CHESS_ROW_NUM 10
#define CHESS_COL_NUM 7
#define CHESS_ROW_DX 20
#define CHESS_COL_DY 20

typedef struct{
  double dx;
  double dy;
  CvSize patternSize;
} ChessBoard;


void store2DCoordinates( CvMat* cornerPoints, CvPoint2D32f* points, 
                        ChessBoard chess, int imageID );

void store3DCoordinates( CvMat* objectPoints, ChessBoard chess, int imageID );

void loadIntrinsicParams(const char* fileName, CvMat* intrinsic, 
                             CvMat* distortion);

void saveExtrinsicParams(const char* fileName, 
                         CvMat* rotation, CvMat* translation );

void calibrateCamera ( const char* intrinsicFileName, 
                      const char* extrinsicFileName,
                      CvMat* cornerPoints, CvMat* objectPoints );

int main( int argc, char** argv ){
  CvCapture* capture = NULL;
  IplImage* src = NULL;
  IplImage* src2 = NULL;
  IplImage* gray = NULL; 
  IplImage* output = NULL; 

  CvMat* cornerPoints;
  CvMat* objectPoints;
  CvMat pointsNumMat;
  CvPoint2D32f* points;
  int pointsNum[1];

  ChessBoard chess;
  int pointsPerScene;
  int detectedPointsNum;
  int allPointsFound;
  int i, j;
  char key;
  int camID;
  char* windowName = "extrinsic calibration";

  capture = cvCreateCameraCapture(0);

  if(!capture) {
    fprintf(stderr, "ERROR: capture is NULL \n");
    return(-1);
  }

  chess.dx = CHESS_ROW_DX;
  chess.dy = CHESS_COL_DY;
  chess.patternSize.width = CHESS_ROW_NUM;
  chess.patternSize.height = CHESS_COL_NUM;

  pointsPerScene 
    = chess.patternSize.width * chess.patternSize.height;


  cornerPoints = cvCreateMat(pointsPerScene, 2, CV_32F);
  objectPoints = cvCreateMat(pointsPerScene, 3, CV_32F);

  pointsNum[0] = pointsPerScene;
  pointsNumMat = cvMat(1, 1, CV_32S, pointsNum);

  points 
    = (CvPoint2D32f*)malloc( sizeof(CvPoint2D32f) * pointsPerScene ) ;

  src = cvQueryFrame(capture);

  if(src == NULL){
    fprintf(stderr, "Could not grab and retrieve frame...\n");
    return(-1);
  }

  src2 = cvCreateImage(cvSize(src->width, src->height), src->depth, 3);
  output = cvCreateImage(cvSize(src->width, src->height), src->depth, 3);
  
  cvCopy( src, src2, NULL ); 

  gray = cvCreateImage(cvSize(src2->width, src2->height), src2->depth, 1);
  
  cvNamedWindow( windowName, CV_WINDOW_AUTOSIZE );

  while( 1 ){
    src = cvQueryFrame(capture);
    if( !src ) {
      break;
    }
    cvCopy( src, src2, NULL ); 

    cvCopy( src2, output, NULL );

    cvCvtColor(src2, gray, CV_BGR2GRAY);
    
    if( cvFindChessboardCorners( gray, chess.patternSize, points, 
        &detectedPointsNum, CV_CALIB_CB_ADAPTIVE_THRESH ) ){
      cvFindCornerSubPix(gray, points, detectedPointsNum, 
        cvSize(5, 5), cvSize(-1, -1), 
        cvTermCriteria(CV_TERMCRIT_ITER, 100, 0.1));
      allPointsFound = 1;
    } else {
      allPointsFound = 0;
    }
    
    cvDrawChessboardCorners( src2, chess.patternSize, points, 
      detectedPointsNum, allPointsFound );

    cvShowImage(windowName, src2);

    key = cvWaitKey( 20 );
    if(key == RETURN && allPointsFound ){
      store2DCoordinates( cornerPoints, points, chess, 0 );
      store3DCoordinates( objectPoints, chess, 0 );
      calibrateCamera("../param/intrinsic_param_ref.txt", 
        "../param/extrinsic_param.txt", 
        cornerPoints, objectPoints );
      cvSaveImage( "../param/board.jpg", output, 0 );
      break;
    } else if(key == ESCAPE) {
      break;
    }
  }

  cvDestroyWindow( windowName );

  cvReleaseCapture(&capture);

  free(points);
  cvReleaseMat(&cornerPoints);
  cvReleaseMat(&objectPoints);
  cvReleaseImage(&gray);
  cvReleaseImage(&src2);

  return(0);
}

void store2DCoordinates( CvMat* cornerPoints, CvPoint2D32f* points, 
                        ChessBoard chess, int imageID ) {
  int i = 0;
  int pointsPerScene = chess.patternSize.width * chess.patternSize.height;
  int offset = imageID * pointsPerScene;

  for ( i = 0; i < pointsPerScene; i++ ) {
    cvmSet(cornerPoints, offset + i, 0, points[i].x);
    cvmSet(cornerPoints, offset + i, 1, points[i].y);
  }
}

void store3DCoordinates( CvMat* objectPoints,ChessBoard chess, int imageID ) {
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
                         CvMat* distortion){
  //
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

void saveExtrinsicParams(const char* fileName, 
                         CvMat* rotation, CvMat* translation ){
  //
  FILE* file;
  CvMat* R = cvCreateMat(3, 3, CV_64F);
  double xOffset = (CHESS_ROW_NUM - 1) * CHESS_ROW_DX;

  if((file = fopen(fileName, "w")) == NULL){
    fprintf(stderr, "file open error\n");
    return;
  }

  cvRodrigues2(rotation, R, 0);
  
  //inverting z axis and translate origin for some reasons
  fprintf(file, "%lf %lf %lf\n",
    -cvmGet(R, 0, 0), cvmGet(R, 0, 1), -cvmGet(R, 0, 2));
  fprintf(file, "%lf %lf %lf\n",
    -cvmGet(R, 1, 0), cvmGet(R, 1, 1), -cvmGet(R, 1, 2));
  fprintf(file, "%lf %lf %lf\n",
    -cvmGet(R, 2, 0), cvmGet(R, 2, 1), -cvmGet(R, 2, 2));

  fprintf(file, "%lf %lf %lf\n", 
	  cvmGet(translation, 0, 0) + cvmGet(R, 0, 0) * xOffset, 
	  cvmGet(translation, 1, 0) + cvmGet(R, 1, 0) * xOffset, 
	  cvmGet(translation, 2, 0) + cvmGet(R, 2, 0) * xOffset);

  fclose(file);

}

void calibrateCamera ( const char* intrinsicFileName, 
		      const char* extrinsicFileName,
		      CvMat* cornerPoints, CvMat* objectPoints ){

  CvMat* intrinsic = cvCreateMat( 3, 3, CV_64F );
  CvMat* distortion = cvCreateMat( 4, 1, CV_64F );

  CvMat* translation = cvCreateMat( 3, 1, CV_64F );

  CvMat* rotation = cvCreateMat( 3, 1, CV_64F );

  loadIntrinsicParams( intrinsicFileName, intrinsic, distortion );

  cvFindExtrinsicCameraParams2( objectPoints, cornerPoints, 
    intrinsic, distortion, rotation, translation, 0 );

  saveExtrinsicParams( extrinsicFileName, rotation, translation );
  
  cvReleaseMat( &intrinsic );
  cvReleaseMat( &distortion );
  cvReleaseMat( &rotation );
  cvReleaseMat( &translation );
}
