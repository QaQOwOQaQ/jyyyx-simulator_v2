#ifndef DATASTRUCTURE_GUARD
#define DATASTRUCTURE_GUARD

#include <stdint.h>

/*====================================*/
/*     Circular Doubly Linked List    */
/*====================================*/
// 双向循环链表
typedef struct LINKED_KIST_NODE_STRUCT
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
void linkedlist_add(linkedlist_t **address, uint64_t value);   // 增加节点 (??为啥用双重指针)
int linkedlist_delete(linkedlist_t *list, uint64_t value);     // 删除节点
linkedlist_node_t *linkedlist_get(linkedlist_t *list, uint64_t value); // 取得值为value的节点
linkedlist_node_t *linkedlist_next(linkedlist_t *list);        // 移动


/*======================================*/
/*      Dynamic Array                   */
/*======================================*/
// 动态数组
typedef struct 
{
    uint32_t size;
    uint32_t count;
    uint64_t *table;
} array_t;

array_t *array_construct(int size); // 创建动态数组
void array_free(array_t *arr);      // 释放动态数组
void array_insert(array_t **address, uint64_t value);   // 增
void array_delete(array_t *arr, int index);             // 删
void array_get(array_t *arr, int index, uint64_t *valptr);  // 查
void print_array(array_t *arr);     // 打印动态数组


/*======================================*/
/*      Trie - Prefix Tree              */
/*======================================*/
// 字典树/前缀树
typedef struct TREE_NODE_STRUCT
{
    // '0'-'9', 'a'-'z', '%'
    struct TREE_NODE_STRUCT *next[37]; // 10 + 26 + 1
    uint64_t value;
    int isvalid;
} trie_node_t;


trie_node_t *trie_construct();
void trie_free(trie_node_t *root);
void trie_insert(trie_node_t *address, char *key, uint64_t value);
void trie_get(trie_node_t *root, char *key, uint64_t *valptr);
void trie_print(trie_node_t *root);


/*======================================*/
/*      Extendible Hash Table           */
/*======================================*/
// 可拓展哈希表
typedef struct 
{
    int localdepth;  // the local depth
    int counter;     // the counter of slots (have data)插槽的计数器（有数据）
    int **karray;
    uint64_t *varray;
} hash_table_bucket_t;

typedef struct 
{
    int num;            // number of buckets = 1 << globaldepth
    int globaldepth;    // the global depth

    int size;           // the size of (key, value) tuples of each bucket
    hashtable_bucket_t **direciory; // the internal table is actually an array
} hashtable_t;

hashtable_t *hashtable_construct(int size);
hashtable_free(hashtable_t *tab);
int hashtable_get(hashtable_t *tab, char *key, uint64_t *valptr);
int hashtale_insert(hashtable_t **address, char *key, uint64_t val);
void print_hashtable(hashtable_t *tab);

#endif