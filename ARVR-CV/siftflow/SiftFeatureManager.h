#ifndef __ICS_HAR_VISION_SIFT_FEATURE_MANAGER__
#define __ICS_HAR_VISION_SIFT_FEATURE_MANAGER__

#include <math.h>

#include <iostream>
#include <set>
#include <vector>

#include "cv.h"

#include "SiftFeature.h"

namespace ics{
  namespace har{
    namespace vision{

      class SiftFeatureManager{
      private:
        class Feature: SiftFeature{
        public:
          double* array;
          int dimension;
          int interval;
          int octave;
          double orientation;
          double scale;
          float scaleInOctave;
          float subInterval;
          int u;
          int v;
          double x;
          double y;
        public:
          Feature(){
            array = NULL;
            dimension = 0;
          }

          ~Feature(){
            if(array){
              delete[] array;
            }
          }

          double getX() const{
            return x;
          }

          double getY() const{
            return y;
          }

          double getDimension() const{
            return dimension;
          }

          double getScale() const{
            return scale;
          }

          double getOrientation() const{
            return orientation;
          }

          double distance(const SiftFeature* feature) const{
            SiftFeatureManager::Feature* ft = (SiftFeatureManager::Feature*) feature;
            int dim = dimension;
            double* ptr1 = this->array;
            double* ptr2 = ft->array;
            double sum = 0;
            while(dim-->0){
              double dis = (*ptr1++ - *ptr2++);
              sum += dis * dis;
            }

            return sqrt(sum);
          }
        };

        // fileds
        IplImage*** pyramid;
        IplImage*** dogs;
        IplImage* img32f;

        int intervalPerOctave;
        int numOfOctaves;
        float sigma0;
        float contrastThreshold;
        float currentContrastThreshold;
        float curveThreshold;
        float eigenValRatioThreshold;
        const int ignoreBorder;
        int dimOriHist;
        double oriSigmaFactor;
        int numOriHistSmooth;
        double oriPeakRatioThreshold;
        int numOfDivision;
        int binsPerHistogram;
        int dimOfDescriptor;
        double descrMagThreshold;

        std::vector<Feature*> featureStorage;
        int currentIndex;
        const double PI2;
        float* checkOrientation;

        bool imgDbl;

        //private methods
        void buildGaussPyramid(){

          float k = pow(2.0f, 1.0f / intervalPerOctave);

          //assume Gaussian blur 0.5
          if(imgDbl){
            double modSig = sqrt(sigma0 * sigma0 - 1.0 * 1.0);
            cvSmooth(pyramid[0][0], pyramid[0][0], CV_GAUSSIAN, 0, 0, modSig, modSig);
          } else{
            double modSig = sqrt(sigma0 * sigma0 - 0.5 * 0.5);
            cvSmooth(pyramid[0][0], pyramid[0][0], CV_GAUSSIAN, 0, 0, modSig, modSig);
          }

          for(int o = 0; o < numOfOctaves; o++){
            if(o > 0){
              downSample(pyramid, o, intervalPerOctave);
            }

            for( int i = 1; i < intervalPerOctave + 3; i++){
              IplImage* gImagePrev = pyramid[o][i-1];
              IplImage* gImage = pyramid[o][i];

              float prevSig = pow(k, (i-1)) * sigma0;
              float currSig = prevSig * k;

              float modifiedSigma = sqrt(currSig * currSig - prevSig * prevSig );
              int windowSize = winSize(modifiedSigma);

              cvSmooth(gImagePrev, gImage, CV_GAUSSIAN, 0, 0, modifiedSigma, modifiedSigma);
            }
          }

          /*
          cvNamedWindow("test sift", CV_WINDOW_AUTOSIZE);
          IplImage* tmp2 = cvCloneImage(pyramid[0][0]);
          for(int o = 0; o< numOfOctaves; o++){
            for(int i = 0; i< intervalPerOctave + 3; i++){
              cvMoveWindow("test sift", 0, 0);
              cvResize(pyramid[o][i], tmp2);
              cvShowImage("test sift", tmp2);
              std::cout << pyramid[o][i]->width << ", " << pyramid[o][i]->height << std::endl;
              cvWaitKey(100);
            }
          }
          */
        }

