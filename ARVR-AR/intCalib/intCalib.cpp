/*
 * intCalib.c
 * written by m. shimosaka, y. sagawa, t. mori
 * modified by t. numata
 *
 */
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <opencv2/opencv.hpp>
using namespace cv;
using namespace std;

#define ESCAPE 27 
#define RETURN 10
#define CHESS_ROW_NUM 10
#define CHESS_COL_NUM 7
#define CHESS_ROW_DX 20
#define CHESS_COL_DY 20
#define IMAGE_NUM 10

typedef struct{
  double dx;
  double dy;
  Size patternSize;
} ChessBoard;
void store2DCoordinates( vector<vector<Point2f>>* cornerPoints, vector<Point2f> points, ChessBoard chess, int imageID );
void store3DCoordinates( vector<vector<Point3f>>* objectPoints,ChessBoard chess, int imageID );
void calibrateCameraIntrinsic ( const char* intrinsicFileName, ChessBoard chess, 
                               vector<vector<Point2f>> cornerPoints, vector<vector<Point3f>> objectPoints );

int main( int argc, char** argv ){
  
  VideoCapture capture(0);
  Mat src;
  Mat smallSrc;
  Mat gray; 

  vector<vector<Point2f>> cornerPoints;
  vector<vector<Point3f>> objectPoints;
  Mat pointsNumMat;
  vector<Point2f> points;
  int pointsNum[IMAGE_NUM];
  int currImgNum = 0;

  ChessBoard chess;
  int pointsPerScene;
  int allPointsFound;
  int i, j;
  char key;
  int camID;
  char* windowName = "intrinsic calibration";

  if( !capture.isOpened() ) {
    fprintf(stderr, "ERROR: capture is NULL \n");
    return(-1);
  }

  chess.dx = CHESS_ROW_DX;
  chess.dy = CHESS_COL_DY;
  chess.patternSize.width = CHESS_ROW_NUM;
  chess.patternSize.height = CHESS_COL_NUM;

  pointsPerScene = chess.patternSize.width * chess.patternSize.height;

  for( i = 0; i < IMAGE_NUM; i++ ) {
    pointsNum[i] = pointsPerScene;
  }

  pointsNumMat = Mat(IMAGE_NUM, 1, CV_32S, pointsNum);

  capture >> src;
  if(&src == nullptr){
    fprintf(stderr, "Could not grab and retrieve frame...\n");
    return(-1);
  }

  src.copyTo(smallSrc);
  
  gray = Mat(cvSize(smallSrc.rows, smallSrc.cols), 
		       smallSrc.dims, 1);

  cvNamedWindow( windowName, CV_WINDOW_AUTOSIZE );

  while( 1 ){
    capture >> src;
    if(&src == nullptr) {
      fprintf(stderr, "Could not grab and retrieve frame...\n");
      return(-1);
    }

    src.copyTo(smallSrc);

    cvtColor(smallSrc, gray, CV_BGR2GRAY);
    
    if( findChessboardCorners( gray, chess.patternSize, points, CV_CALIB_CB_ADAPTIVE_THRESH ) ){
      find4QuadCornerSubpix(gray, points, Size(5, 5));
      allPointsFound = 1;
    } else {
      allPointsFound = 0;
    }
    
    drawChessboardCorners( smallSrc, chess.patternSize, points, allPointsFound);

    imshow(windowName, smallSrc);

    key = cvWaitKey(20);
    if(key == RETURN && allPointsFound ){
      store2DCoordinates( &cornerPoints, points, chess, currImgNum );
      store3DCoordinates( &objectPoints, chess, currImgNum );
      
      currImgNum++;
      fprintf(stderr, "scene %d is saved\n", currImgNum );

      if(currImgNum == IMAGE_NUM){
        calibrateCameraIntrinsic("../param/intrinsic_param.txt", chess, cornerPoints, objectPoints );
        break;
      }
    } else if(key == ESCAPE) {
      break;
    }
  }

  cvDestroyWindow( windowName );

  capture.release();
  return 0;
}

void store2DCoordinates( vector<vector<Point2f>>* cornerPoints, vector<Point2f> points, 
                        ChessBoard chess, int imageID ) {
  cornerPoints->push_back(points);
}

void store3DCoordinates( vector<vector<Point3f>>* objectPoints,ChessBoard chess, int imageID ) {
  int pointsPerScene = chess.patternSize.width * chess.patternSize.height;
  int rowNum = chess.patternSize.width;
  int colNum = chess.patternSize.height;
  double dx = chess.dx;
  double dy = chess.dy;

  vector<Point3f> vec;
  for( int i = 0; i < colNum; i++){
    for (auto j = 0; j < rowNum; j++) {
      int offset = pointsPerScene * imageID;

      Point3f p(i * dx, j * dy, 0.0);
      vec.push_back(p);
    }
  }
  objectPoints->push_back(vec);
}

void saveIntrinsicParams(const char* fileName, 
                         Mat intrinsic, Mat distortion ){
  FILE* file;
  if((file = fopen(fileName, "w")) == NULL){
    fprintf(stderr, "file open error\n");
    return;
  }

  fprintf(file, "%lf %lf %lf\n", intrinsic.at<double>(0, 0), intrinsic.at<double>(0, 1), intrinsic.at<double>(0, 2));
  fprintf(file, "%lf %lf %lf\n", intrinsic.at<double>(1, 0), intrinsic.at<double>(1, 1), intrinsic.at<double>(1, 2));
  fprintf(file, "%lf %lf %lf\n", intrinsic.at<double>(2, 0), intrinsic.at<double>(2, 1), intrinsic.at<double>(2, 2));
  fprintf(file, "%lf %lf %lf %lf\n", distortion.at<double>(0, 0), distortion.at<double>(1, 0), distortion.at<double>(2, 0), distortion.at<double>(3, 0));

  fclose(file);

}

void calibrateCameraIntrinsic ( const char* intrinsicFileName, ChessBoard chess, 
                               vector<vector<Point2f>> cornerPoints, vector<vector<Point3f>> objectPoints ){
  Mat intrinsic(3, 3, CV_64F);
  Mat distortion(4, 1, CV_64F);
  vector<Mat> rvecs, tvecs;
  calibrateCamera( objectPoints, cornerPoints, chess.patternSize, intrinsic, distortion, rvecs, tvecs);
  saveIntrinsicParams( intrinsicFileName, intrinsic, distortion );
}
