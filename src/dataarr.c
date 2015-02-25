#include <stdio.h>
#include <unistd.h>
#include <boruvka/alloc.h>
#include "plan/dataarr.h"


plan_data_arr_t *planDataArrNew(size_t el_size,
                                plan_data_arr_el_init_fn init_fn,
                                const void *init_data)
{
    plan_data_arr_t *arr;
    size_t segment_size;

    // compute best segment size
    segment_size = sysconf(_SC_PAGESIZE);
    segment_size *= 128;
    while (segment_size < el_size)
        segment_size *= 2;

    arr = BOR_ALLOC(plan_data_arr_t);
    arr->arr = borSegmArrNew(el_size, segment_size);
    if (arr->arr == NULL){
        fprintf(stderr, "Error: Too low segment size for segmented array.\n");
        exit(-1);
    }

    arr->num_els = 0;
    if (init_fn){
        arr->init_fn   = init_fn;
        arr->init_data = (void *)init_data;
    }else{
        arr->init_fn   = NULL;
        arr->init_data = BOR_ALLOC_ARR(char, el_size);
        memcpy(arr->init_data, init_data, el_size);
    }

    return arr;
}

void planDataArrDel(plan_data_arr_t *arr)
{
    borSegmArrDel(arr->arr);
    if (!arr->init_fn && arr->init_data)
        BOR_FREE(arr->init_data);

    BOR_FREE(arr);
}

plan_data_arr_t *planDataArrClone(const plan_data_arr_t *src)
{
    plan_data_arr_t *arr;
    size_t elsize, segmsize;
    void *data;
    int i;

    elsize = src->arr->el_size;
    segmsize = src->arr->segm_size;

    arr = BOR_ALLOC(plan_data_arr_t);
    memcpy(arr, src, sizeof(*src));
    arr->arr = borSegmArrNew(elsize, segmsize);
    if (arr->arr == NULL){
        fprintf(stderr, "Error: Too low segment size for segmented array.\n");
        exit(-1);
    }

    if (!src->init_fn){
        arr->init_data = BOR_ALLOC_ARR(char, elsize);
        memcpy(arr->init_data, src->init_data, elsize);
    }

    arr->num_els = src->num_els;
    for (i = 0; i < src->num_els; ++i){
        data = borSegmArrGet(arr->arr, i);
        memcpy(data, borSegmArrGet(src->arr, i), elsize);
    }

    return arr;
}

void planDataArrResize(plan_data_arr_t *arr, size_t eli)
{
    size_t i;
    void *data;

    for (i = arr->num_els; i < eli + 1; ++i){
        data = borSegmArrGet(arr->arr, i);
        if (!arr->init_fn){
            memcpy(data, arr->init_data, arr->arr->el_size);
        }else{
            arr->init_fn(data, arr->init_data);
        }
    }

    arr->num_els = eli + 1;
}
