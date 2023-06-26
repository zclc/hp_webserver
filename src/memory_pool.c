
#include "memory_pool.h"
#include "http_request.h"
#include "http.h"

#include <assert.h>
#include <stdlib.h>

FixedAllocor mpool[MEMPOOL_TYPE_LEN];

#include "dbg.h"

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


Chunk* create_chunk(TYPEINPOOL poolType)
{
    // 申请chunk的内存空间
    Chunk* chunk = (Chunk*)malloc(sizeof(Chunk));
    if(chunk == NULL)
    {
        log_err("bad alloc chunk");
        return NULL;
    }

    
    // 申请chunk中pData的内存空间
    chunk->pData = (unsigned char*)malloc(manage_type_size(poolType)*(mpool[poolType].blocks));
    if(chunk == NULL)
    {
        log_err("bad alloc chunk pdata");
        free(chunk);
        return NULL;
    }

    // 给chunk中每个block的头一个字节表示下一个可用空间的索引
    unsigned char* p = chunk->pData;
    int i;
    for ( i = 0; i < mpool[poolType].blocks; p+=mpool[poolType].blockSize)
    {
        *p = ++i;
    }

    // 初始化chunk的信息
    chunk->next = NULL;
    chunk->firstAvailableBlock = 0;
    chunk->blocksAvailable = mpool[poolType].blocks;

    return chunk;
}

/**
 * @brief 创建一个内存池
 * 
 * @param blocks 内存池中 一个chunk中多少个blocks
 * @param chunks 内存池中 一个FixedAlloctor中多少个chunks
 * @return MemoryPool* 内存池句柄
 */
int memorypool_create(unsigned char blocks)
{
    int i;
    for (i = 0; i < MEMPOOL_TYPE_LEN; i++)
    {
        // 初始化mpool中FixedAllocator
        mpool[i].chunksHead = (Chunk*)malloc(sizeof(Chunk));//挂的chunk为0
        check_exit(mpool[i].chunksHead != NULL, "bad alloc");

        mpool[i].chunksHead->next = NULL;
        mpool[i].alloctorChunk = NULL;
        mpool[i].dealloctorChunk = NULL;
        
        mpool[i].blocks = blocks;
        mpool[i].blockSize = manage_type_size(i); // blockSize由传入的类型确定
        mpool[i].blank_chunk_num = 0;
    }

    return MEM_TRUE;
}


void* Allocate(TYPEINPOOL poolType)
{
    Chunk* chunk = mpool[poolType].chunksHead->next;
    unsigned char blockSize = mpool[poolType].blockSize;
    unsigned char* presult = NULL;

    // 遍历链表查找可用的chunk
    for(;;chunk=chunk->next)
    {
        // 没有可用的chunk, 申请一个新的chunk
        if(chunk == NULL)
        {
            // 申请一个新的区块
            Chunk* newChunk = create_chunk(poolType);
            if(newChunk == NULL) /* 申请失败返回NULL */
                return NULL;
            
            // 申请成功 头插到head链表中
            newChunk->next = mpool[poolType].chunksHead->next;
            mpool[poolType].chunksHead->next = newChunk;

            chunk = newChunk;
        } 

        // 该块有可用的block
        if(chunk->blocksAvailable > 0)
        {
            presult = chunk->pData + (chunk->firstAvailableBlock)*(blockSize);
            
            // 更新该chunk的信息
            chunk->firstAvailableBlock = *presult;
            chunk->blocksAvailable--;

            break;
        }
    }

    return (void*)presult; 
}

Chunk* VicinityFind(void* p, TYPEINPOOL poolType)
{
    size_t chunkLength = mpool[poolType].blocks * mpool[poolType].blockSize;
    Chunk* curChunk = mpool[poolType].chunksHead->next;
    unsigned char* uchP = (unsigned char*)p;

    /* 遍历寻找p属于哪个chunk */
    for (;curChunk != NULL;curChunk=curChunk->next)
    {
        if(uchP >= curChunk->pData && uchP < curChunk->pData + chunkLength)
        {   
            return curChunk;
        }
    }
    
    return NULL;
}

void Deallocate(void* p, TYPEINPOOL poolType)
{
    Chunk* deallocChunk = VicinityFind(p, poolType);
    check_exit(deallocChunk != NULL, "p not own memory pool");
    
    unsigned char blockSize = mpool[poolType].blockSize;
    /* p在chunk中的位置 */
    unsigned char* toRelease = (unsigned char*)p;
    *toRelease = deallocChunk->firstAvailableBlock; /* p的下一个可用block为当前的firstAvailableBlock */

    /* 更新最新的availableBlock为 p所在block */
    deallocChunk->firstAvailableBlock = (unsigned char)(toRelease - deallocChunk->pData)/blockSize;

    // 该chunk的可用区块+1
    deallocChunk->blocksAvailable++;

    /* 如果该chunk空闲 */
    if(deallocChunk->blocksAvailable == mpool[poolType].blocks)
    {
        mpool[poolType].blank_chunk_num++;
    }

    // 有两份空闲归还一块给操作系统,
    if(mpool[poolType].blank_chunk_num == 2)
    {
        Chunk* curChunk = mpool[poolType].chunksHead->next;
        Chunk* preChunk = mpool[poolType].chunksHead;
        for (;curChunk != NULL;curChunk=curChunk->next, preChunk=preChunk->next)
        {
            if(curChunk->blocksAvailable == mpool[poolType].blocks)
            {
                preChunk->next = curChunk->next;
                free(curChunk->pData);
                free(curChunk);
                --mpool[poolType].blank_chunk_num;
                break;
            }
        }
    }
}

