#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <headers/common.h>
#include <headers/algorithm.h>

typedef void(cleanup_t)(); // 函数指针cleanup_t

static array_t *events = NULL; // array 是一个函数指针数组

void add_cleanup_event(void *func) // 增加一个函数指针
{
    assert(func != NULL);

    if(events == NULL)
    {
        // uninitialized -- lay malloc
        // start from 8 slots(槽)
        events = array_construct(8); 
    }
    
    // fill in the first event
    array_insert(&events, (uint64_t)func); // 将函数func的地址放入array
    return ;
}

void finally_cleanup()
{
    for(int i = 0; i < events->count; i ++ )
    {
        uint64_t address;
        assert(array_get(events, i, &address) != 0);

        cleanup_t *func;
        *(uint64_t *)&func = address;
        (*func)();
    }

    // clean itself
    array_free(events);
    events = NULL;
}