        bool isValidRange(int u, int v, int interval, int octave){
          if( interval < 1){
            return false;
          }

          if(interval > intervalPerOctave){
            return false;
          }

          if(u < ignoreBorder){
            return false;
          }
          if(v < ignoreBorder){
            return false;
          }
          if(u >= dogs[octave][0]->width - ignoreBorder){
            return false;
          }

          if(v >= dogs[octave][0]->height - ignoreBorder ) {
            return false;
          }

          return true;
        }

        void newtonDirection(int octave, int interval, int u, int v, float* di, float* du, float* dv){

          CvMat* deriv = derivDoG( octave, interval, u, v );
          CvMat* hessian = hessianDoG( octave, interval, u, v );

          CvMat*  X = cvCreateMat(3, 1, CV_64FC1);

          cvSolve(hessian, deriv, X, CV_SVD);
  
          *di = -cvmGet(X, 0, 0);
          *du = -cvmGet(X, 1, 0);
          *dv = -cvmGet(X, 2, 0);

          cvReleaseMat( &deriv );
          cvReleaseMat( &hessian );
          cvReleaseMat(&X);
        }

        CvMat* derivDoG(int octave, int interval, int u, int v){

          float di = ( pixval32f( dogs[octave][interval+1], u, v ) -
            pixval32f( dogs[octave][interval-1], u, v ) ) / 2.0;


          float du = ( pixval32f( dogs[octave][interval], u+1, v ) -
            pixval32f( dogs[octave][interval], u-1, v ) ) / 2.0;


          float dv = ( pixval32f( dogs[octave][interval], u, v+1 ) -
            pixval32f( dogs[octave][interval], u, v-1 ) ) / 2.0;


  
          CvMat* delta = cvCreateMat( 3, 1, CV_64FC1 );
          cvmSet( delta, 0, 0, di );
          cvmSet( delta, 1, 0, du );
          cvmSet( delta, 2, 0, dv );

          return delta;
        }

        CvMat* hessianDoG(int octave, int interval, int u, int v){

          float val = pixval32f( dogs[octave][interval], u, v );


          float dii = ( pixval32f( dogs[octave][interval+1], u, v ) +
            pixval32f( dogs[octave][interval-1], u, v ) - 2 * val );

          float duu = ( pixval32f( dogs[octave][interval], u+1, v ) +
            pixval32f( dogs[octave][interval], u-1, v ) - 2 * val );

          float dvv = ( pixval32f( dogs[octave][interval], u, v+1 ) + 
            pixval32f( dogs[octave][interval], u, v-1 ) - 2 * val );

          float diu = ( pixval32f( dogs[octave][interval+1], u+1, v ) -
            pixval32f( dogs[octave][interval+1], u-1, v ) -
            pixval32f( dogs[octave][interval-1], u+1, v ) +
            pixval32f( dogs[octave][interval-1], u-1, v ) ) / 4.0;

          float duv = ( pixval32f( dogs[octave][interval], u+1, v+1 ) -
            pixval32f( dogs[octave][interval], u+1, v-1 ) -
            pixval32f( dogs[octave][interval], u-1, v+1 ) +
            pixval32f( dogs[octave][interval], u-1, v-1 ) ) / 4.0;

          float dvi = ( pixval32f( dogs[octave][interval+1], u, v+1 ) -
            pixval32f( dogs[octave][interval+1], u, v-1 ) -
            pixval32f( dogs[octave][interval-1], u, v+1 ) +
            pixval32f( dogs[octave][interval-1], u, v-1 ) ) / 4.0;

          CvMat* H = cvCreateMat( 3, 3, CV_64FC1 );

          cvmSet(H, 0, 0, dii); cvmSet( H, 0, 1, diu ); cvmSet( H, 0, 2, dvi );
          cvmSet( H, 1, 0, diu ); cvmSet( H, 1, 1, duu ); cvmSet( H, 1, 2, duv );
          cvmSet( H, 2, 0, dvi ); cvmSet( H, 2, 1, duv ); cvmSet( H, 2, 2, dvv );

          return H;
        }

