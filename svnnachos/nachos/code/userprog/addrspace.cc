// addrspace.cc
//      Routines to manage address spaces (executing user programs).
//
//      In order to run a user program, you must:
//
//      1. link with the -N -T 0 option
//      2. run coff2noff to convert the object file to Nachos format
//              (Nachos object code format is essentially just a simpler
//              version of the UNIX executable object code format)
//      3. load the NOFF file into the Nachos file system
//              (if you haven't implemented the file system yet, you
//              don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"
#include "syscall.h"
#include "new"
class Semaphore;
//----------------------------------------------------------------------
// SwapHeader
//      Do little endian to big endian conversion on the bytes in the
//      object file header, in case the file was generated on a little
//      endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void
SwapHeader (NoffHeader * noffH)
{
    noffH->noffMagic = WordToHost (noffH->noffMagic);
    noffH->code.size = WordToHost (noffH->code.size);
    noffH->code.virtualAddr = WordToHost (noffH->code.virtualAddr);
    noffH->code.inFileAddr = WordToHost (noffH->code.inFileAddr);
    noffH->initData.size = WordToHost (noffH->initData.size);
    noffH->initData.virtualAddr = WordToHost (noffH->initData.virtualAddr);
    noffH->initData.inFileAddr = WordToHost (noffH->initData.inFileAddr);
    noffH->uninitData.size = WordToHost (noffH->uninitData.size);
    noffH->uninitData.virtualAddr =
	WordToHost (noffH->uninitData.virtualAddr);
    noffH->uninitData.inFileAddr = WordToHost (noffH->uninitData.inFileAddr);
}

#ifdef CHANGED

static void ReadAtVirtual(OpenFile *exec, int virtualaddr, int numBytes, int position, TranslationEntry *pageTable, unsigned numPages){
  char buf[numBytes];
  int bit = exec->ReadAt(buf,numBytes,position);

  TranslationEntry *saveTable = machine->pageTable;
  unsigned saveNumPages = machine->pageTableSize;

  machine->pageTable = pageTable;
  machine->pageTableSize = numPages;

  for(int i=0;i<bit;i++){
    machine->WriteMem(virtualaddr+i,1,buf[i]);
  }

  machine->pageTable = saveTable;
  machine->pageTableSize = saveNumPages;
}

#endif //CHANGED

//----------------------------------------------------------------------
// AddrSpaceList
//      List of all address spaces, for debugging
//----------------------------------------------------------------------
List AddrSpaceList;

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
//      Create an address space to run a user program.
//      Load the program from a file "executable", and set everything
//      up so that we can start executing user instructions.
//
//      Assumes that the object code file is in NOFF format.
//
//      First, set up the translation from program memory to physical
//      memory.  For now, this is really simple (1:1), since we are
//      only uniprogramming, and we have a single unsegmented page table
//
//      "executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

AddrSpace::AddrSpace (OpenFile * executable)
{
    unsigned int i, size;

    executable->ReadAt (&noffH, sizeof (noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) &&
	(WordToHost (noffH.noffMagic) == NOFFMAGIC))
	SwapHeader (&noffH);
    /* Check that this is really a MIPS program */
    ASSERT (noffH.noffMagic == NOFFMAGIC);

// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size + UserStacksAreaSize;	// we need to increase the size
    // to leave room for the stack
    numPages = divRoundUp (size, PageSize);
    size = numPages * PageSize;

    // check we're not trying
    // to run anything too big --
    // at least until we have
    // virtual memory
    if (numPages > NumPhysPages)
	    throw std::bad_alloc();

    DEBUG ('a', "Initializing address space, num pages %d, total size 0x%x\n",
	   numPages, size);
// first, set up the translation
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++)
      {
	#ifdef CHANGED
	  pageTable[i].virtualPage = i;
	  pageTable[i].physicalPage = page_provider->GetEmptyPage();	// for now, phys page # = virtual page #
        #endif //CHANGED
	  pageTable[i].valid = TRUE;
	  pageTable[i].use = FALSE;
	  pageTable[i].dirty = FALSE;
	  pageTable[i].readOnly = FALSE;	// if the code segment was entirely on
	  // a separate page, we could set its
	  // pages to be read-only
      }
    #ifdef CHANGED
    nbThread=0;
    bitmapMemory = new BitMap(1024/(2*PageSize)); //1024 -> UserStacksAreaSize
                                                // 2:Page per threads (page = 128 octets)
    bitmapMemory->Mark(0);
    wait_memory = new Semaphore("Wait memory",0);
    lockSem = new Semaphore("Lock Semaphore",0);
    Sem = new Semaphore("Semaphore lambda",1);
#endif //CHANGED

// then, copy in the code and data segments into memory
    if (noffH.code.size > 0)
      {
	  DEBUG ('a', "Initializing code segment, at 0x%x, size 0x%x\n",
		 noffH.code.virtualAddr, noffH.code.size);

	  ReadAtVirtual(executable,noffH.code.virtualAddr,noffH.code.size,noffH.code.inFileAddr,pageTable,numPages);
	  //executable->ReadAt (&(machine->mainMemory[noffH.code.virtualAddr]), noffH.code.size, noffH.code.inFileAddr);
      }
    if (noffH.initData.size > 0)
      {
	  DEBUG ('a', "Initializing data segment, at 0x%x, size 0x%x\n",
		 noffH.initData.virtualAddr, noffH.initData.size);
	  ReadAtVirtual(executable,noffH.initData.virtualAddr,noffH.initData.size,noffH.initData.inFileAddr,pageTable,numPages);


	  //executable->ReadAt (&(machine->mainMemory[noffH.initData.virtualAddr]),noffH.initData.size, noffH.initData.inFileAddr);
      }

    DEBUG ('a', "Area for stacks at 0x%x, size 0x%x\n",
	   size - UserStacksAreaSize, UserStacksAreaSize);

    pageTable[0].valid = FALSE;			// Catch NULL dereference

    AddrSpaceList.Append(this);
}
//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
//      Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

