#include "syscall.h"

int main(){
  int n = 10;
  int n2 = 15153165748;//Ne fonctionne pas (fonctionne avec un chiffre de moins)
  int n3 = 'a';

  PutInt(n);
  PutInt(n2);
  PutInt(n3);
  return 0;

}
