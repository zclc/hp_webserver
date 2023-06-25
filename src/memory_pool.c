
#include "memory_pool.h"
#include "http_request.h"
#include "http.h"

#include <stdlib.h>

FixedAllocor* mpool[MEMPOOL_TYPE_LEN];

size_t manage_type_size(TYPEINPOOL poolType)
{
    switch (poolType)
    {
    case MEMPOOL_HTTP_REQUESET_T:
        return sizeof(zv_http_request_t);
        break;
    case MEMPOOL_HTTP_OUT_T:
        return sizeof(zv_http_out_t);
        break;
    case MEMPOOL_HTTP_HEADER_T:
        return sizeof(zv_http_header_t);
        break;
    case MEMPOOL_HTTP_HEADER_HANDLE_T:
        return sizeof(zv_http_header_handle_t); 
        break;
    default:
        return MEM_FALSE;
        break;
    }
}

void initChunk(MemoryPool* mp, Chunk* chunk)
{

}

Chunk* create_chunk(TYPEINPOOL poolType)
{
    Chunk* chunk = (Chunk*)malloc(sizeof(Chunk));
    assert(chunk != NULL);

    chunk->next = NULL;
    chunk->blockSize= manage_type_size(poolType);
    chunk->pData = (unsigned char*)malloc(manage_type_size(poolType)*(alloc->blocks));
    assert(chunk != NULL);

    int i;
    unsigned char* p = chunk->pData;
    for ( i = 0; i < mpool[poolType]->blocks; p+=chunk->blockSize)
    {
        *p = ++i;
    }
    chunk->firstAvailableBlock = 0;
    chunk->blocksAvailable = mpool[poolType]->blocks;

    return chunk;
}

/**
 * @brief 创建一个内存池
 * 
 * @param blocks 内存池中 一个chunk中多少个blocks
 * @param chunks 内存池中 一个FixedAlloctor中多少个chunks
 * @return MemoryPool* 内存池句柄
 */
int memorypool_create(int blocks, int chunks, size_t chunkSize)
{
    // MemoryPool* mp = (MemoryPool*)malloc(sizeof(MemoryPool));
    // if(mp == NULL)
    // {
    //     return NULL;
    // }

    // // 初始化mp中的值
    // mp->blocks = blocks;
    // mp->chunks = chunks;
    // mp->fixArrLen = MEMPOOL_TYPE_LEN; // TYPEINPOOL 中的最大值
   
    // // 分配chunks管理者的空间
    // mp->fixArr = (FixedAllocor**)malloc(sizeof(FixedAllocor*));
    // if(mp->fixArr == NULL)
    // {
    //     free(mp);
    //     return NULL;
    // }

    int i;
    for (i = 0; i < MEMPOOL_TYPE_LEN; i++)
    {
        mpool[i] = (FixedAllocor*)malloc(sizeof(FixedAllocor));
        if(mpool[i] == NULL)
        {
            goto mallocErr;
        }
        mpool[i]->chunks = chunks; 
        mpool[i]->blocks = blocks;
        mpool[i]->alloctorChunk = NULL;
        mpool[i]->dealloctorChunk = NULL;
        mpool[i]->chunk_num = 0;
        mpool[i]->chunksHead = NULL;//挂的chunk为0
    }
    
   

    return MEM_TRUE;
mallocErr:
    for (i = 0; i < MEMPOOL_TYPE_LEN; i++)
    {
        if(mpool[i] != NULL)
        {
            free(mpool[i]);
        }
    }

    return MEM_FALSE;

}


void* allocate(TYPEINPOOL poolType)
{
    Chunk* tmp = mpool[poolType]->chunksHead;
    if(mpool[poolType]->alloctorChunk == NULL || mpool[poolType]->alloctorChunk->blocksAvailable == 0) // 当前结点没有chunk
    {
        for(;;tmp=tmp->next)
        {
            if(tmp == NULL)
            {
                Chunk* chunk = create_chunk(poolType);
                if(chunk == NULL)
                    return NULL;
                
                chunk->next = mpool[poolType]->chunksHead->next;
                mpool[poolType]->chunksHead->next = chunk;

                mpool[poolType]->alloctorChunk = chunk;
            } 
        }

        mpool[poolType]->chunksHead->next = create_chunk(poolType);
        if(mpool[poolType]->chunksHead->next == NULL)
            return NULL;
        
        mpool[poolType]->alloctorChunk = mpool[poolType]->chunksHead->next;
    }

    if()
    {
        Chunk* chunk = create_chunk(poolType);
        if(chunk == NULL)
            return NULL;
        
        chunk->next = mpool[poolType]->chunksHead->next;
        mpool[poolType]->chunksHead->next = chunk;

        mpool[poolType]->alloctorChunk = chunk;
    }



}