
#include <stdlib.h>
#include <malloc.h>
#include "FreeRTOS.h"
#include "picolibc.h"

size_t xPortGetFreeHeapSize( void ) PRIVILEGED_FUNCTION {
    struct mallinfo mi = mallinfo();
    return mi.fordblks;
}

