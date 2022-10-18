/*
 * heap.h
 *
 *  Created on: 10 окт. 2022 г.
 *      Author: Ivan
 */

#ifndef REAL_HEAP_H_
#define REAL_HEAP_H_

#include "stdlib.h"

void* malloc(size_t size)
{
    void* ptr = NULL;

    if(size > 0)
    {
        // We simply wrap the FreeRTOS call into a standard form
        ptr = pvPortMalloc(size);
    } // else NULL if there was an error

    return ptr;
}

void free(void* ptr)
{
    if(ptr)
    {
        // We simply wrap the FreeRTOS call into a standard form
        vPortFree(ptr);
    }
}



#endif /* REAL_HEAP_H_ */
