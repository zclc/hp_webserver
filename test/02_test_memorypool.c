#include "../src/http_request.h"
#include "../src/memory_pool.h"

#include <stdio.h>

#define MAXLEN 10000

int main(int argc, char const *argv[])
{
    zv_http_request_t *p1[MAXLEN];
    zv_http_out_t *p2[MAXLEN];
    zv_http_header_t *p3[MAXLEN];
    zv_http_header_handle_t *p4[MAXLEN];

    memorypool_create(64);

    for (int i = 0; i < MAXLEN; i++)
    {
        p1[i] = (zv_http_request_t *)Allocate(MEMPOOL_HTTP_REQUESET_T);
        p2[i] = (zv_http_out_t *)Allocate(MEMPOOL_HTTP_OUT_T);
        p3[i] = (zv_http_header_t *)Allocate(MEMPOOL_HTTP_HEADER_T);
        p4[i] = (zv_http_header_handle_t *)Allocate(MEMPOOL_HTTP_HEADER_HANDLE_T);
    }

    for (int i = MAXLEN-1; i >= 0; i--)
    {
        Deallocate(p1[i], MEMPOOL_HTTP_REQUESET_T);
        Deallocate(p2[i], MEMPOOL_HTTP_OUT_T);
        Deallocate(p3[i], MEMPOOL_HTTP_HEADER_T);
        Deallocate(p4[i], MEMPOOL_HTTP_HEADER_HANDLE_T);
    }

    
    for (int i = 0; i < MAXLEN; i++)
    {
        p1[i] = (zv_http_request_t *)Allocate(MEMPOOL_HTTP_REQUESET_T);
        p2[i] = (zv_http_out_t *)Allocate(MEMPOOL_HTTP_OUT_T);
        p3[i] = (zv_http_header_t *)Allocate(MEMPOOL_HTTP_HEADER_T);
        p4[i] = (zv_http_header_handle_t *)Allocate(MEMPOOL_HTTP_HEADER_HANDLE_T);
    }

    for (int i = MAXLEN-1; i >= 0; i--)
    {
        Deallocate(p1[i], MEMPOOL_HTTP_REQUESET_T);
        Deallocate(p2[i], MEMPOOL_HTTP_OUT_T);
        Deallocate(p3[i], MEMPOOL_HTTP_HEADER_T);
        Deallocate(p4[i], MEMPOOL_HTTP_HEADER_HANDLE_T);
    }

    for (int i = 0; i < MAXLEN; i++)
    {
        p1[i] = (zv_http_request_t *)Allocate(MEMPOOL_HTTP_REQUESET_T);
        p2[i] = (zv_http_out_t *)Allocate(MEMPOOL_HTTP_OUT_T);
        p3[i] = (zv_http_header_t *)Allocate(MEMPOOL_HTTP_HEADER_T);
        p4[i] = (zv_http_header_handle_t *)Allocate(MEMPOOL_HTTP_HEADER_HANDLE_T);
    }

    for (int i = MAXLEN-1; i >= 0; i--)
    {
        Deallocate(p1[i], MEMPOOL_HTTP_REQUESET_T);
        Deallocate(p2[i], MEMPOOL_HTTP_OUT_T);
        Deallocate(p3[i], MEMPOOL_HTTP_HEADER_T);
        Deallocate(p4[i], MEMPOOL_HTTP_HEADER_HANDLE_T);
    }


    return 0;
}