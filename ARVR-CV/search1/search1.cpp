#include <stdio.h>
#include <ctype.h>
#include "cv.h"
#include "highgui.h"

void search_template(const CvArr* tmp, const CvArr* curr,
    const CvPoint2D32f template_feature, CvPoint2D32f* curr_feature_p,
    CvSize halfsize, int step, char *status_p);
void on_mouse(int event, int x, int y, int flags, void* parameters);

#define SELECTING 0
#define SELECTED 1
#define SEARCHING 2
#define GOOD 1
#define BAD 0
#define WINDOW_STEP 3
#define WINDOW_HALFSIZE 8
#define THRESHOLD 5000

IplImage* gray = 0;
int mode = SELECTING;
CvPoint mouse_position;

int main(int argc, char** argv) {
  CvCapture* capture = 0;
  IplImage* input = 0;
  IplImage* gray = 0;
  IplImage* template_gray = 0;
  int win_halfsize;
  int win_step;
  CvPoint2D32f template_point;
  CvPoint2D32f current_point;
  char state;
  int p1x, p1y, p2x, p2y;
  int tick = 0, previous_tick = 0;
  double now = 0.0;
  CvFont font;
  char buffer[256];

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
  gray->origin = input->origin;
  cvCvtColor(input, gray, CV_BGR2GRAY);
  template_gray = cvCreateImage(cvGetSize(input), 8, 1);

  cvNamedWindow("Template", CV_WINDOW_AUTOSIZE);
  cvMoveWindow("Template", 40, 20);
  cvResizeWindow("Template", 160, 120);

  cvNamedWindow("Search", CV_WINDOW_AUTOSIZE);
  cvSetMouseCallback("Search", on_mouse, 0);
  cvMoveWindow("Search", 400, 200);
  cvResizeWindow("Search", 640, 480);

  cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 1.0, 1.0, 0.0, 1, 0);

  win_halfsize = WINDOW_HALFSIZE;
  win_step = WINDOW_STEP;

  for (;;) {
    input = cvQueryFrame(capture);
    if (!input) {
      fprintf(stderr, "Could not query frame...\n");
      break;
    }

    cvCvtColor(input, gray, CV_BGR2GRAY);

    if (mode == SEARCHING) {
      search_template(template_gray, gray, template_point, &current_point,
          cvSize(win_halfsize, win_halfsize), win_step, &state);
      p1x = (int) current_point.x - win_halfsize * win_step;
      p1y = (int) current_point.y - win_halfsize * win_step;
      p2x = (int) current_point.x + win_halfsize * win_step - 1;
      p2y = (int) current_point.y + win_halfsize * win_step - 1;
      if (state == GOOD) {
        cvRectangle(gray, cvPoint(p1x, p1y), cvPoint(p2x, p2y),
            CV_RGB(0, 255, 0), 1, // thickness, 1
            8, // line_type, 8=8-connected
            0);
      } else {
        cvRectangle(gray, cvPoint(p1x, p1y), cvPoint(p2x, p2y),
            CV_RGB(255, 0, 0), -1, // thickness, -1 = filled
            8, // line_type, 8=8-connected
            0);
      }
      p1x = (int) template_point.x - win_halfsize * win_step;
      p1y = (int) template_point.y - win_halfsize * win_step;
      p2x = (int) template_point.x + win_halfsize * win_step - 1;
      p2y = (int) template_point.y + win_halfsize * win_step - 1;
      cvRectangle(template_gray, cvPoint(p1x - 1, p1y - 1),
          cvPoint(p2x + 1, p2y + 1), CV_RGB(0, 255, 0), 1, // thickness, 1
          8, // line_type, 8=8-connected
          0);
    } else if (mode == SELECTED) {
      cvCvtColor(input, template_gray, CV_BGR2GRAY);
      template_point = cvPointTo32f(
          cvPoint(mouse_position.x, mouse_position.y));
      mode = SEARCHING;
    } else {
      // draw rectangle
      p1x = (int) mouse_position.x - win_halfsize * win_step;
      p1y = (int) mouse_position.y - win_halfsize * win_step;
      p2x = (int) mouse_position.x + win_halfsize * win_step - 1;
      p2y = (int) mouse_position.y + win_halfsize * win_step - 1;
      cvRectangle(gray, cvPoint(p1x, p1y), cvPoint(p2x, p2y), CV_RGB(0, 0, 255),
          1, // thickness, 1
          8, // line_type, 8=8-connected
          0);
      sprintf(buffer, "Select a region!");
      cvPutText(gray, buffer, cvPoint(150, 250), &font, CV_RGB(0, 255, 0));
    }

    // draw cross
    cvLine(gray, cvPoint(mouse_position.x, 0),
        cvPoint(mouse_position.x, gray->height - 1), CV_RGB(0, 255, 255), 1, 8,
        0);
    cvLine(gray, cvPoint(0, mouse_position.y),
        cvPoint(gray->width - 1, mouse_position.y), CV_RGB(0, 255, 255), 1, 8,
        0);

    sprintf(buffer, "%3.1lfms", now / 1000);
    cvPutText(gray, buffer, cvPoint(50, 150), &font, CV_RGB(255, 0, 0));

    cvShowImage("Search", gray);
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

  cvReleaseImage(&template_gray);
  cvReleaseImage(&gray);

  cvReleaseCapture(&capture);
  cvDestroyWindow("Search");

  return 0;
}

