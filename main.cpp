#include "kernel/src/melib.h"
#include "random.h"
#include <psppower.h>
#include <pspctrl.h>
#include <pspsdk.h>

PSP_MODULE_INFO("meds", 0, 1, 1);
PSP_HEAP_SIZE_KB(-1024);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_VFPU | PSP_THREAD_ATTR_USER);

#define BUFFER_WIDTH 512

#define BASE_SHARED_MEM 0x08400000
#define MAX_COUNTER 256

int* mem = nullptr;
bool stop = false;
u32 color = 0xFF;


void drawLine(const u32 y, const u32 size, const u32 color) {
  const u32 base = BASE_SHARED_MEM + y * 4 * BUFFER_WIDTH;
  u32* addr = (u32*)base;
  while (addr < (u32 *)(base + (BUFFER_WIDTH*4) * size)) {
    *addr = 0xFF000000 | color;
    addr++;
  }
}

int meLoop() {
  static u32 counter = 0;
  if(counter++ > MAX_COUNTER) {
    color = randInRange(0xFF) << 16 | randInRange(0xFF) << 8 | randInRange(0xFF);
    counter = 0;
  }
  drawLine(16, 16, color);
  return !stop;
}

int setupDmacplusLcdc() {
  // reg(0xBC800104) = 3; // pixel format
  *((u32*)0xBC800108) = 512; // buffer size
  *((u32*)0xBC80010C) = 512; // stride
  *((u32*)0xBC800100) = BASE_SHARED_MEM;
  sceKernelDcacheWritebackInvalidateAll();
  return 0;
}

int main(int argc, char **argv) {
  scePowerSetClockFrequency(333, 333, 166);

  if (pspSdkLoadStartModule("ms0:/PSP/GAME/me/mds_klib.prx", PSP_MEMORY_PARTITION_KERNEL) < 0){
    sceKernelExitGame();
    return 0;
  }
  
  MeCom meCom = {
    nullptr,
    meLoop,
  };
  
  kernel_callback(&setupDmacplusLcdc); // make sure to call this before me
  me_init(&meCom);
  sceKernelDelayThread(100000);

  SceCtrlData ctl;
  do {
    drawLine(48, 16, color & 0xFFAAA555);
    sceKernelDcacheWritebackInvalidateAll();
    asm("sync");
    sceKernelDelayThread(500000); // make sure to visualize both CPUs working differently
    sceCtrlPeekBufferPositive(&ctl, 1);
  } while(!(ctl.Buttons & PSP_CTRL_HOME)); // keep pressing to exit due to the delay
  
  stop = true;
  sceKernelDcacheWritebackInvalidateAll();
  asm("sync");
  
  pspDebugScreenInit();
  pspDebugScreenClear();
  pspDebugScreenSetXY(0, 1);
  pspDebugScreenPrintf("Exiting...");
  sceKernelDelayThread(1000000);
  sceKernelExitGame();
  return 0;
}
