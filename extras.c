
#include <stdlib.h>
#include <malloc.h>
#include "FreeRTOS.h"
#include "picolibc.h"

size_t xPortGetFreeHeapSize( void ) PRIVILEGED_FUNCTION {
    struct mallinfo mi = mallinfo();

    // Get total heap size from linker
    extern size_t __heap_size;
    
    return (((size_t) &__heap_size) - mi.arena) + mi.fordblks;
}
