#ifdef CHANGED

#ifndef SYNCHCONSOLE_H
#define SYNCHCONSOLE_H

#include "copyright.h"
#include "utility.h"
#include "console.h"
#include "system.h"
#include "synch.h"

class SynchConsole:dontcopythis {
 public:
  SynchConsole(const char *readFile, const char *writeFile); // initialize the hardware console device
  ~SynchConsole();                                           // clean up console emulation
  void SynchPutChar(int ch);                                 // Unix getchar(3S)
  int SynchGetChar();                                        // Unix getchar(3S)
  void SynchPutString(const char *s);                        // Unix fputs(3S)
  void SynchGetString(char *s, int n);                       // Unix fgets(3S)
  void SynchGetInt(int *n);
  void SynchPutInt(int n);
  
private:
  Console *console;
  Machine *machine;
  Semaphore *ecritureChar;
  Semaphore *ecritureString;
  Semaphore *lectureChar;
  Semaphore *lectureString;
};

#endif // SYNCHCONSOLE_H

#endif // CHANGED
