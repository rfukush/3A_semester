#include <stdio.h>
#include <ctype.h>
#include "cv.h"
#include "highgui.h"

void search_template(const CvArr* previous, const CvArr* current,
    CvPoint2D32f previous_feature, CvPoint2D32f *current_feature_p,
    double *zncc_array, double *maximum_p, double *minimum_p,
    CvSize template_size, int step);

#define THRESHOLD (0.5)
#define WINDOW_STEP 3

int main(int argc, char** argv) {
  CvCapture* capture = 0;
  IplImage* input = 0;
  IplImage* gray = 0;
  IplImage* output = 0;
  IplImage* previous_gray = 0;
  int win_step;
  int template_width, template_height;
  CvPoint2D32f previous_point;
  CvPoint2D32f current_points[6][5];
  double maximum_value;
  double minimum_value;
  double *znccs;
  int p1x, p1y, p2x, p2y;
  CvRect window;
  int tick = 0, previous_tick = 0;
  double now = 0.0;
  CvFont font;
  char buffer[256];
  int i, j;

  if (argc == 1 || (argc == 2 && strlen(argv[1]) == 1 && isdigit(argv[1][0]))) {
    capture = cvCreateCameraCapture(argc == 2 ? argv[1][0] - '0' : 0);
  } else if (argc == 2) {
    capture = cvCreateFileCapture(argv[1]);
  }
  if (!capture) {
    fprintf(stderr, "ERROR: capture is NULL \n");
    return (-1);
  }

  input = cvQueryFrame(capture);
  if (!input) {
    fprintf(stderr, "Could not query frame...\n");
    return (-1);
  }

  gray = cvCreateImage(cvGetSize(input), IPL_DEPTH_8U, 1);
  cvCvtColor(input, gray, CV_BGR2GRAY);
  previous_gray = cvCreateImage(cvGetSize(input), 8, 1);

  output = cvCreateImage(cvSize(input->width, input->height), IPL_DEPTH_8U, 3);
  output->origin = input->origin;

  cvNamedWindow("Optical Flow", CV_WINDOW_AUTOSIZE);
  cvMoveWindow("Optical Flow", 200, 100);
  cvResizeWindow("Optical Flow", 640, 480);

  cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 1.0, 1.0, 0.0, 1, 0);

  znccs = (double *) malloc(16 * 16 * sizeof(double));

  template_width = 32;
  template_height = 32;
  win_step = WINDOW_STEP;

  for (;;) {
    input = cvQueryFrame(capture);
    if (!input) {
      fprintf(stderr, "Could not query frame...\n");
      break;
    }
    cvCopy(input, output, NULL);
    cvCvtColor(input, gray, CV_BGR2GRAY);

    for (j = 0; j < 5; j++) {
      for (i = 0; i < 6; i++) {
        previous_point.x = template_width / 2 + 8 * win_step
            + i * (8 * 2 * 3 + 32) + 10;
        previous_point.y = template_height / 2 + 8 * win_step
            + j * (8 * 2 * 3 + 32) + 10;
        p1x = (int) previous_point.x - template_width / 2 - 8 * win_step;
        p1y = (int) previous_point.y - template_height / 2 - 8 * win_step;
        p2x = (int) previous_point.x + template_width / 2 - 1 + 8 * win_step;
        p2y = (int) previous_point.y + template_height / 2 - 1 + 8 * win_step;
        cvRectangle(output, cvPoint(p1x, p1y), cvPoint(p2x, p2y),
            CV_RGB(0, 0, 255), 1, // thickness, -1=filled
            8, // line_type, 8=8-connected
            0);
        search_template(previous_gray, gray, previous_point,
            &current_points[i][j], znccs, &maximum_value, &minimum_value,
            cvSize(template_width, template_height), win_step);
        cvLine(output, cvPoint(previous_point.x, previous_point.y),
            cvPoint(current_points[i][j].x, current_points[i][j].y),
            CV_RGB(255, 0, 0), 1, 8, 0);
        p1x = (int) current_points[i][j].x - template_width / 2;
        p1y = (int) current_points[i][j].y - template_height / 2;
        p2x = (int) current_points[i][j].x + template_width / 2 - 1;
        p2y = (int) current_points[i][j].y + template_height / 2 - 1;
        if (maximum_value > THRESHOLD) {
          cvRectangle(output, cvPoint(p1x, p1y), cvPoint(p2x, p2y),
              CV_RGB(0, 255, 0), 1, // thickness, 1
              8, // line_type, 8=8-connected
              0);
        } else {
          cvRectangle(output, cvPoint(p1x, p1y), cvPoint(p2x, p2y),
              CV_RGB(255, 0, 0), 1, // thickness, 1
              8, // line_type, 8=8-connected
              0);
        }
      }
    }
    cvCopy(gray, previous_gray, NULL);

    sprintf(buffer, "%3.1lfms", now / 1000);
    cvPutText(output, buffer, cvPoint(50, 150), &font, CV_RGB(255, 0, 0));

    cvShowImage("Optical Flow", output);

    //If a certain key pressed
    if (cvWaitKey(10) >= 0) {
      break;
    }
    tick = cvGetTickCount();
    now = (tick - previous_tick) / cvGetTickFrequency();
    previous_tick = tick;
  }

  free(znccs);

  cvReleaseImage(&previous_gray);
  cvReleaseImage(&gray);
  cvReleaseImage(&output);

  cvReleaseCapture(&capture);
  cvDestroyWindow("Previous");
  cvDestroyWindow("Optical Flow");

  return (0);
}