        Feature* interpolateExtremum(int octave, int interval, int u, int v) {
          float di = 0;
          float du = 0;
          float dv = 0; 
          int iteration = 0;

          int maxIteration = 5;
  

          while( iteration < maxIteration ) {
            newtonDirection(octave, interval, u, v, &di, &du, &dv );
            if(fabs(di) < 0.5  &&  fabs( du ) < 0.5  &&  fabs( dv ) < 0.5 ){
              break;
            }

            interval += cvRound( di );
            u += cvRound( du );
            v += cvRound( dv );

            if(!isValidRange(u, v, interval, octave)){
              return NULL;
            }
      
            iteration++;
          }
  
          if( iteration >= maxIteration){
            return NULL;
          }
  
          float contr = interpolateContrast(octave, interval, u, v, di, du, dv );

          if(fabs(contr) < currentContrastThreshold){
            return NULL;
          }

          Feature* feat = new Feature();
          feat->interval = interval;
          feat->octave = octave;
          feat->subInterval = di;
          feat->u = u;
          feat->v = v;

          feat->x = (u + du) * pyramid[0][0]->width / pyramid[octave][0]->width;
          feat->y = (v + dv) * pyramid[0][0]->height / pyramid[octave][0]->height;

          return feat;
        }

        double interpolateContrast(int octave, int interval, int u,
          int v, float di, float du, float dv ) {

          CvMat* deriv = derivDoG(octave, interval, u, v);

          float derivI = cvmGet(deriv, 0, 0);
          float derivU = cvmGet(deriv, 1, 0);
          float derivV = cvmGet(deriv, 2, 0);
          
          float innerProd = derivI * di + derivU * du + derivV * dv;

          cvReleaseMat(&deriv);

          return pixval32f(dogs[octave][interval], u, v) + innerProd * 0.5;
        }

        void findKeyPoints(int octave){

          int width = dogs[octave][0]->width;
          int height = dogs[octave][0]->height;
          int widthMax = width - ignoreBorder;
          int heightMax = height - ignoreBorder;

          std::set<int> position;

          for( int interval = 1; interval <= intervalPerOctave; interval++){
            currentContrastThreshold =  contrastThreshold / intervalPerOctave;
            double preliminaryThreshold = 0.2 * currentContrastThreshold;

            for( int v = ignoreBorder; v < heightMax; v++){
              float* imgPtr = ptr32f(dogs[octave][interval], ignoreBorder, v);
              for( int u = ignoreBorder; u < widthMax; u++, imgPtr++){

                std::set<int>::iterator itr = position.find(v *width + u);
                if(itr != position.end()){
                  continue;
                }

                if(fabs(*imgPtr) < preliminaryThreshold){
                  continue;
                }

                if(!isExtremum(octave, interval, u, v, *imgPtr)){
                  continue;
                }

                Feature* feature = interpolateExtremum(octave, interval, u, v);

                if(!feature){
                 continue; 
                }

                if(!isCorner(dogs[octave][interval], feature->u, feature->v)){
                  delete feature;
                  continue;
                }


                calcScale(feature);
                calcOrientation(feature);

                position.insert(v * width + u);

                delete feature;
              }
            }
          }
        }

        void calcScale(Feature* feat){
          double interval = (feat->interval + feat->subInterval) / intervalPerOctave;
          feat->scale = sigma0 * pow(2.0, feat->octave + interval);
          feat->scaleInOctave = sigma0 * pow(2.0, interval);
        }

