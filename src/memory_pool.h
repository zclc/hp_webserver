// 专门为
// zv_http_request_t
// zv_http_out_t
// zv_http_header_t
// zv_http_header_handle_t
// mime_type_t

#define MEM_TRUE 1;
#define MEM_FALSE 0;

typedef enum
{
    MEMPOOL_HTTP_REQUESET_T = 0,
    MEMPOOL_HTTP_OUT_T,
    MEMPOOL_HTTP_HEADER_T,
    MEMPOOL_HTTP_HEADER_HANDLE_T,
    MEMPOOL_TYPE_LEN // 不存储信息，记录该枚举类型的个数
}TYPEINPOOL;



struct _Chunk
{
    unsigned char firstAvailableBlock;
    unsigned char blockSize;
    unsigned char* pData;
    unsigned char blocksAvailable;
    struct _Chunk* next;
};
typedef struct _Chunk Chunk;

typedef struct 
{
    Chunk* chunksHead; // chunk链表
    Chunk* alloctorChunk; // 指向将要分配块
    Chunk* dealloctorChunk; // 指向将要删除块
    unsigned int chunks; // 一个fixedalloctor管理多少个chunk
    unsigned int blocks; // 一个chunk拥有多少个block
    unsigned int chunk_num;
}FixedAllocor;







// 传一个类型返回给它一个指针
void* allocate(TYPEINPOOL pool_data_t);

/**
 * @brief 创建一个内存池
 * 
 * @param blocks 内存池中 一个chunk中多少个blocks
 * @param chunks 内存池中 一个FixedAlloctor中多少个chunks
 * @return MemoryPool* 内存池句柄
 */



