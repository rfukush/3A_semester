#include <stdlib.h>
#include <sys/time.h>

//-------------------------------------------------------------
// 紀元からの秒数を返す(マイクロ秒まで)
//-------------------------------------------------------------
double get_clock_now(){
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec + (double)tv.tv_usec*1e-6;
}