        void getOriHistogram( IplImage* img, int u, int v, double sigma, double* hist){
          memset(hist, 0, sizeof(double) * dimOriHist);

          int rad = cvRound(oriSigmaFactor * 3 * sigma);

          double expNormalizer= 0.5 / ( sigma  * sigma  * oriSigmaFactor * oriSigmaFactor) ;

          for(int  i = -rad; i <= rad; i++ ){
            for(int  j = -rad; j <= rad; j++ ){
              double mag, ori;
              if( calcGradMagOrientation( img, u + i, v + j, &mag, &ori ) ) {
                double w = exp( -( i*i + j*j ) * expNormalizer );
                int bin = cvRound(dimOriHist * ori / PI2);

                if(bin >= dimOriHist){
                  bin = 0;
                }

                hist[bin] += w * mag;
              }
            }
          }
        }

        bool calcGradMagOrientation( IplImage* img, int u, int v, double* mag, double* ori ){
          if( (v > 0)  &&  (v < img->height - 1)  && 
            (u > 0)  &&  (u < img->width - 1) ) 
          {
            //double dy = (pixval32f( img, u, v+1 ) - pixval32f( img, u, v-1 )) / 2.0;
            //double dx = (pixval32f( img, u+1, v ) - pixval32f( img, u-1, v )) / 2.0;
            double dy = (pixval32f( img, u, v+1 ) - pixval32f( img, u, v-1 ));
            double dx = (pixval32f( img, u+1, v ) - pixval32f( img, u-1, v ));

            *mag = sqrt( dx*dx + dy*dy );
            *ori = atan2( -dy, -dx ) + CV_PI;
            return true;
          } else{
            return false;
          }
        }

        void calcOrientation(Feature* feat){
          double hist[36];

          memset(hist, 0, sizeof(double) * 36);

          getOriHistogram( pyramid[feat->octave][feat->interval],
            feat->u, feat->v, feat->scaleInOctave, hist);

          for(int  i = 0; i < numOriHistSmooth; i++ ){
            smoothOrientationHistogram(hist);
          }

          double omax = detectDominantOri(hist);

          assignOriAndRegister(hist, feat, omax * oriPeakRatioThreshold);

        }

        void smoothOrientationHistogram(double* hist){
          double h0 = hist[0];
          hist[0] = (hist[dimOriHist-1] + 2 * hist[0] + hist[1] ) / 4.0;

          double hPrev = h0;
          for(int d = 1; d < dimOriHist-1; d++){
            double tmp = hist[d];
            hist[d] =  (hPrev + 2 * hist[d] + hist[d+1] ) / 4.0;
            hPrev = tmp;
          }

          hist[dimOriHist-1] = (hPrev + 2 * hist[dimOriHist-1] + h0) / 4.0;
        }

        double detectDominantOri( const double* hist){
          double omax = hist[0];
          int maxbin = 0;

          for(int i = 1; i < dimOriHist; i++ ){
            if( hist[i] > omax ) {
              omax = hist[i];
              maxbin = i;
            }
          }
          return omax;
        }

        double interpolateHistPeak(double  left, double center, double right ) {
          double grad = 0.5 * (left - right);
          double hess = left - 2.0 * center + right;
          return grad / hess;
        }


        void assignOriAndRegister(double* hist, Feature* feature, double threshold) {

          for(int b = 0; b < dimOriHist; b++ ) {
            int left = ( b == 0 )? dimOriHist - 1 : b-1;
            int right = ( b + 1 ) % dimOriHist;
      
            if((hist[b] > hist[left])  &&  
              (hist[b] > hist[right])  &&
              (hist[b] >= threshold )) {

              double bin = b + interpolateHistPeak(hist[left], hist[b], hist[right] );

              if(bin < 0){
                bin = dimOriHist + bin;
              }else if(bin >= dimOriHist){
                bin -= dimOriHist;
              }

              Feature* newFeature = new Feature();

              newFeature->interval = feature->interval;
              newFeature->octave = feature->octave;
              newFeature->scale = feature->scale;
              newFeature->scaleInOctave = feature->scaleInOctave;
              newFeature->subInterval= feature->subInterval;
              newFeature->u = feature->u;
              newFeature->v = feature->v;
              newFeature->x = feature->x;
              newFeature->y = feature->y;

              newFeature->orientation =  (PI2 * bin ) / dimOriHist;

              featureStorage.push_back(newFeature);
            }
          }
        }

