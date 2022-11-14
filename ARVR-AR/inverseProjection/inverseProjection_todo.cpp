/*
 * inverseProjection_todo.c
 * written by m. shimosaka, y. sagawa, t. mori
 * modified by m. shimosaka, t. numata
 *
 */

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

int bytes_per_pixel(const IplImage* image); 

void store2DCoordinates( CvMat* cornerPoints, CvPoint2D32f* points, 
                        ChessBoard chess, int imageID );

void store3DCoordinates( CvMat* objectPoints, ChessBoard chess, int imageID );

void loadIntrinsicParams(const char* fileName, CvMat* intrinsic, 
                             CvMat* distortion);

void calibrate ( CvMat* cornerPoints, CvMat* objectPoints,
                CvMat* intrinsic, CvMat* distortion,
                CvMat* rotation, CvMat* translation );

void drawPlaneImage( IplImage* output, IplImage* plane, 
                    CvMat* intrinsic,
                    CvMat* rotation, CvMat* translation,
                    ChessBoard board );

int regionCheck(CvPoint2D32f point, ChessBoard board );


void computePositionOnPlane( CvPoint m, CvMat* intrinsic,
                            CvMat* rotation, CvMat* translation, 
                            CvPoint2D32f* result );


int main( int argc, char** argv ){
  CvCapture* capture = NULL;
  IplImage* src = NULL;
  IplImage* src2 = NULL;
  IplImage* gray = NULL; 
  IplImage* output = NULL; 
  IplImage* pyonkichiImage;

  CvMat* cornerPoints;
  CvMat* objectPoints;
  CvMat pointsNumMat;
  CvMat* intrinsic;
  CvMat* distortion;
  CvMat* rotation;
  CvMat* translation;

  CvPoint2D32f* points;
  int pointsNum[1];

  ChessBoard chess;
  int pointsPerScene;
  int detectedPointsNum;
  int allPointsFound;
  int i, j;
  char key;
  int camID;
  char* inputWindowName = "input image";
  char* renderedWindowName = "rendered image";
  char* originalWindowName = "original";
  char* intrinsicFileName = "../param/intrinsic_param_ref.txt";
  char* imageFileName = "pyonkichi.jpg";

  capture = cvCreateCameraCapture( 0 );

  if(!capture) {
    fprintf(stderr, "ERROR: capture is NULL \n");
    return(-1);
  }

  chess.dx = CHESS_ROW_DX;
  chess.dy = CHESS_COL_DY;
  chess.patternSize.width = CHESS_ROW_NUM;
  chess.patternSize.height = CHESS_COL_NUM;

  pointsPerScene = chess.patternSize.width * chess.patternSize.height;


  cornerPoints = cvCreateMat(pointsPerScene, 2, CV_32F);
  objectPoints = cvCreateMat(pointsPerScene, 3, CV_32F);

  pointsNum[0] = pointsPerScene;
  pointsNumMat = cvMat(1, 1, CV_32S, pointsNum);

  points = (CvPoint2D32f*)malloc( sizeof(CvPoint2D32f) * pointsPerScene ) ;

  distortion = cvCreateMat( 4, 1, CV_64F );
  intrinsic = cvCreateMat( 3, 3, CV_64F );
  rotation = cvCreateMat( 3, 3, CV_64F );
  translation = cvCreateMat( 3, 1, CV_64F );

  loadIntrinsicParams( intrinsicFileName, intrinsic, distortion );
  pyonkichiImage = cvLoadImage( imageFileName, 1 );

  src = cvQueryFrame( capture );
  if(src == NULL){
    fprintf(stderr, "Could not grab and retrieve frame...\n");
    return(-1);
  }

  src2 = cvCloneImage( src );
  output = cvCloneImage( src );
  
  
  gray = cvCreateImage(cvSize(src2->width, src2->height),
           src2->depth, 1);
  
  cvNamedWindow( inputWindowName, CV_WINDOW_AUTOSIZE );
  cvNamedWindow( renderedWindowName, CV_WINDOW_AUTOSIZE );
  cvNamedWindow( originalWindowName, CV_WINDOW_AUTOSIZE );

  while( 1 ){
    src = cvQueryFrame( capture );
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

    cvShowImage( inputWindowName, src2 );

    if( allPointsFound ){
      store2DCoordinates( cornerPoints, points, chess, 0 );
      store3DCoordinates( objectPoints, chess, 0 );

      calibrate( cornerPoints, objectPoints, 
        intrinsic, distortion, rotation, translation );

      /*
      fprintf( stderr,
	       "%lf %lf %lf\n%lf %lf %lf\n%lf %lf %lf\n\n%lf %lf %lf\n",
	       cvmGet(rotation, 0, 0),
	       cvmGet(rotation, 0, 1),
	       cvmGet(rotation, 0, 2),
	       cvmGet(rotation, 1, 0),
	       cvmGet(rotation, 1, 1),
	       cvmGet(rotation, 1, 2),
	       cvmGet(rotation, 2, 0),
	       cvmGet(rotation, 2, 1),
	       cvmGet(rotation, 2, 2),
	       cvmGet(translation, 0, 0),
	       cvmGet(translation, 1, 0),
	       cvmGet(translation, 2, 0));
      */

      drawPlaneImage( output, pyonkichiImage, intrinsic, 
        rotation, translation, chess );
    }
    
    cvShowImage( renderedWindowName, output );
    cvShowImage( originalWindowName, pyonkichiImage );
    key = cvWaitKey( 20 );

    if(key == ESCAPE) {
      break;
    }

  }

  cvDestroyWindow( inputWindowName );
  cvDestroyWindow( renderedWindowName );
  cvDestroyWindow( originalWindowName );

  cvReleaseCapture(&capture);

  free(points);
  cvReleaseMat(&cornerPoints);
  cvReleaseMat(&objectPoints);
  cvReleaseMat(&intrinsic);
  cvReleaseMat(&rotation);
  cvReleaseMat(&distortion);
  cvReleaseMat(&translation);
  cvReleaseImage(&gray);

  return(0);
}

