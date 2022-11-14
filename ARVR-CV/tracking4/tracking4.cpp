#include <stdio.h>
#include <ctype.h>
#include "cv.h"
#include "highgui.h"

void track_template(const CvArr* tmp, const CvArr* current,
    CvPoint2D32f template_feature, CvPoint2D32f *current_feature_p,
    double *ncc_array, double *maximum_p, double *minimum_p,
    CvSize template_size, int step);
void on_mouse(int event, int x, int y, int flags, void* parameters);

#define SELECTING_FIRST_CORNER 0
#define SELECTING_SECOND_CORNER 1
#define SELECTED_REGION 2
#define SEARCHING 3
#define THRESHOLD (0.5)
#define WINDOW_STEP 3

IplImage* output = 0;
int mode = SELECTING_FIRST_CORNER;
CvPoint mouse_position;
CvPoint origin;
CvRect selection;

int main(int argc, char** argv) {
  CvCapture* capture = 0;
  IplImage* input = 0;
  IplImage* gray = 0;
  IplImage* template_gray = 0;
  int win_step;
  int template_width, template_height;
  CvPoint2D32f template_point;
  CvPoint2D32f current_point;
  double maximum_value;
  double minimum_value;
  double *nccs;
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
    return -1;
  }

  input = cvQueryFrame(capture);
  if (!input) {
    fprintf(stderr, "Could not query frame...\n");
    return -1;
  }

  gray = cvCreateImage(cvGetSize(input), IPL_DEPTH_8U, 1);
  cvCvtColor(input, gray, CV_BGR2GRAY);
  template_gray = cvCreateImage(cvGetSize(input), 8, 1);

  output = cvCreateImage(cvSize(input->width, input->height), IPL_DEPTH_8U, 3);
  output->origin = input->origin;

  cvNamedWindow("Template", 0);
  cvMoveWindow("Template", 40, 20);
  cvResizeWindow("Template", 160, 120);

  cvNamedWindow("Tracking", CV_WINDOW_AUTOSIZE);
  cvSetMouseCallback("Tracking", on_mouse, 0);
  cvMoveWindow("Tracking", 200, 100);
  cvResizeWindow("Tracking", 640, 480);

  cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 1.0, 1.0, 0.0, 1, 0);

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

    if (mode == SEARCHING) {
      p1x = (int) current_point.x - template_width / 2 - 8 * win_step;
      p1y = (int) current_point.y - template_height / 2 - 8 * win_step;
      p2x = (int) current_point.x + template_width / 2 - 1 + 8 * win_step;
      p2y = (int) current_point.y + template_height / 2 - 1 + 8 * win_step;
      cvRectangle(output, cvPoint(p1x, p1y), cvPoint(p2x, p2y),
          CV_RGB(0, 0, 255), 1, // thickness, -1=filled
          8, // line_type, 8=8-connected
          0);
      track_template(template_gray, gray, template_point, &current_point, nccs,
          &maximum_value, &minimum_value,
          cvSize(template_width, template_height), win_step);
      p1x = (int) current_point.x - template_width / 2;
      p1y = (int) current_point.y - template_height / 2;
      p2x = (int) current_point.x + template_width / 2 - 1;
      p2y = (int) current_point.y + template_height / 2 - 1;
      sprintf(buffer, "%1.3lf", maximum_value);
      cvPutText(output, buffer, cvPoint(50, 350), &font, CV_RGB(255, 0, 255));
      if (maximum_value > THRESHOLD) {
        cvRectangle(output, cvPoint(p1x, p1y), cvPoint(p2x, p2y),
            CV_RGB(0, 255, 0), 1, // thickness, -1=filled
            8, // line_type, 8=8-connected
            0);
      } else {
        cvRectangle(output, cvPoint(p1x, p1y), cvPoint(p2x, p2y),
            CV_RGB(255, 0, 0), -1, // thickness, -1 = filled
            8, // line_type, 8=8-connected
            0);
      }
      p1x = (int) template_point.x - template_width / 2;
      p1y = (int) template_point.y - template_height / 2;
      p2x = (int) template_point.x + template_width / 2 - 1;
      p2y = (int) template_point.y + template_height / 2 - 1;
      cvRectangle(template_gray, cvPoint(p1x - 1, p1y - 1),
          cvPoint(p2x + 1, p2y + 1), CV_RGB(0, 255, 0), 1, // thickness, 1
          8, // line_type, 8=8-connected
          0);
      for (j = 0; j < 16; j++) {
        for (i = 0; i < 16; i++) {
          fprintf(stderr, "%d, %d, %lf\n", i, j, nccs[i + 16 * j]);
          cvRectangle(
              output,
              cvPoint(i * 10, j * 10),
              cvPoint((i + 1) * 10, (j + 1) * 10),
              CV_RGB(
                  255 * (nccs[i + 16 * j] - minimum_value)
                      / (maximum_value - minimum_value),
                  0,
                  255 * (nccs[i + 16 * j] - minimum_value)
                      / (maximum_value - minimum_value)), -1, // thickness, -1=filled
              8, // line_type, 8=8-connected
              0);
        }
      }
    } else if (mode == SELECTED_REGION) {
      window = selection;
      cvRectangle(output, cvPoint(window.x, window.y),
          cvPoint(window.x + window.width, window.y + window.height),
          CV_RGB(0, 255, 0), 1, 8, 0);
      cvCvtColor(input, template_gray, CV_BGR2GRAY);
      template_point = cvPointTo32f(
          cvPoint(window.x + window.width / 2, window.y + window.height / 2));
      template_width = window.width;
      template_height = window.height;
      nccs = (double *) malloc(16 * 16 * sizeof(double));
      current_point = cvPointTo32f(
          cvPoint(window.x + window.width / 2, window.y + window.height / 2));
      if (current_point.y <= template_height / 2 + 8 * win_step) {
        current_point.y = template_height / 2 + 8 * win_step;
      }
      if (current_point.y
          >= gray->height - template_height / 2 - 8 * win_step - 1) {
        current_point.y = gray->height - template_height / 2 - 8 * win_step - 1;
      }
      if (current_point.x <= template_width / 2 + 8 * win_step) {
        current_point.x = template_width / 2 + 8 * win_step;
      }
      if (current_point.x
          >= gray->width - template_width / 2 - 8 * win_step - 1) {
        current_point.x = gray->width - template_width / 2 - 8 * win_step - 1;
      }
      mode = SEARCHING;
    } else if (mode == SELECTING_SECOND_CORNER) {
      sprintf(buffer, "Select another corner!");
      cvPutText(output, buffer, cvPoint(150, 250), &font, CV_RGB(0, 255, 0));
    } else {
      sprintf(buffer, "Select a corner!");
      cvPutText(output, buffer, cvPoint(150, 250), &font, CV_RGB(0, 255, 0));
    }

    if ((mode == SELECTING_SECOND_CORNER) & (selection.width > 0)
        && (selection.height > 0)) {
      cvRectangle(
          output,
          cvPoint(selection.x, selection.y),
          cvPoint(selection.x + selection.width,
              selection.y + selection.height), CV_RGB(0, 0, 255), 1, 8, 0);
      cvSetImageROI(output, selection);
      cvXorS(output, cvScalarAll(255), output, 0);
      cvResetImageROI(output);
    }

    // draw cross 
    cvLine(output, cvPoint(mouse_position.x, 0),
        cvPoint(mouse_position.x, output->height - 1), CV_RGB(0, 255, 255), 1,
        8, 0);
    cvLine(output, cvPoint(0, mouse_position.y),
        cvPoint(output->width - 1, mouse_position.y), CV_RGB(0, 255, 255), 1, 8,
        0);

    sprintf(buffer, "%3.1lfms", now / 1000);
    cvPutText(output, buffer, cvPoint(50, 150), &font, CV_RGB(255, 0, 0));

    cvShowImage("Tracking", output);
    if (template_gray) {
      cvShowImage("Template", template_gray);
    }

    //If a certain key pressed
    if (cvWaitKey(10) >= 0) {
      break;
    }

    tick = cvGetTickCount();
    now = (tick - previous_tick) / cvGetTickFrequency();
    previous_tick = tick;
  }

  free(nccs);
  cvReleaseImage(&template_gray);
  cvReleaseImage(&gray);
  cvReleaseImage(&output);

  cvReleaseCapture(&capture);
  cvDestroyWindow("Tracking");

  return 0;
}