        bool isCorner(IplImage* image, int u, int v){
          float val = pixval32f(image, u, v);

          float dxx = pixval32f( image, u+1, v ) + pixval32f( image, u-1, v ) - 2 * val;
          float dyy = pixval32f( image, u, v+1 ) + pixval32f( image, u, v-1 ) - 2 * val;
          float dxy = ( pixval32f(image, u+1, v+1) - pixval32f(image, u+1, v-1) -
            pixval32f(image, u-1, v+1) + pixval32f(image, u-1, v-1) ) / 4.0;
          
          float trace = dxx + dyy;
          float det = dxx * dyy - dxy * dxy;

          if( det <= 0 ){
            return false;
          }

          if( trace * trace / det < curveThreshold){
            return true;
          } else{
            return false;
          }
        }

        void findKeyPoints(){
          for(int o = 0; o < numOfOctaves; o++){
            findKeyPoints(o);
          }
          adjustScale();
        }

        bool isExtremum(int octave, int interval, int u, int v, float val){
          float min = val;
          float max = val;

          for(int  i = -1; i <= 1; i++ ){
            for(int du = -1; du <= 1; du++ ){
              for(int  dv = -1; dv <= 1; dv++ ){
                float cmpVal = pixval32f( dogs[octave][interval+i], u + du, v + dv ) ;
                if(cmpVal > max){
                  max = cmpVal;
                }
                if(cmpVal < min){
                  min = cmpVal;
                }
                if((val < max) && (val > min)){
                  return false;
                }
              }
            }
          }

          if((val < max) && (val > min)){
            return false;
          }
          
          return true;

        }

        float pixval32f(IplImage* image, int u, int v){
          float* ptr = (float*)image->imageData;
          return *(ptr + v * image->width + u);
        }

        float* ptr32f(IplImage* image, int u, int v){
          float* ptr = (float*)image->imageData;
          return ptr + v * image->width + u;
        }

        void downSample(IplImage*** pyramid, int o, int s){
          cvResize(pyramid[o-1][s], pyramid[o][0], CV_INTER_NN);
        }


        void buildDoGPyramid(){
          int numOfDoGs = intervalPerOctave + 2;

#ifdef _OPENMP
#pragma omp parallel for
#endif
          for(int o = 0; o < numOfOctaves; o++){
            for( int i = 0; i < numOfDoGs; i++){
              cvSub(pyramid[o][i+1], pyramid[o][i], 
                dogs[o][i], NULL);
            }
          }

          /*
          cvNamedWindow("test sift", CV_WINDOW_AUTOSIZE);
          cvMoveWindow("test sift", 0, 0);
          IplImage* tmp = cvCloneImage(dogs[0][0]);
          cvConvertScale(dogs[0][0], tmp, 5.0);
          cvShowImage("test sift", tmp);
          cvWaitKey(0);
          cvReleaseImage(&tmp);
          */


          /**
          cvNamedWindow("test sift", CV_WINDOW_AUTOSIZE);
          IplImage* tmp2 = cvCloneImage(dogs[0][0]);
          for(int o = 0; o< numOfOctaves; o++){
            for(int i = 0; i< numOfDoGs; i++){
              cvMoveWindow("test sift", 0, 0);
              cvResize(dogs[o][i], tmp2);
              cvShowImage("test sift", tmp2);
              std::cout << dogs[o][i]->width << ", " << dogs[o][i]->height << std::endl;
              cvWaitKey(100);
            }
          }
          */
        }

        void releaseImageBuffer(){
          if(pyramid == NULL){
            return ;
          }

          for(int o = 0;  o < numOfOctaves; o++){
            for(int i = 0; i < intervalPerOctave + 3; i++){
              cvReleaseImage(&pyramid[o][i]);
            }
            delete[] pyramid[o];

            for( int i = 0; i < intervalPerOctave + 2; i++){
              cvReleaseImage(&dogs[o][i]);
            }
            delete[] dogs[o];
          }

          delete[] pyramid;
          delete[] dogs;

          if(img32f == NULL){
            return;
          }
          cvReleaseImage(&img32f);
        }

