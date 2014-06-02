#ifndef __PLAN_DATAARR_H__
#define __PLAN_DATAARR_H__

#include <boruvka/segmarr.h>

struct _plan_data_arr_t {
    bor_segmarr_t *arr; /*!< Underlying segmented array */
    size_t num_els;     /*!< Number of elements stored in array. */
    void *init_data;    /*!< Initialization data buffer of size
                             arr->el_size. */
};
typedef struct _plan_data_arr_t plan_data_arr_t;


#define PLAN_DATA_ARR_GET(type, arr, i) \
    (type *)planDataArrGet((arr), (i))

plan_data_arr_t *planDataArrNew(size_t el_size, size_t segment_size,
                                const void *init_data);

void planDataArrDel(plan_data_arr_t *arr);

_bor_inline void *planDataArrGet(plan_data_arr_t *arr, size_t i);

void planDataArrResize(plan_data_arr_t *arr, size_t i);

/**** INLINES ****/
_bor_inline void *planDataArrGet(plan_data_arr_t *arr, size_t i)
{
    if (i >= arr->num_els){
        planDataArrResize(arr, i);
    }

    return borSegmArrGet(arr->arr, i);
}

#endif /* __PLAN_DATAARR_H__ */
