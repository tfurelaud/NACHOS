#ifndef PAGEPROVIDER_H
#define PAGEPROVIDER_H

#include "bitmap.h"
#include <stdio.h>
#include <string.h>

class PageProvider
{
 public:
  PageProvider();
  ~PageProvider();

  int GetEmptyPage();
  void ReleasePage(int numPage);
  int NumAvailPage();

 private:
  BitMap *bitmap;

};

#endif // PAGEPROVIDER_H
