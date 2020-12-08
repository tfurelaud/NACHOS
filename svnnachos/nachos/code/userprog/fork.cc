#ifdef CHANGED

#include "fork.h"
#include "thread.h"
#include "addrspace.h"
#include "system.h"

static void StartUserProc(void *arg){
  currentThread->space->InitRegisters();
  currentThread->space->RestoreState();
  currentThread->space->AllocateUserStack();

  machine->Run();
}

int ForkExec(char *s){
  
  OpenFile *exec = fileSystem->Open(s);
  
  AddrSpace *space;

  if (exec == NULL){
      SetColor (stdout, ColorRed);
      SetBold (stdout);
      printf ("Unable to open file %s\n", s);
      ClearColor (stdout);
      return 0;
  }

  
  space = new AddrSpace(exec);

  Thread *thread = new Thread("Processus");

  AjustNbProc(1);
  thread->space = space;
  delete exec;

  void* arg;
  thread->Start(StartUserProc,arg);
  thread->space->AllocateUserStack();
  //currentThread->Yield();

  return 0;
}



#endif //CHANGED 