void track_template(const CvArr* tmp, const CvArr* current,
    CvPoint2D32f template_feature, CvPoint2D32f *current_feature_p,
    double *ncc_array, double *maximum_p, double *minimum_p,
    CvSize template_size, int step) {
  uchar *dataR, *dataS;
  int stepR, stepS;
  CvSize sizeR, sizeS;
  int u, v;
  int uu, vv;
  double sum_it;
  double sum_ii;
  double sum_tt;
  double ncc;
  double max_ncc;
  double min_ncc;
  int max_u, max_v;
  uchar *pR, *pS;
  uchar r, s;

  cvGetRawData(tmp, &dataR, &stepR, &sizeR);
  cvGetRawData(current, &dataS, &stepS, &sizeS);

  max_ncc = 0.0;
  min_ncc = 1.0;
  max_u = 8;
  max_v = 8;

  for (v = 0; v < 16; v++) {
    for (u = 0; u < 16; u++) {
      sum_tt = 0.0;
      sum_ii = 0.0;
      sum_it = 0.0;
      for (vv = 0; vv < template_size.height; vv += step) {
        for (uu = 0; uu < template_size.width; uu += step) {
          pR = dataR + (int) template_feature.x + uu - template_size.width / 2
              + stepR
                  * ((int) template_feature.y + vv - template_size.height / 2);
          r = *pR;
          pS = dataS + u * step + ((int) (*current_feature_p).x - 8 * step) + uu
              - template_size.width / 2
              + stepS
                  * (v * step + ((int) (*current_feature_p).y - 8 * step) + vv
                      - template_size.height / 2);
          s = *pS;

          sum_tt += r * r;
          sum_ii += s * s;
          sum_it += s * r;
        }
      }
      ncc = sum_it / sqrt(sum_ii * sum_tt);
      ncc_array[u + 16 * v] = ncc;
      if (ncc > max_ncc) {
        max_ncc = ncc;
        max_u = u;
        max_v = v;
      }
      if (ncc < min_ncc) {
        min_ncc = ncc;
      }
    }
  }
  *minimum_p = min_ncc;
  *maximum_p = max_ncc;
  (*current_feature_p).x = max_u * step
      + ((int) (*current_feature_p).x - 8 * step);
  (*current_feature_p).y = max_v * step
      + ((int) (*current_feature_p).y - 8 * step);
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
  fprintf(
      stderr,
      "x=%lf, y=%lf, min_ncc=%lf, max_ncc=%lf, thr=%lf, t_width=%d, t_height=%d, step=%d\n",
      (*current_feature_p).x, (*current_feature_p).y, min_ncc, max_ncc,
      THRESHOLD, template_size.width, template_size.height, step);
}

