// exception.cc 
//      Entry point into the Nachos kernel from user programs.
//      There are two kinds of things that can cause control to
//      transfer back to here from user code:
//
//      syscall -- The user code explicitly requests to call a procedure
//      in the Nachos kernel.  Right now, the only function we support is
//      "Halt".
//
//      exceptions -- The user code does something that the CPU can't handle.
//      For instance, accessing memory that doesn't exist, arithmetic errors,
//      etc.  
//
//      Interrupts (which can also cause control to transfer from user
//      code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "userthread.h"
#include "fork.h"

#ifdef CHANGED
int copyStringFromMachine(int from, char *to, unsigned size){
  int i;
  for(i=0; i<(int)size; i++){
    int val;
    machine->ReadMem(from,1,&val);
    to[i] = (char)val;
    from++;
    if(to[i]=='\0'){
      return i;
    }
  }
  to[i] = '\0';
  return size;
}

int copyStringToMachine(char *from, int to, unsigned size){
  int val;
  int i;
  for(i=0; i<(int)size; i++){
    val = from[i];
    machine->WriteMem(to,1,val);
    to++;
    if(from[i]=='\0'){
      return i;
    }
  }
  val = '\0';
  machine->WriteMem(to,1,val);
  return size;
}
#endif //CHANGED

//----------------------------------------------------------------------
// UpdatePC : Increments the Program Counter register in order to resume
// the user program immediately after the "syscall" instruction.
//----------------------------------------------------------------------
static void
UpdatePC ()
{
    int pc = machine->ReadRegister (PCReg);
    machine->WriteRegister (PrevPCReg, pc);
    pc = machine->ReadRegister (NextPCReg);
    machine->WriteRegister (PCReg, pc);
    pc += 4;
    machine->WriteRegister (NextPCReg, pc);
}


//----------------------------------------------------------------------
// ExceptionHandler
//      Entry point into the Nachos kernel.  Called when a user program
//      is executing, and either does a syscall, or generates an addressing
//      or arithmetic exception.
//
//      For system calls, the following is the calling convention:
//
//      system call code -- r2
//              arg1 -- r4
//              arg2 -- r5
//              arg3 -- r6
//              arg4 -- r7
//
//      The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//      "which" is the kind of exception.  The list of possible exceptions 
//      are in machine.h.
//----------------------------------------------------------------------

void
ExceptionHandler (ExceptionType which)
{
    int type = machine->ReadRegister (2);

    switch (which)
      {
	case SyscallException:
          {
	    switch (type)
	      {
		case SC_Halt:
		  {
		    /*while(currentThread->space->GetNbThread()>0){
		      currentThread->space->LockSem();
		      }*/
		    currentThread->Finish();
		    delete currentThread->space;
		    DEBUG('s',"Halt\n");
		    interrupt->Halt();
		    break;
		  }
	       case SC_Exit:
		{
		  DEBUG('s',"Exit\n");
		  AjustNbProc(-1);
		   while(currentThread->space->GetNbThread()>0){
		      currentThread->space->LockSem();
		   }
		  delete currentThread->space;
		  if(GetNbProc()>=0){
		    currentThread->Finish();
		  }
		  else{
		    interrupt->Halt ();
		  }
		  break;
		}
#ifdef CHANGED
	        case SC_PutChar:
		  {
		    DEBUG('s', "PutChar\n ");
		    synchconsole->SynchPutChar(machine->ReadRegister(4));
		    break;
		  }
		  
	        case SC_PutString:
		  {
		    DEBUG('s', "PutString\n ");
		    int read = machine->ReadRegister(4);
		    char *buffer = new char[MAX_STRING_SIZE];
		    int copy = MAX_STRING_SIZE;
		    while(copy == MAX_STRING_SIZE){
		      copy = copyStringFromMachine(read,buffer,MAX_STRING_SIZE);
		      synchconsole->SynchPutString(buffer);
		      read = read + copy;
		    }
		    delete [] buffer;
		    break;
		  }
	        case SC_GetChar:
	 	  {
		    DEBUG('s', "GetChar\n ");
		    machine->WriteRegister(2,synchconsole->SynchGetChar());
		    break;
		  }

	        case SC_GetString:
		  {
		    DEBUG('s', "GetString\n ");
		    int get = machine->ReadRegister(4);
		    int size = machine->ReadRegister(5);
		    char *buffer = new char[size];
		    int copy = MAX_STRING_SIZE;
		    while(copy == MAX_STRING_SIZE){
		      synchconsole->SynchGetString(buffer,size);
		      copy = copyStringToMachine(buffer,get,size);
		      get = get + copy;
		      size = size - copy;
		    }
		    delete [] buffer;
		    break;
		  }

	      case SC_PutInt:
	          {
		    DEBUG('s',"PutInt\n ");
		    int n = machine->ReadRegister(4);
		    synchconsole->SynchPutInt(n);
		    break;
	          }

	      case SC_GetInt:
		{
		  DEBUG('s',"GetInt\n ");
		  int n, get;
		  get = machine->ReadRegister(4);
		  synchconsole->SynchGetInt(&n);
		  machine->WriteMem(get, sizeof(int),n);
		  break;
		}

	      case SC_ThreadCreate:
		{
		  DEBUG('s',"ThreadCreate\n");
		  int f = machine->ReadRegister(4);
		  int arg = machine->ReadRegister(5);
		  int res = do_ThreadCreate(f,arg);
		  machine->WriteRegister(2,res);
		  break;
		}
		
	      case SC_ThreadExit:
		{
		  DEBUG('s',"ThreadExit\n");
		  if(currentThread->space->GetNbThread()==0){
		    AjustNbProc(-1);
		     if(GetNbProc()>=0){
		       currentThread->Finish();
		     }
		     else{
		       interrupt->Halt ();
		     }
		  }
		  do_ThreadExit();
		  break;
		}

	      case SC_ForkExec:
		{
		  DEBUG('s',"ForkExec\n");
		  char *buffer = new char[MAX_STRING_SIZE];
		  int s = machine->ReadRegister(4);
		  copyStringFromMachine(s,buffer,MAX_STRING_SIZE);
		  int res = ForkExec(buffer);
		  delete [] buffer;
		  break;
		}
		
#endif //CHANGED
		default:
		  {
#ifdef CHANGED
		    //printf("Unimplemented system call %d\n", type);
		    //ASSERT(FALSE);
		    DEBUG ('s', "Shutdown, initiated by user program.\n");
		    delete currentThread->space;
		   
		    interrupt->Halt ();
		    break;
#endif //CHANGED
		  }
	      }

	    // Do not forget to increment the pc before returning!
	    UpdatePC ();
	    break;
	  }

	case PageFaultException:
	  if (!type) {
	    printf("NULL dereference at PC %x!\n", machine->registers[PCReg]);
	    ASSERT (FALSE);
	  } else {
	    printf ("Page Fault at address %x at PC %x\n", type, machine->registers[PCReg]);
	    ASSERT (FALSE);	// For now
	  }
	  break;
	  
	default:
	  printf ("Unexpected user mode exception %d %d at PC %x\n", which, type, machine->registers[PCReg]);
	  ASSERT (FALSE);
      }
}