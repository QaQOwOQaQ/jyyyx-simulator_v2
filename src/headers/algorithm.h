#ifndef DATASTRUCTURE_GUARD
#define DATASTRUCTURE_GUARD

// 用作程序的结束标志 0 - 非正常结束； 1 - 正常结束
#define FAILURE 0
#define SUCCESS 1

#define NULL_ID 0 // ??

#include <stdint.h>

/*====================================*/
/*     Circular Doubly Linked List    */
/*====================================*/
// 双向循环链表
typedef struct LINKED_LIST_NODE_STRUCT
{
    uint64_t value;
    struct LINKED_LIST_NODE_STRUCT *prev;
    struct LINKED_LIST_NODE_STRUCT *next;
} linkedlist_node_t;

typedef struct 
{
    linkedlist_node_t *head;
    uint64_t count;
} linkedlist_t;

linkedlist_t *linkedlist_construct();     // 创建链表
void linkedlist_free(linkedlist_t *list); // 释放链表
int linkedlist_add(linkedlist_t **address, uint64_t value);            // 在链表的末尾增加节点
int linkedlist_delete(linkedlist_t *list, linkedlist_node_t *node);    // 删除node节点
linkedlist_node_t *linkedlist_get(linkedlist_t *list, uint64_t value); // 取得值为value的节点
linkedlist_node_t *linkedlist_next(linkedlist_t *list);        // 将链表向前移动一个单位之前的头节点


/*======================================*/
/*      Dynamic Array                   */
/*======================================*/
// 动态数组(完成)
typedef struct 
{
    uint32_t size;      // 动态数组的实际大小
    uint32_t count;     // 动态数组中实际存放的元素的数目
    uint64_t *table;    // 存放数组元素的空间
} array_t;

array_t *array_construct(int size); // 创建数字大小为size的动态数组并返回指向这个数组的指针
void array_free(array_t *arr);      // 释放动态数组
array_t *array_insert(array_t *arr, uint64_t value);   // 在数组的末尾添加一个元素
int array_delete(array_t *arr, int index);             // 删除下表为index的元素
int array_get(array_t *arr, int index, uint64_t *valptr);  // 查找下表为index的元素并将元素返回到valptr


/*======================================*/
/*      Trie - Prefix Tree              */
/*======================================*/
// 字典树/前缀树
typedef struct TREE_NODE_STRUCT
{
    // '0'-'9', 'a'-'z', '%'
    struct TREE_NODE_STRUCT *next[37]; // 10 + 26 + 1
    uint64_t value;
    int isvalid; // 不太理解
} trie_node_t;

trie_node_t *trie_construct();
void trie_free(trie_node_t *root);
void trie_insert(trie_node_t **address, char *key, uint64_t value);
void trie_get(trie_node_t *root, char *key, uint64_t *valptr);
void trie_print(trie_node_t *root);


/*======================================*/
/*      Extendible Hash Table           */
/*======================================*/
// 可拓展哈希表
typedef struct 
{
    int localdepth;  // the local depth
    int counter;     // the counter of slots (have data): 插槽的计数器（有数据）
    int **karray;
    uint64_t *varray;
} hashtable_bucket_t;

typedef struct 
{
    int num;            // number of buckets = 1 << globaldepth
    int globaldepth;    // the global depth

    int size;           // the size of (key, value) tuples of each bucket
    hashtable_bucket_t **directory; // the internal table is actually an array
} hashtable_t;

hashtable_t *hashtable_construct(int size);
void hashtable_free(hashtable_t *tab);
int hashtable_get(hashtable_t *tab, char *key, uint64_t *valptr);
int hashtale_insert(hashtable_t **address, char *key, uint64_t val);
void print_hashtable(hashtable_t *tab);

#endif