        bool needForAllocateBuffer(IplImage* img){
          if(pyramid == NULL){
            return true;
          }

          int width = imgDbl? img->width * 2: img->width;
          int height = imgDbl? img->height * 2: img->height;

          if((pyramid[0][0]->width != width) ||
             (pyramid[0][0]->height != height)){
               return true;
          }

          return false;
        }

        void calcNumOfOctaves(IplImage* image){
          int width = imgDbl? image->width * 2: image->width;
          int height = imgDbl? image->height * 2: image->height;

          double minSize = (width < height)? width: height;

          numOfOctaves = (int)(log(minSize) / log(2.0) - 2.0);

          //numOfOctaves = cvFloor(log(minSize) / log(2.0) - 2.0);
        }

        void allocateImgBuffer(IplImage* img){
          if(needForAllocateBuffer(img)){
            releaseImageBuffer();
            calcNumOfOctaves(img);

            pyramid = new IplImage**[numOfOctaves];
            dogs = new IplImage**[numOfOctaves];

            int width = imgDbl? img->width * 2:img->width;
            int height = imgDbl? img->height * 2:img->height;

            img32f = cvCreateImage(cvGetSize(img), IPL_DEPTH_32F, 1);

            for(int o = 0; o < numOfOctaves; o++){
              pyramid[o] = new IplImage*[intervalPerOctave + 3];
              dogs[o] = new IplImage*[intervalPerOctave + 2];

              double scaleFactor = pow(2.0, o);

              CvSize size = cvSize(width, height);

              for(int i = 0; i < intervalPerOctave + 3; i++){
                pyramid[o][i] = cvCreateImage(size, IPL_DEPTH_32F, 1);
              }

              for(int i = 0; i < intervalPerOctave + 2; i++){
                dogs[o][i] = cvCreateImage(size, IPL_DEPTH_32F, 1);
              }

              width /= 2;
              height /= 2;
            }
          }
        }

        void convertGray32f(IplImage* src, IplImage* dest){
          assert(src->nChannels == 1);
          assert(src->depth == IPL_DEPTH_8U);

          if(imgDbl){
            cvConvertScale(src, img32f, 1.0 / 255.0, 0);
            cvResize(img32f, dest, CV_INTER_CUBIC);
          } else{
            cvConvertScale(src, dest, 1.0 / 255.0, 0);
          }
        }

        int winSize(float sigma){
          return cvRound(sigma * 2 * 4 + 1)|1;
        }

        void releaseFeature(){
          int size = featureStorage.size();
          for(int i = currentIndex; i< size; i++){
            Feature* tmp =  featureStorage[i];
            delete tmp;
          }
          featureStorage.clear();
        }

        void computeDescriptors(){
#ifdef _OPENMP
#pragma omp parallel for
#endif
          for(int i = 0; i < featureStorage.size(); i++ ) {
            Feature* feature = featureStorage[i];
            describe(feature);
            rescaleDescriptor(feature);
          }
        }

        void releaseHistogram( double**** vvv){
          double*** hist = *vvv;

          for(int  i = 0; i < numOfDivision; i++) {
            for(int j = 0; j < numOfDivision; j++ ){
              delete[] hist[i][j];
            }
            delete[] hist[i];
          }

          delete hist;
        }