void search_template(const CvArr* previous, const CvArr* current,
    CvPoint2D32f previous_feature, CvPoint2D32f *current_feature_p,
    double *zncc_array, double *maximum_p, double *minimum_p,
    CvSize template_size, int step) {
  uchar *dataR, *dataS;
  int stepR, stepS;
  CvSize sizeR, sizeS;
  int u, v;
  int uu, vv;
  double sum_i;
  double sum_t;
  double avg_i;
  double avg_t;
  double sum_iimttm;
  double sum_iim;
  double sum_ttm;
  double zncc;
  double max_zncc;
  double min_zncc;
  int max_u, max_v;
  uchar *pR, *pS;
  uchar r, s;

  cvGetRawData(previous, &dataR, &stepR, &sizeR);
  cvGetRawData(current, &dataS, &stepS, &sizeS);

  max_zncc = 0.0;
  min_zncc = 1.0;
  max_u = 8;
  max_v = 8;

  for (v = 0; v < 16; v++) {
    for (u = 0; u < 16; u++) {
      sum_i = 0.0;
      sum_t = 0.0;
      for (vv = 0; vv < template_size.height; vv += step) {
        for (uu = 0; uu < template_size.width; uu += step) {
          pR = dataR + (int) previous_feature.x + uu - template_size.width / 2
              + stepR
                  * ((int) previous_feature.y + vv - template_size.height / 2);
          r = *pR;
          pS = dataS + u * step + ((int) previous_feature.x - 8 * step) + uu
              - template_size.width / 2
              + stepS
                  * (v * step + ((int) previous_feature.y - 8 * step) + vv
                      - template_size.height / 2);
          s = *pS;
          sum_t += r;
          sum_i += s;
        }
      }
      avg_t = sum_t * step * step
          / (double) (template_size.height * template_size.width);
      avg_i = sum_i * step * step
          / (double) (template_size.height * template_size.width);
      sum_iim = 0.0;
      sum_ttm = 0.0;
      sum_iimttm = 0.0;
      for (vv = 0; vv < template_size.height; vv += step) {
        for (uu = 0; uu < template_size.width; uu += step) {
          pR = dataR + (int) previous_feature.x + uu - template_size.width / 2
              + stepR
                  * ((int) previous_feature.y + vv - template_size.height / 2);
          r = *pR;
          pS = dataS + u * step + ((int) previous_feature.x - 8 * step) + uu
              - template_size.width / 2
              + stepS
                  * (v * step + ((int) previous_feature.y - 8 * step) + vv
                      - template_size.height / 2);
          s = *pS;
          sum_iimttm += (s - avg_i) * (r - avg_t);
          sum_iim += (s - avg_i) * (s - avg_i);
          sum_ttm += (r - avg_t) * (r - avg_t);
        }
      }
      zncc = sum_iimttm / sqrt(sum_iim * sum_ttm);
      zncc_array[u + 16 * v] = zncc;
      if (zncc > max_zncc) {
        max_zncc = zncc;
        max_u = u;
        max_v = v;
      }
      if (zncc < min_zncc) {
        min_zncc = zncc;
      }
    }
  }
  *minimum_p = min_zncc;
  *maximum_p = max_zncc;
  (*current_feature_p).x = max_u * step + ((int) previous_feature.x - 8 * step);
  (*current_feature_p).y = max_v * step + ((int) previous_feature.y - 8 * step);

  if ((*current_feature_p).y <= template_size.height / 2 + 8 * step) {
    (*current_feature_p).y = template_size.height / 2 + 8 * step;
  }
  if ((*current_feature_p).y
      >= sizeS.height - template_size.height / 2 - 8 * step - 1) {
    (*current_feature_p).y = sizeS.height - template_size.height / 2 - 8 * step
        - 1;
  }
  if ((*current_feature_p).x <= template_size.width / 2 + 8 * step) {
    (*current_feature_p).x = template_size.width / 2 + 8 * step;
  }
  if ((*current_feature_p).x
      >= sizeS.width - template_size.width / 2 - 8 * step - 1) {
    (*current_feature_p).x = sizeS.width - template_size.width / 2 - 8 * step
        - 1;
  }

  /*
   fprintf(stderr,
   "x=%lf, y=%lf, min_zncc=%lf, max_zncc=%lf,
   thr=%lf, t_width=%d, t_height=%d, step=%d\n",
   (*current_feature_p).x, (*current_feature_p).y,
   min_zncc, max_zncc,
   THRESHOLD,
   template_size.width, template_size.height, step);
   */
}

