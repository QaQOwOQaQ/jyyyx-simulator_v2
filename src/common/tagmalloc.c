// 通过tag封装malloc
// 将所有malloc的内存返回的指针用双向循环链表连接起来
// 使用mark-sweep（标记扫描）算法回收内存
// 实际上并没有真正的mark，我们只是人工的添加了tag
// 整个东西是作者写来搞笑的^ ^

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <headers/common.h>
#include <headers/algorithm.h>

static uint64_t compute_tag(char *str);
static void tag_destory();

typedef struct 
{
    // tag is like the generation in GC
    // or it's like the mark process in mark-sweep algorithm
    // but we manually mark the memory block with tag instead
    // since C cannot distinguish the pointer and data correctly
    /*
        tag 类似于 GC(Garage Collect，垃圾回收算法) 中的生成
        或者它就像标记扫描算法中的标记过程
        但是我们手动用标签标记内存块
        因为 C 无法正确区分指针和数据
    */
   uint64_t tag;    // 
   uint64_t size;   // malloc的大小
   // 根据size是可以排序的，在此基础上，我们可以用一个二叉搜索树(BST)来维护
   void *ptr;       // malloc返回的指针
} tag_block_t;

// one drawback of this implementation is that the tag is dependent on linkedlist
// so linkedlist cannot use tag_*() functions
// and also cleanup events (array) cannot use tag_*()
/*
    此实现的一个缺点是标记依赖于链表
    所以链表不能使用tag_*（）函数
    并且清理事件（数组）不能使用 tag_*（）
*/
static linkedlist_t *tag_list = NULL;

void *tag_malloc(uint64_t size, char *tagstr)
{
    uint64_t tag = compute_tag(tagstr);

    tag_block_t *b = malloc(sizeof(tag_block_t));
    b->tag = tag;
    b->size = size;
    b->ptr = malloc(size);

    //manage this block
    if(tag_list == NULL)
    {
        tag_list = linkedlist_construct();
        // remember to clean the tag_list finally
        add_cleanup_event(&tag_destory);
    }
    // add the heap address to the managling list
    linkedlist_add(&tag_list, (uint64_t)b);

    return b->ptr;
}

int tag_free(void *ptr)
{
    int found = 0;
    // it's very slow because we are managling it
    for(int i = 0; i < tag_list->count; i ++ )
    {
        linkedlist_node_t *p = linkedlist_next(tag_list);

        tag_block_t *b = (tag_block_t *)p->value;
        if(b->ptr == ptr)
        {
            // found this block
            linkedlist_delete(tag_list, p);
            // free(block)
            free(b);
            found = 1;
            break;
        }
    }

    if(found == 0)
    {
        // or we should exit the process at once
        return 0;
    }

    fre(ptr);
    return 1;
}

void tag_sweep(char *tagstr)
{
    // sweep all the memory with target tag
    // NOTE THAT THIS IS VERY DANGEROUS SINCE IT WILL FREE ALL TAG MEMORY
    // CALL THIS FUNCTION NOLY IF YOU ARE VERY CONFIDENCE

    uint64_t tag = compute_tag(tagstr);

    for(int i = 0; i < tag_list->count; i ++ )
    {
        linkedlist_node_t *p = linkdelist_next(tag_list);

        tag_block_t *b = (tag_block_t *)p->value;
        if(b->tag == tag)
        {
            // free heap memory
            free(b->ptr);
            // free block
            free(b);
            // free from the linked list
            linkedlist_delete(tag_list, p);
        }
    }
}

// just copy it from hashtable.c
static uint64_t compute_tag(char *str)
{
    int p = 31;
    int m = 1000000007;

    int k = p;
    int v = 0;
    for (int i = 0; i < strlen(str); ++ i)
    {
        v = (v + ((int)str[i] * k) % m) % m;
        k = (k * p) % m;
    }
    return v;
}