/* bytes_per_pixel */
int bytes_per_pixel(const IplImage* image) {
  return((((image)->depth & 255) / 8 ) *(image)->nChannels);
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

void calibrate ( CvMat* cornerPoints, CvMat* objectPoints,
                CvMat* intrinsic, CvMat* distortion,
                CvMat* rotation, CvMat* translation ){

  CvMat* rotVector = cvCreateMat( 3, 1, CV_64F );

  int i = 0;
  double xOffset = (CHESS_ROW_NUM - 1) * CHESS_ROW_DX;

  cvFindExtrinsicCameraParams2( objectPoints, cornerPoints, 
    intrinsic, distortion, rotVector, translation );

  cvRodrigues2( rotVector, rotation, 0 );
  
  //inverting z axis and translate origin for some reasons

  /*
  fprintf(stderr, "(%lf %lf %lf)\n", 
	  cvmGet(translation, 0, 0),
	  cvmGet(translation, 1, 0),
	  cvmGet(translation, 2, 0));
  */

  for ( i = 0; i < 3; i++ ){
    cvmSet( translation, i, 0,  
	    cvmGet(translation, i, 0) 
	    + cvmGet(rotation, i, 0) * xOffset );
  }

  for ( i = 0; i < 3; i++ ){
    cvmSet(rotation, i, 0,
      -cvmGet(rotation, i, 0));
    cvmSet(rotation, i, 2,
      -cvmGet(rotation, i, 2));
  }

  cvReleaseMat( &rotVector );
}

void drawPlaneImage( IplImage* output, IplImage* plane, 
                    CvMat* intrinsic,
                    CvMat* rotation, CvMat* translation,
                    ChessBoard board ) {
  //
  int u = 0;
  int v = 0;
  uchar* outputArray;
  uchar* planeArray;
  int stepOutput;
  int stepPlane;
  int bppOutput;
  int bppPlane;
  CvSize sizeOutput;
  CvSize sizePlane;

  int width = output->width;
  int height = output->height;

  cvGetRawData( output, &outputArray, &stepOutput, &sizeOutput );
  cvGetRawData( plane, &planeArray, &stepPlane, &sizePlane );
  bppOutput = bytes_per_pixel( output );
  bppPlane = bytes_per_pixel( plane );

  for( u = 0; u < width; u ++ ) {
    for( v = 0; v < height; v ++ ){
      CvPoint2D32f position;
      computePositionOnPlane( cvPoint( u, v ), intrinsic, 
        rotation, translation, &position );

      if( regionCheck(position, board ) ){
        int x = (int)floor( position.x );
        int y = 120-(int)floor( position.y );

        uchar* pO = outputArray + stepOutput * v + bppOutput * u;
        uchar* pP = planeArray + stepPlane * y + bppPlane * x;

        *(pO + 0) = *(pP + 0);
        *(pO + 1) = *(pP + 1);
        *(pO + 2) = *(pP + 2);
      }
    }
  }
}

int regionCheck(CvPoint2D32f point, ChessBoard board ){
  float x = point.x;
  float y = point.y;
  if( (x >= 0 ) && 
      (x < board.dx * (board.patternSize.width-1)) &&
      (y >= 0 ) &&
      (y < board.dy * (board.patternSize.height-1) )
      ){
    return 1;
  } else {
    return 0;
  }
}

void computePositionOnPlane( CvPoint m, CvMat* intrinsic,
                            CvMat* rotation, CvMat* translation, 
                            CvPoint2D32f* result ){
  double scale = 0;
  int i = 0;

  CvMat* w = cvCreateMat( 3, 1, CV_64F );
  CvMat* tmp = cvCreateMat( 3, 3, CV_64F );
  CvMat* q = cvCreateMat( 3, 1, CV_64F );
  CvMat* mTilda = cvCreateMat( 3, 1, CV_64F );
  CvMat* A = intrinsic;
  CvMat* R = rotation;
  CvMat* o = translation;

  cvmSet( mTilda, 0, 0, m.x );
  cvmSet( mTilda, 1, 0, m.y );
  cvmSet( mTilda, 2, 0, 1.0 );

  //related to Assignment 
  // use cvSolve, cvMatMul, cvmGet, cvmSet
  //TODO

  // tmp = A * R;
  cvMatMul( A, ..., ... );

  // q = (A*R)^{-1} * m~;
  // use tmp variable as (A*R)
  // hint: (A*R) * q = m~
  cvSolve( ..., mTilda, q, CV_LU );

  // w = R^{-1} * o;
  // R * w = o
  cvSolve( R, o, w, CV_LU );
  
  scale = cvmGet( w, 2, 0 ) / cvmGet( q, 2, 0 );

  result->x = scale * cvmGet( q, 0, 0) - cvmGet( w, 0, 0 );
  result->y = scale * cvmGet( q, 1, 0) - cvmGet( w, 1, 0 );

  cvReleaseMat( &w );
  cvReleaseMat( &mTilda );
  cvReleaseMat( &tmp );
  cvReleaseMat( &q );
}
