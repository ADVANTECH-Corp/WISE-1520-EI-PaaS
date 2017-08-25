#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wisememory.h"
#include "wiseconfig.h"

static int dynamic = 0;
static char *buffer = 0;
static int size = 0;
static int current = 0;
static int remain = 0;

void WiseMem_Init(void *address, int len) {
    if(address == 0) {
        dynamic = 1;
        buffer = (char *)malloc(len);
    } else {
        dynamic = 0;
        buffer = (char *)address;
    }
    size = len;
    current = 0;
    remain = len;
    memset(buffer,0,len);
}

void *__WiseMem_Alloc(int len, char *file, int line) {
    char *result = NULL;
    //printf("alloc %d, remain %d , max %d <%s, %d>\n", len, remain, file, line, MAX_ALLOC_SIZE);
    
    if(len > MAX_ALLOC_SIZE) len = MAX_ALLOC_SIZE;
    if(len+sizeof(int) > remain) {
    	return NULL;
    }
    
    result = buffer + current;
    *(int *)result = len;
    
    result += sizeof(int);
    current += len + sizeof(int);
    remain -= len + sizeof(int);
    return result;
}

int WiseMem_Size(void *address) {
	char *a = (char *)address;
    a -= sizeof(int);
    return *(int *)a;
}

void WiseMem_Release() {
    memset(buffer,0,current);
    current = 0;
    remain = size;
}

void WiseMem_Destory() {
    if(dynamic == 1) {
        dynamic = 0;
        free(buffer);
    }
}
