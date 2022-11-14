/*
 * projection_todo.c
 * written by m. shimosaka
 * modified by t. numata
 *
 */

#include <stdio.h>
#include <ctype.h> 
#include <opencv2/opencv.hpp>
#include <opencv/cvaux.h>

#define ESCAPE 27
#define PROJECTED_POINTS 6

void loadIntrinsicParams(const char* fileName, CvMat* intrinsic, 
                         CvMat* distortion );

void loadExtrinsicParams(const char* fileName, CvMat* rotation, 
                         CvMat* translation );

void project( CvPoint3D32f* objectPoint, CvPoint2D32f* projectedPoint, 
             CvMat* intrinsic, CvMat* rotation, CvMat* translation );

int main( int argc, char* argv[] ){
  int i;
  IplImage* boardImage = NULL;
  IplImage* output = NULL;

  CvMat* intrinsic = cvCreateMat( 3, 3, CV_64F );
  CvMat* distortion = cvCreateMat( 4, 1, CV_64F );
  
  CvMat* rotation = cvCreateMat( 3, 3, CV_64F );
  CvMat* translation = cvCreateMat( 3, 1, CV_64F );

  CvScalar xAxisColor = CV_RGB( 0, 255, 255 );
  CvScalar yAxisColor = CV_RGB( 255, 0, 255 );
  CvScalar zAxisColor = CV_RGB( 255, 255, 0 );
  CvFont font;

  char* intCalibName = "../param/intrinsic_param_ref.txt";
  char* extCalibName = "../param/extrinsic_param.txt";
  char* boardFileName = "../param/board.jpg";

  char* windowName = "projection";

  CvPoint3D32f* objectPoints;
  CvPoint2D32f* projectedPoints;

  boardImage = cvLoadImage( boardFileName, 1 );

  if( !boardImage ){
    fprintf( stderr, "error at loading image.\n" );
  }

  output = cvCloneImage( boardImage );

  cvNamedWindow( windowName, CV_WINDOW_AUTOSIZE );
  cvInitFont( &font, CV_FONT_HERSHEY_PLAIN, 1.0f, 1.0f, 0.0f, 1, CV_AA );

  //load camera parameters
  loadIntrinsicParams ( intCalibName, intrinsic, distortion );
  loadExtrinsicParams ( extCalibName, rotation, translation );

  //allocate memory for 3d position in chess board coordinate
  objectPoints = 
    (CvPoint3D32f*)malloc( sizeof(CvPoint3D32f) * PROJECTED_POINTS );

  // allocate memory for image points
  projectedPoints =
    (CvPoint2D32f*)malloc( sizeof(CvPoint2D32f) * PROJECTED_POINTS );

  // original point of chess board
  objectPoints[0].x = 0;
  objectPoints[0].y = 0;
  objectPoints[0].z = 0;

  // (180, 0, 0);
  objectPoints[1].x = 180;
  objectPoints[1].y = 0;
  objectPoints[1].z = 0;

  // (0, 120, 0);
  objectPoints[2].x = 0;
  objectPoints[2].y = 120;
  objectPoints[2].z = 0;

  // (0, 0, 80);
  objectPoints[3].x = 0;
  objectPoints[3].y = 0;
  objectPoints[3].z = 80;

  // (40, 60, 0);
  objectPoints[4].x = 40;
  objectPoints[4].y = 60;
  objectPoints[4].z = 0;

  // (80, 120, 0);
  objectPoints[5].x = 80;
  objectPoints[5].y = 120;
  objectPoints[5].z = 0;

  for( i = 0; i < PROJECTED_POINTS; i++ ){
    // project object point in image coordinate.
    // related to Assignment.
    project ( &objectPoints[i], &projectedPoints[i], 
      intrinsic, rotation, translation );
  }

  //render lines from the board orgin to the projected points.
  cvLine( output, cvPointFrom32f(projectedPoints[0]), 
    cvPointFrom32f( projectedPoints[1] ), xAxisColor, 2, 8, 0 );

  cvLine( output, cvPointFrom32f(projectedPoints[0]), 
    cvPointFrom32f( projectedPoints[2] ), yAxisColor, 2, 8, 0 );

  cvLine( output, cvPointFrom32f(projectedPoints[0]), 
    cvPointFrom32f( projectedPoints[3] ), zAxisColor, 2, 8, 0 );

  cvCircle( output, cvPointFrom32f( projectedPoints[4] ), 
    5, CV_RGB(255, 0, 0), 2, 8, 0);

  cvCircle( output, cvPointFrom32f( projectedPoints[5] ), 
    5, CV_RGB(255, 0, 0), 2, 8, 0);

  cvPutText( output, "O", cvPointFrom32f( projectedPoints[0]), 
    &font, CV_RGB( 255, 0, 0));
  cvPutText( output, "x", cvPointFrom32f( projectedPoints[1]), 
    &font, xAxisColor);
  cvPutText( output, "y", cvPointFrom32f( projectedPoints[2]), 
    &font, yAxisColor);
  cvPutText( output, "z", cvPointFrom32f( projectedPoints[3]), 
    &font, zAxisColor);

  //modify character info. in image.
  cvPutText( output, "(xx mm, yy mm)", cvPointFrom32f( projectedPoints[4]), 
    &font, CV_RGB(255, 0, 0));

  cvPutText( output, "(xxx mm, yyy mm)", cvPointFrom32f( projectedPoints[5]), 
    &font, CV_RGB(255, 0, 0));

  // showing the rendered result until escape key is pushed.
  cvNamedWindow( windowName, CV_WINDOW_AUTOSIZE ) ;

  while( 1 ){

    int key = cvWaitKey( 20 );
    cvShowImage( windowName, output );
    if( key == ESCAPE ) {
      break;
    }
  }

  cvSaveImage( "axisRendered.jpg", output );

  //release memory at the end of program
  cvReleaseImage( &boardImage );
  cvReleaseImage( &output );

  cvReleaseMat( &intrinsic );
  cvReleaseMat( &distortion );
  cvReleaseMat( &rotation );
  cvReleaseMat( &translation );

  cvDestroyWindow ( windowName );

  free( objectPoints );
  free( projectedPoints );

  return(0);
}