AddrSpace::~AddrSpace ()
{
  // LB: Missing [] for delete
  // delete pageTable;
  #ifdef CHANGED

  for(int i = 0; i < numPages; i++)
    page_provider->ReleasePage(pageTable[i].physicalPage);

  #endif //CHANGED

  delete [] pageTable;
  // End of modification

  AddrSpaceList.Remove(this);
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
//      Set the initial values for the user-level register set.
//
//      We write these directly into the "machine" registers, so
//      that we can immediately jump to user code.  Note that these
//      will be saved/restored into the currentThread->userRegisters
//      when this thread is context switched out.
//----------------------------------------------------------------------


#ifdef CHANGED
int AddrSpace::AllocateUserStack(){
  int memory;
  while( (memory = bitmapMemory->Find()) == -1){
      wait_memory->P();
  }
  currentThread->SetId(memory);
  return numPages*PageSize - 256*memory;
}

void AddrSpace::LockSem(){
  lockSem->P();
}

void AddrSpace::FreeSem(){
  lockSem->V();
}

void AddrSpace::FreeUserStack(){
  Sem->P();
  nbThread--;
  int id = currentThread->GetId();
  bitmapMemory->Clear(currentThread->GetId());
  DEBUG('x',"Free -> NbThread : %d\n",nbThread);
  wait_memory->V();
  lockSem->V();
  Sem->V();
}

int AddrSpace::GetNbThread(){
  return nbThread;
}

void AddrSpace::SetNbThread(){
  Sem->P();
  nbThread++;
  Sem->V();
  DEBUG('x',"NbThread set to : %d\n",nbThread);
}

#endif //CHANGED


void
AddrSpace::InitRegisters ()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister (i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister (PCReg, USER_START_ADDRESS);

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister (NextPCReg, machine->ReadRegister(PCReg) + 4);

    // Set the stack register to the end of the address space, where we
    // allocated the stack; but subtract off a bit, to make sure we don't
    // accidentally reference off the end!
    machine->WriteRegister (StackReg, numPages * PageSize - 16);
    DEBUG ('a', "Initializing stack register to 0x%x\n",
	   numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::Dump
//      Dump program layout as SVG
//----------------------------------------------------------------------

static void
DrawArea(FILE *output, unsigned x, unsigned virtual_x,
	 unsigned y, unsigned blocksize,
	 struct segment *segment, const char *name)
{
    if (segment->size == 0)
	return;

    ASSERT((segment->virtualAddr % PageSize == 0));
    ASSERT((segment->size % PageSize == 0));
    unsigned page = segment->virtualAddr / PageSize;
    unsigned end = (segment->virtualAddr + segment->size) / PageSize;

    fprintf(output, "<rect x=\"%u\" y=\"%u\" width=\"%u\" height=\"%u\" "
		    "fill=\"#ffffff\" "
		    "stroke=\"#000000\" stroke-width=\"1\"/>\n",
		    x, y - end * blocksize,
		    virtual_x - x, (end - page) * blocksize);

    fprintf(output, "<text x=\"%u\" y=\"%u\" fill=\"#000000\" font-size=\"%u\">%s</text>\n",
	    x, y - page * blocksize, blocksize, name);
}

unsigned
AddrSpace::Dump(FILE *output, unsigned virtual_x, unsigned virtual_width,
		unsigned physical_x, unsigned virtual_y, unsigned y,
		unsigned blocksize)
{
    unsigned ret = machine->DumpPageTable(output, pageTable, numPages,
	    virtual_x, virtual_width, physical_x, virtual_y, y, blocksize);

    DrawArea(output, 0, virtual_x, virtual_y, blocksize, &noffH.code, "code");
    DrawArea(output, 0, virtual_x, virtual_y, blocksize, &noffH.initData, "data");
    DrawArea(output, 0, virtual_x, virtual_y, blocksize, &noffH.uninitData, "bss");

    DumpThreadsState(output, this, virtual_x, virtual_y, blocksize);

    return ret;
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpacesRoom
//      Return how much room is needed for showing address spaces
//----------------------------------------------------------------------

unsigned
AddrSpacesRoom(unsigned blocksize)
{
    ListElement *element;
    unsigned room = 0;

    for (element = AddrSpaceList.FirstElement ();
	 element;
	 element = element->next) {
	AddrSpace *space = (AddrSpace*) element->item;
	room += machine->PageTableRoom(space->NumPages(), blocksize);
    }

    return room;
}

//----------------------------------------------------------------------
// AddrSpace::DumpAddrSpaces
//      Dump all address spaces
//----------------------------------------------------------------------

void
DumpAddrSpaces(FILE *output,
	       unsigned virtual_x, unsigned virtual_width,
	       unsigned physical_x, unsigned y, unsigned blocksize)
{
    ListElement *element;
    unsigned virtual_y = y;

    /* TODO: sort by physical page addresses to avoid too much mess */
    for (element = AddrSpaceList.FirstElement ();
	 element;
	 element = element->next) {
	AddrSpace *space = (AddrSpace*) element->item;
	virtual_y -= space->Dump(output, virtual_x, virtual_width, physical_x, virtual_y, y, blocksize);
    }
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
//      On a context switch, save any machine state, specific
//      to this address space, that needs saving.
//
//      For now, nothing!
//----------------------------------------------------------------------

void
AddrSpace::SaveState ()
{
}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
//      On a context switch, restore the machine state so that
//      this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void
AddrSpace::RestoreState ()
{
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}