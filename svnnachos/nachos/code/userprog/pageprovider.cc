#include "pageprovider.h"
#include "machine.h"
#include "system.h"
#include <stdio.h>
#include <string.h>

PageProvider::PageProvider(){
  bitmap = new BitMap(NumPhysPages);
}

PageProvider::~PageProvider(){
  delete bitmap;
}

int PageProvider::GetEmptyPage(){
  int res = bitmap->Find();
  bitmap->Mark(res);
  memset(&machine->mainMemory[res*PageSize], 0, PageSize);
  return res;
}

void PageProvider::ReleasePage(int numPage){
  bitmap->Clear(numPage);
}

int PageProvider::NumAvailPage(){
  return bitmap->NumClear();
}