void search_template(const CvArr* tmp, const CvArr* curr,
    const CvPoint2D32f template_feature, CvPoint2D32f* curr_feature_p,
    CvSize halfsize, int step, char *status_p) {
  uchar *dataR, *dataS;
  int stepR, stepS;
  CvSize sizeR, sizeS;
  int u, v;
  int uu, vv;
  unsigned int sum;
  unsigned int absdiff;
  unsigned int min_sum;
  int min_u, min_v;
  uchar *pR, *pS;
  uchar r, s;

  cvGetRawData(tmp, &dataR, &stepR, &sizeR);
  cvGetRawData(curr, &dataS, &stepS, &sizeS);

  min_sum = 255 * (halfsize.height * 2) * (halfsize.width * 2) + 1;
  min_u = (int) template_feature.x;
  min_v = (int) template_feature.y;
  *status_p = BAD;

  for (v = halfsize.height * step; v < sizeS.height - halfsize.height * step;
      v += step * 2) {
    for (u = halfsize.width * step; u < sizeS.width - halfsize.width * step;
        u += step * 2) {
      sum = 0;
      for (vv = -halfsize.height * step; vv < halfsize.height * step;
          vv += step) {
        for (uu = -halfsize.width * step; uu < halfsize.width * step;
            uu += step) {
          // r...get pixel of(template_f.x+uu, template_f.y+vv)
          //   from tmp frame
          pR = dataR + (int) template_feature.x + uu
              + stepR * ((int) template_feature.y + vv);
          r = *pR;

          // S...get pixel of(u+uu, v+vv) from current frame
          pS = dataS + u + uu + stepS * (v + vv);
          s = *pS;

          if (s > r) {
            absdiff = (int) (s - r);
          } else {
            absdiff = (int) (r - s);
          }
          sum = sum + absdiff;
        }
      }

      if (sum < min_sum) {
        min_sum = sum;
        min_u = u;
        min_v = v;
      }
    }
  }

  if (min_sum < THRESHOLD) {
    *status_p = GOOD;
  }
  (*curr_feature_p).x = min_u;
  (*curr_feature_p).y = min_v;
  fprintf(stderr, "x=%lf, y=%lf, min_sum=%d\n", (*curr_feature_p).x,
      (*curr_feature_p).y, min_sum);
}

void on_mouse(int event, int x, int y, int flags, void* parameters) {
  switch (event) {
  case CV_EVENT_MOUSEMOVE:
    mouse_position = cvPoint(x, y);
    break;
  case CV_EVENT_LBUTTONDOWN:
    mouse_position = cvPoint(x, y);
    break;
  case CV_EVENT_LBUTTONUP:
    mode = SELECTED;
    break;
  }
}

