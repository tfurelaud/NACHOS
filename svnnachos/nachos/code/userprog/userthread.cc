#ifdef CHANGED

#include "thread.h"
#include "system.h"
#include "userthread.h"
#include "utility.h"
#include "addrspace.h"


struct ThreadArguments{
  int func;
  int argv;
};

int NbThreads = 1;
Semaphore *lockThread = new Semaphore("lock thread",1);

static void StartUserThread(void *schmurtz){
  ThreadArguments* args = (ThreadArguments *)schmurtz;

  int f = args->func;
  int arg = args->argv;

  for(int i=0; i<NumTotalRegs;i++){
    machine->WriteRegister(i,0);
  }
  machine->WriteRegister(PCReg,f);
  machine->WriteRegister(4,arg);
  machine->WriteRegister(NextPCReg, f+4);
  DEBUG('x', "NextPCReg %d\n",machine->ReadRegister(PCReg)+4);
  DEBUG('x', "function args %d\n", machine->ReadRegister(PCReg));
  DEBUG('x', "argv args %d\n", machine->ReadRegister(4));
  machine->WriteRegister(StackReg,currentThread->space->AllocateUserStack());
  machine->Run();
}

int do_ThreadCreate(int f, int arg){
  Thread* t = new Thread("ThreadCreate");

  if(t==NULL){
    printf("Thread non created\n");
    return -1;
  }
  ThreadArguments* args = new ThreadArguments;
  args->func = f;
  args->argv = arg;
  lockThread->P();
  currentThread->space->SetNbThread();
  lockThread->V();
  t->Start(StartUserThread,args);
  return 0;
}

void do_ThreadExit(){
  currentThread->space->FreeSem();
  currentThread->space->FreeUserStack();
  currentThread->Finish();
}


#endif //CHANGED
