#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <headers/algorithm.h>

// constructor and destructor
linkedlist_t *linkedlist_construct()
{
    linkedlist_t *list = malloc(sizeof(linkedlist_t));
    list->count = 0;
    list->head = NULL;
    return list;
}

void linkedlist_free(linkedlist_t *list)
{
    if(list == NULL)
    {
        return ;
    }

    // listpre is the STACK ADDRESS storing the referenced address
    for(int i = 0; i < list->count; i ++ )
    {
        linkedlist_node_t *node = list->head;
        list->head = list->head->next;
        if(node == list->head) 
        {
            // only one element
            free(node);
            // do not update list->count during deleting
        }
        else 
        {
            // at least 2 elements
            node->prev->next = node->next;
            node->next->prev = node->prev;
            free(node);
            // do not update list->count during deleting
        }
    }
    free(list);
}

int linkedlist_add(linkedlist_t **address, uint64_t value)
{
    linkedlist_t *list = *address;
    if(list == NULL)
    {
        return FAILURE;
    }

    if(list->count == 0)
    {
        // create a new head
        list->head = malloc(sizeof(linkedlist_node_t));
        // circular linked list initialization
        list->head->prev = list->head;
        list->head->next = list->head;
        list->head->value = value;
        list->count = 1;
    }
    else 
    {
        linkedlist_node_t *node = malloc(sizeof(linkedlist_node_t));
        node->value = value;
        node->next = list->head;
        node->prev = list->head->prev;
        node->next->prev = node;
        node->prev->next = node;
        list->count ++ ;
    }
    return SUCCESS;
}

int linkedlist_delete(linkedlist_t *list, linkedlist_node_t *node)
{
    if(list == NULL)
    {
        return FAILURE;
    }

    // update the prev and next pointers
    // same foe the only one node situation
    node->prev->next = node->next;
    node->next->prev = node->prev;
    free(node);
    list->count -- ;
    if(list->count == 0) // speciall judge
    {
        list->head == NULL;
    }
    return SUCCESS;
}

linkedlist_node_t *linkedlist_get(linkedlist_t *list, uint64_t value)
{
    if(list == NULL)
    {
        return NULL;
    }

    linkedlist_node_t *p = list->head;
    for(int i = 0; i < list->count; i ++ ) // 
    {
        if(p->value == value)
        {
            return p;
        }
        p = p->next;
    }
    return NULL;
}

// traverse the linked list
linkedlist_node_t *linkedlist_next(linkedlist_t *list)
{
    if(list == NULL)
    {
        return NULL;
    }
    list->head = list->head->next;
    return list->head->prev;
}