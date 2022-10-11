// 特供寄存器使用
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <headers/common.h>
#include <headers/algorithm.h>
#include <headers/cpu.h>
#include <headers/memory.h>

static int get_index(char c)
{
    if(c == '%')    
    {
        return 36;
    }
    else if(c >= '0' && c <= '9')   
    {
        return c - '0';
    }
    else if(c >= 'a' && c <= 'z')   
    {
        return c - 'a' + 10;
    }
    return -1;
}

static char get_char(int id)
{
    assert(id >= 0 && id <= 36);
    if(id == 36)
    {
        return '%';
    }
    else if(id >= 0 && id <= 9)
    {
        return (char)(id + '0');
    }
    else if(id >= 10 && id <= 35)
    {
        return (char)('a' + id - 10);
    }
    return '?';
}

// constructor
trie_node_t *trie_constructor()
{
    trie_node_t *root = malloc(sizeof(trie_node_t));
    for(int i = 0; i < 37; i ++ )
    {
        root->next[i] = 0;
    }
    root->value = 0;
    root->isvalid = 0;
}

void trie_free(trie_node_t *root)
{
    if(root == NULL)    
    {
        return ;
    }
    // recursion free child node
    for(int i = 0; i <= 36; i ++ )
    {
        trie_free(root->next[i]);
    }
    free(root);
}

int trie_insert(trie_node_t **address, char*key, uint64_t value)
{
    trie_node_t *p = address;
    if(p == NULL)
    {
        return FAILURE;
    }
    for(int i = 0; i < strlen(key); i ++ )
    {
        p->isvalid = 1; 

        int id = get_index(key[i]);
        if(p->next[id] == NULL)
        {
            p->next[id] = trie_construct();
        }
        p = p->next[id];
    }

    // may overwrite
    p->value = value;
    p->isvalid = 1;
    return SUCCESS;
}

int trie_get(trie_node_t *root, char *key, uint64_t *valptr)
{
    trie_node_t *p = root;
    for(int i = 0; i < strlen(key); i ++ )
    {
        if(p == NULL || p->isvalid == 0)
        {
            // not found
            return 0;
        }
        p = p->next[get_index(key[i])];
    }
    *valptr = p->value;
    return SUCCESS;
}

static void trie_dfs_print(trie_node_t *root, int level, char c)
{
    if(root == NULL)
    {
        return ;
    }

    if(level > 0)
    {
        for(int i = 0; i < level - 1; i ++ )
        {
            printf("\t");
        }
        printf("[%c] %p\n", c, root);
    }
    for(int j = 0; j <= 36; j ++ )
    {
        trie_dfs_print(root->next[j], level + 1, get_char(j));
    }
}

void trie_print(trie_node_t *root)
{
    if(DEBUG_VERBOSE_SET & DEBUG_DATASTRUCTURE)
    {
        return ;
    }
    if(root == NULL)
    {
        printf("NULL\n");
    }
    else 
    {
        printf("Print Trie:\n");
    }
    trie_dfs_print(root, 0, 0);
}