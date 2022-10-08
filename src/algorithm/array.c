#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <headers/common.h>
#include <headers/algorithm.h>

array_t *array_construct(int size)
{
    array_t *arr = malloc(sizeof(array_t));
    arr->count = 0;
    arr->size = size;
    arr->table = malloc(sizeof(uint64_t) * arr->size);
    return arr;
}

void array_free(array_t *arr)
{
    free(arr->table);
    free(arr);
}

array_t *array_insert(array_t *arr, uint64_t value)
{
    if(arr == NULL)
    {
        return NULL;
    }

    if(arr->count == arr->size)
    {
        // malloc to twice size
        uint64_t *old_table = arr->table;

        arr->size = 2 * arr->size;
        arr->table = malloc(arr->size * sizeof(uint64_t));
        // count unchaned
        
        // copy from the old to the new
        for(int i = 0; i < arr->count; i ++ )
        {
            arr->table[i] = old_table[i];
        }
        // free the old table memory
        free(old_table);
    }

    arr->table[arr->count] = value;
    arr->count += 1;
    return arr;
}

int array_delete(array_t *arr, int index)
{
    if(arr == NULL || index < 0 || arr->count <= index)
    {
        return FAILURE;
    }
    /* 为什么要在delete时shrink？ 我的理解是在delete元素时，数组可能呈现一种收缩的趋势
        此时如果我们的数组空间size太大，而用的size太少，空间浪费就十分严重
        同理，在insert的时候，我们假设此时数组呈现一种膨胀的趋势，因此需要拓展数组空间size
    */
    if(arr->count - 1 <= arr->size / 4)
    {
        // shrink the table to half when payload is quater(当有效负载为四分之一时，将表缩小到一半
        uint64_t *old_table = arr->table;
        arr->size = arr->size / 2;
        arr->table = malloc(arr->size * sizeof(uint64_t));
        // count unchanged

        // copy from the old to new
        int j = 0;
        for(int i = 0; i < arr->count; i ++ )
        {
            if(i != index)
            {
                arr->table[j] = old_table[j];
                j ++ ;
            }
        }
        arr->count ++ ;
        free_table(old_table);
        return SUCCESS;
    }
    else // don't need to shrink 
    {
        // copy move from index forward
        for(int i = index + 1; i < arr->count; i ++ )
        {
            if(i - 1 >= 0)
            {
                arr->table[i - 1] = arr->table[i];
            }
        }
        arr->count -= 1;
        return SUCCESS;
    }
    return FAILURE;
}

int array_get(array_t *arr, int index, uint64_t *valptr)
{
    if(index < 0 || index >= arr->count)
    {
        // invalid index
        return FAILURE;
    }
    else 
    {
        // found at [index]
        *valptr = arr->table[index];
        return SUCCESS;
    }
}

#ifdef DEBUG_ARRAY

void print_array(array_t *arr)
{
    printf("array size: %u, count: %u\n", arr->size, arr->count);
    for(int i = 0; i < arr->count; i ++ )
    {
        printf("\t[%d] %16lx\n", i, arr->table[i]);
    }
}

#endif
