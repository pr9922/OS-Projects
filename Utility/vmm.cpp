#include "vmm.h"
#include "console.h"
#include "stdint.h"
#include "machine.h"

#define MISSING() do { \
    putStr(__FILE__); \
    putStr(":"); \
    putDec(__LINE__); \
    putStr(" is missing\n"); \
    shutdown(); \
} while (0)

/* Each frame is 4K */
#define FRAME_SIZE (1 << 12)

/* A table contains 4K/4 = 1K page table entries */
#define TABLE_ENTRIES (FRAME_SIZE / sizeof(uint32_t))

/* A table, either a PD, or a PT */
typedef struct {
    uint32_t entries[TABLE_ENTRIES];
} table_s;

/* Address of first avaialble frame */
uint32_t avail = 0x101000;
uint32_t commonFrame = 0;

/* pointer to page directory */
table_s *pd = 0;
uint32_t mostRecentFault = 0;

/* The world's simplest frame allocator */
uint32_t frame(void) {
    uint32_t ptr = avail;
    avail += FRAME_SIZE;
    return ptr;
}

extern void vmm_on(uint32_t);
extern void invlpg(uint32_t);

void doNothing(uint32_t pa) {
}

void zeroIt(uint32_t va) {
    char* p = (char*) va;
    for (int i=0; i<FRAME_SIZE; i++) {
        p[i] = 0;
    }
}

void paging(void) {
   if(pd == 0){   //if pd hasn't been initialized
       pd = (table_s*)frame();
   } 
   for(int i = 0; i < (1 << 21); i+=FRAME_SIZE){ //initialize mappings
     map(i,i, doNothing);
   }
   vmm_on((uint32_t)pd); 
}

uint32_t last(void) {
    return mostRecentFault;
}

/* handle a page fault */
void pageFault(uint32_t addr) {
   if(commonFrame == 0){       //CREATE common frame to handle multiple page faults. CRUCIAL!
       commonFrame = frame();
       zeroIt(commonFrame);
   } 
   putStr("fault at ");
   putHex(addr);
   putStr("\n");
   mostRecentFault = addr;
   map(addr, commonFrame, doNothing); 
}

/* Create a new mapping from va to pa */
void map(uint32_t va, uint32_t pa, void (*init)(uint32_t)) {
  uint32_t VPN = va >> 12;  
  uint32_t offset = va & 0xFFF;
  uint32_t VPNHigh = ((VPN >> 10) & 0x3FF);  //Get Highest 10 bits
  uint32_t VPNLow = (VPN & 0x3FF);            //Get Lowest 10 bits
  uint32_t pde = pd->entries[VPNHigh];
   if(pde == 0){ 
       pde = frame() | 3;           //Thanks Shavran
       pd->entries[VPNHigh] = pde;
       zeroIt(pde & 0xFFFFF000);
   } 
   table_s* pt = (table_s*)((pde >> 12) << 12);
   pt->entries[VPNLow] = pa | 3;
   invlpg(va);
   init(va - offset);       //invalidate page and call init
}

/* return the PA that corresponds to the given VA, 0xffffffff is not mapped */
uint32_t va2pa(uint32_t va) { //Just like what he said in class
   uint32_t VPN = (va >> 12) & 0xFFFFF;  
   uint32_t offset = va & 0xFFF;
   uint32_t VPNHigh = ((VPN >> 10) & 0x3FF);  //Get Highest 10 bits
   uint32_t VPNLow = (VPN & 0x3FF);            //Get Lowest 10 bits
   uint32_t pde = pd->entries[VPNHigh]; 
   table_s* pt = (table_s*)((pde >> 12) << 12);
   uint32_t pte = pt->entries[VPNLow];
   uint32_t ppn = ((pte >> 12) << 12);
   uint32_t pa = 0;
   if(((pde&1)==1) && ((pte&1)==1)){ //Need to check both pde AND pte!
       pa = ppn + offset;
   }
   else{
       return 0xffffffff;
   }
   return pa;
}

/* unmap the given va */
int forget(uint32_t va) {
   uint32_t VPN = (va >> 12) & 0xFFFFF;  
   uint32_t VPNHigh = ((VPN >> 10));  //Get Highest 10 bits
   uint32_t VPNLow = (VPN & 0x3FF);            //Get Lowest 10 bits
   uint32_t pde = pd->entries[VPNHigh]; 
   table_s* pt = (table_s*)((pde >> 12) << 12);
   uint32_t pte = pt->entries[VPNLow];
   uint32_t flipped = pte & -2;
   pt->entries[VPNLow] = flipped;
   invlpg(va);
   return ((flipped & 0x7F) >> 6) & 1; //Return dirty bit
}