void loadIntrinsicParams( const char* fileName, CvMat* intrinsic, 
                         CvMat* distortion ){
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

void loadExtrinsicParams( const char* fileName, CvMat* rotation, 
                         CvMat* translation ){
  //
  FILE* file;
  int i, j;

  if((file = fopen(fileName, "r")) == NULL){
    fprintf(stderr, "file open error\n");
    return;
  }

  for( i = 0; i < 3; i++ ){
    for( j = 0; j < 3; j++ ){
      double value = 0;
      fscanf( file, "%lf", &value );
      cvmSet( rotation, i, j, value );
    }
  }

  for( i = 0; i < 3; i++ ){
    double value;
    fscanf( file, "%lf", &value );
    cvmSet( translation, i, 0,  value );
  }

  fclose(file);
}

void project( CvPoint3D32f* objectPoint, CvPoint2D32f* projectedPoint, 
             CvMat* intrinsic, CvMat* rotation, CvMat* translation ) {

  double x[3];
  double xInCameraCoord[3];
  double A[3][3];
  double R[3][3];
  double o[3];
  double projected[3];

  double tmp[3];

  int i, j;

  x[0] = objectPoint->x;
  x[1] = objectPoint->y;
  x[2] = objectPoint->z;

  for ( i = 0; i < 3; i++ ){
    for( j = 0; j < 3; j++ ){
      A[i][j] = cvmGet( intrinsic, i, j );
      R[i][j] = cvmGet( rotation, i, j );
    }
    o[i] = cvmGet( translation, i, 0 );
  }

  //compute projected points in the image-plane
  // related to Assignment
  // projected~ = s A ( R * x + o ) 
  // see eq. 20 in the text
  // TODO

  //compute tmp = R * x + o;

  tmp[0] = R[0][0] * x[0] + R[0][1] * x[1] + R[0][2] * x[2] + o[0];
  tmp[1] = ...;
  tmp[2] = ...;

  // compute projected~ = A * tmp;
  projected[0] = A[0][0] * tmp[0] + A[0][1] * tmp[1] + A[0][2] * tmp[2];
  projected[1] = ...;
  projected[2] = ...;

  //divide scale factor s
  projected[0] /= projected[2];
  projected[1] /= projected[2];

			     
  projectedPoint->x = projected[0];
  projectedPoint->y = projected[1];
}
