#ifndef __CLOCK_H__
#define __CLOCK_H__

#ifdef __cplusplus
extern "C" {
#endif

//-------------------------------------------------------------
// 紀元からの秒数を返す(マイクロ秒まで)
//-------------------------------------------------------------
double get_clock_now();
  
#ifdef __cplusplus
}
#endif

#endif
