#include "../src/http_request.h"
#include "memory_pool.h"

#include <stdio.h>

#define MAXLEN 10

int main(int argc, char const *argv[])
{
    zv_http_request_t *p1[MAXLEN];
    zv_http_request_t *p2[MAXLEN];
    zv_http_header_t *p3[MAXLEN];
    zv_http_header_handle_t *p4[MAXLEN];

    memorypool_create(2);

    for (int i = 0; i < MAXLEN; i++)
    {
        p1[i] = (zv_http_request_t *)Allocate(MEMPOOL_HTTP_REQUESET_T);
    }

    for (int i = 0; i < MAXLEN; i++)
    {
        Deallocate(p1[i],MEMPOOL_HTTP_REQUESET_T);
    }

    for (int i = 0; i < MAXLEN; i++)
    {
        p1[i] = (zv_http_request_t *)Allocate(MEMPOOL_HTTP_REQUESET_T);
    }
    for (int i = 0; i < MAXLEN; i+=2)
    {
        Deallocate(p1[i],MEMPOOL_HTTP_REQUESET_T);
    }
    for (int i = 1; i < MAXLEN; i+=2)
    {
        Deallocate(p1[i],MEMPOOL_HTTP_REQUESET_T);
    }

    for (int i = 0; i < MAXLEN; i++)
    {
        p1[i] = (zv_http_request_t *)Allocate(MEMPOOL_HTTP_REQUESET_T);
    }

    
    for (int i = 0; i < MAXLEN; i+=3)
    {
        Deallocate(p1[i],MEMPOOL_HTTP_REQUESET_T);
    }
    for (int i = 1; i < MAXLEN; i+=3)
    {
        Deallocate(p1[i],MEMPOOL_HTTP_REQUESET_T);
    }
    for (int i = 2; i < MAXLEN; i+=3)
    {
        Deallocate(p1[i],MEMPOOL_HTTP_REQUESET_T);
    }


    return 0;
}