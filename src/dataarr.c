#include <stdio.h>
#include <boruvka/alloc.h>
#include "plan/dataarr.h"


plan_data_arr_t *planDataArrNew(size_t el_size, size_t segment_size,
                                void *init_data)
{
    plan_data_arr_t *arr;

    arr = BOR_ALLOC(plan_data_arr_t);
    arr->arr = borSegmArrNew(el_size, segment_size);
    if (arr->arr == NULL){
        fprintf(stderr, "Error: Too low segment size for segmented array.\n");
        exit(-1);
    }

    arr->num_els = 0;
    arr->init_data = BOR_ALLOC_ARR(char, el_size);
    memcpy(arr->init_data, init_data, el_size);

    return arr;
}

void planDataArrDel(plan_data_arr_t *arr)
{
    borSegmArrDel(arr->arr);
    if (arr->init_data)
        BOR_FREE(arr->init_data);

    BOR_FREE(arr);
}

void planDataArrResize(plan_data_arr_t *arr, size_t eli)
{
    size_t i;
    void *data;

    for (i = arr->num_els; i < eli + 1; ++i){
        data = borSegmArrGet(arr->arr, i);
        memcpy(data, arr->init_data, arr->arr->el_size);
    }

    arr->num_els = eli + 1;
}