        void describe(Feature* feature){
          IplImage* img = pyramid[feature->octave][feature->interval];
          int u = feature->u;
          int v = feature->v;
          double ori = feature->orientation;
          double scale = feature->scaleInOctave;

          feature->array = new double[dimOfDescriptor];
          memset(feature->array, 0, sizeof(double) * dimOfDescriptor);
          feature->dimension = dimOfDescriptor;

          double w;

          double*** hist = new double**[numOfDivision];

          for(int i = 0; i < numOfDivision; i++ ) {
            hist[i] = new double*[numOfDivision];
            for(int j = 0; j < numOfDivision; j++ ){
              hist[i][j] = feature->array + (i * numOfDivision + j) * binsPerHistogram;
            }
          }
  
          double cosT = cos( ori );
          double sinT = sin( ori );

          double binsPerRadian = binsPerHistogram / PI2;

          // expDenom = 2 * (numOfDivision / 2) * (numOfDivision / 2)  = numOfDivision^2 /2
          double expDenom = numOfDivision * numOfDivision / 2.0;

          double histWidth = 3.0 * scale;

          //orientationに関わらず最大のサーチ領域を確保．
          int radius = (int)(histWidth * sqrt(2.0) * (numOfDivision + 1.0 ) * 0.5 + 0.5);

          for(int i = -radius; i <= radius; i++ ){
            for(int  j = -radius; j <= radius; j++ ) {
              //coordinate transformation by rot(ori)
              double uRot = ( i * cosT - j * sinT ) / histWidth;
              double vRot = ( i * sinT + j * cosT ) / histWidth;

              //(1,1) ~ (numOfDivision, numOfDivision)のどの領域に属するかを計算．
              double ubin = uRot + numOfDivision / 2 - 0.5;
              double vbin = vRot + numOfDivision / 2 - 0.5;

              double gradMag;
              double gradOri;	
              if((ubin > -1.0) && (ubin < numOfDivision)  &&  
                  (vbin > -1.0) && (vbin < numOfDivision))
              {
                if( calcGradMagOrientation(img, u + j, v + i, &gradMag, &gradOri)){
                  gradOri -= ori;

                  while(gradOri < 0.0){
                    gradOri += PI2;
                  } 

                  while ( gradOri >= PI2 ){
                    gradOri -= PI2;
                  }
	      
                  double obin = gradOri * binsPerRadian;

                  double w = exp( -(uRot * uRot + vRot * vRot) / expDenom );

                  interpHistEntry( hist, ubin, vbin, obin, gradMag * w);
                }
              }
            }
          }

          for(int i = 0; i < numOfDivision; i++){
            delete[] hist[i];
          }
          delete[] hist;
        }

        void interpHistEntry( double*** hist, double ubin, double vbin, double obin, double mag) {
          int v0 = cvFloor( vbin );
          int u0 = cvFloor( ubin );
          int o0 = cvFloor( obin );
          double dV = vbin - v0;
          double dU = ubin - u0;
          double dO = obin - o0;

          for(int v = 0; v <= 1; v++ ) {
            int vb = v0 + v;
            if( vb >= 0  &&  vb < numOfDivision ) {
              double vV = mag * ( ( v == 0 )? 1.0 - dV : dV );
              double** row = hist[vb];
              for(int  u = 0; u <= 1; u++ ){
                int ub = u0 + u;
                if( ub >= 0  &&  ub < numOfDivision ) {
                  double vU = vV * ( ( u == 0 )? 1.0 - dU : dU );
                  double* h = row[ub];
                  for(int o = 0; o <= 1; o++ ) {
                    int ob = ( o0 + o ) % binsPerHistogram;
                    double vO = vU * ( ( o == 0 )? 1.0 - dO : dO );
                    h[ob] += vO;
                  }
                }
              }
            }
          }
        }

        void rescaleDescriptor( Feature* feat ) {
          normalize(feat->array, feat->dimension);

          for(int i = 0; i < dimOfDescriptor; i++ ){
            if( feat->array[i] > descrMagThreshold ){
              feat->array[i] = descrMagThreshold;
            }
          }

          normalize(feat->array, feat->dimension);

          for(int i = 0; i < dimOfDescriptor; i++ ) {
            int intValue = (int)(512.0 * feat->array[i]);
            feat->array[i] = (255 < intValue)? 255: intValue;
          }
        }

        void normalize(double* array, int dimension){
          int dim = dimension;
          double* data = array;

          double sum = 0;
          while(--dim >= 0){
            sum += *(data) * *(data);
            data++;
          }

          double factor = 1.0 / sqrt(sum);
          dim = dimension;

          data = array;

          while(--dim >= 0){
            *(data) = *(data) * factor;
            data++;
          }
        }