void on_mouse(int event, int x, int y, int flags, void* parameters) {
  if (!output) {
    return;
  }

  if (output->origin) {
    y = (output->height) - y;
  }

  if (mode == SELECTING_SECOND_CORNER) {
    selection.x = MIN(x, origin.x);
    selection.y = MIN(y, origin.y);
    selection.width = selection.x + CV_IABS(x - origin.x);
    selection.height = selection.y + CV_IABS(y - origin.y);

    selection.x = MAX(selection.x, 0);
    selection.y = MAX(selection.y, 0);
    selection.width = MIN(selection.width, output->width);
    selection.height = MIN(selection.height, output->height);
    selection.width = selection.width - selection.x;
    selection.height = selection.height - selection.y;
  }

  switch (event) {
  case CV_EVENT_MOUSEMOVE:
    mouse_position = cvPoint(x, y);
    break;
  case CV_EVENT_LBUTTONDOWN:
    if (mode == SEARCHING) {
      mode = SELECTING_FIRST_CORNER;
    }
    origin = cvPoint(x, y);
    selection = cvRect(x, y, 0, 0);
    mode = SELECTING_SECOND_CORNER;
    break;
  case CV_EVENT_LBUTTONUP:
    if ((selection.width > 0) && (selection.height > 0)) {
      mode = SELECTED_REGION;
    } else {
      mode = SELECTING_FIRST_CORNER;
    }
    break;
  }
}