        void adjustScale(){
          if(!imgDbl){
            return ;
          }
          int n = featureStorage.size();
          for(int idx = 0; idx < n; idx++ ) {
            Feature* feat = featureStorage[idx];
            feat->x /= 2.0;
            feat->y /= 2.0;
            feat->scale /= 2.0;
          }
        }

      public:
        //constructor
        SiftFeatureManager():
            ignoreBorder(5),PI2(CV_PI * 2.0)
        {
          intervalPerOctave = 3;
          sigma0 = 1.6f;
          contrastThreshold = 0.03f;
          currentContrastThreshold = 0;
          eigenValRatioThreshold = 10.0f;
          curveThreshold = (eigenValRatioThreshold + 1) * 
            (eigenValRatioThreshold + 1) / eigenValRatioThreshold;

          dimOriHist = 36;
          oriSigmaFactor = 1.5;
          numOriHistSmooth = 2;
          oriPeakRatioThreshold = 0.8;

          numOfDivision = 4;
          binsPerHistogram = 8;
          dimOfDescriptor = binsPerHistogram * numOfDivision * numOfDivision;
          descrMagThreshold = 0.2;

          pyramid = NULL;
          dogs = NULL;

          imgDbl = true;
          currentIndex = 0;

          checkOrientation = new float[dimOriHist * 2];

          for(int i = 0; i < dimOriHist; i++){
            checkOrientation[2 * i] = cos(i * dimOriHist / PI2);
            checkOrientation[2 * i + 1] = sin(i * dimOriHist / PI2);
          }
        }

        //destructor
        ~SiftFeatureManager(){
          releaseImageBuffer();
          releaseFeature();
          delete[] this->checkOrientation;
        }

        //public methods
        void setNumOfIntervalsPerOctave(int s){
          assert(s > 1);
          this->intervalPerOctave = s;
        }

        int getNumOfIntervalsPerOctave() const{
          return intervalPerOctave;
        }

        float getBaseSigma() const {
          return sigma0;
        }

        void setBaseSigma(float sig){
          this->sigma0 = sig;
        }

        void setContrastThreshold(double contrastTh){
          assert(contrastTh >= 0.005);
          contrastThreshold = contrastTh;
        }

        double  getContrastThreshold() const{
          return contrastThreshold;
        }

        double getEigenValRatioThreshold() const{
          return eigenValRatioThreshold;
        }

        void setEigenValRatioThreshold(double ratio){
          assert(ratio > 5 && ratio < 20);
          eigenValRatioThreshold = ratio;
          curveThreshold = (ratio + 1) * (ratio + 1) / ratio;
        }

        int getDimensionOfOrientationHistogram() const{
          return dimOriHist;
        }

        double getOrientationSigmaFactor() const{
          return oriSigmaFactor;
        }

        int getNumOrientationHistogramSmoothing() const{
          return numOriHistSmooth;
        }

        double getOrientationPeakRatioThreshold() const{
          return oriPeakRatioThreshold;
        }

        int getNumOfDivisionOfDescriptor() const{
          return numOfDivision;
        }

        int getBinsPerHistogram() const{
          return binsPerHistogram;
        }

        int getDimOfDescriptor() const{
          return dimOfDescriptor;
        }

        double getDescriptorMagnitudeThreshold() const{
          return descrMagThreshold;
        }

        bool isImageDoublyMultiplyMode() const{
          return imgDbl;
        }

        void setImageDoublyMultiplyMode(bool mode){
          imgDbl = mode;
        }

        int computeFeature(IplImage* image){
          releaseFeature();
          allocateImgBuffer(image);
          convertGray32f(image, pyramid[0][0]);

          buildGaussPyramid();
          buildDoGPyramid();
          findKeyPoints();
          computeDescriptors();

          currentIndex = 0;

          return featureStorage.size();
        }

        bool hasNextFeature() const{
          if(currentIndex < featureStorage.size()){
            return true;
          } else{
            return false;
          }
        }

        SiftFeature* nextFeature(){
          SiftFeature* tmp = (SiftFeature*)featureStorage[currentIndex];
          currentIndex++;
          return tmp;
        }
      };

    }
  }
}

#endif
