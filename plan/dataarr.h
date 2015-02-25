#ifndef __PLAN_DATAARR_H__
#define __PLAN_DATAARR_H__

#include <boruvka/segmarr.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef void (*plan_data_arr_el_init_fn)(void *el, const void *userdata);

struct _plan_data_arr_t {
    bor_segmarr_t *arr; /*!< Underlying segmented array */
    size_t num_els;     /*!< Number of elements stored in array. */
    plan_data_arr_el_init_fn init_fn;
    void *init_data;    /*!< Initialization data buffer of size
                             arr->el_size. */
};
typedef struct _plan_data_arr_t plan_data_arr_t;

/**
 * Creates a new segmented array.
 * The size of segment is determined automatically.
 * If init_fn function callback is non-NULL, it is used for initialization
 * of each element before the element is first returned. In this case
 * init_data is used as last argument of init_fn function.
 * If init_fn is NULL, the init_data should point to a memory of el_size
 * size and the data it points at are copied to internal storage and used
 * for initialization of each element before it is first returned.
 */
plan_data_arr_t *planDataArrNew(size_t el_size,
                                plan_data_arr_el_init_fn init_fn,
                                const void *init_data);

/**
 * Deletes segmented array
 */
void planDataArrDel(plan_data_arr_t *arr);

/**
 * Creates an exact copy of the segmented array.
 */
plan_data_arr_t *planDataArrClone(const plan_data_arr_t *arr);

/**
 * Returns pointer to the i'th element of the array.
 */
_bor_inline void *planDataArrGet(plan_data_arr_t *arr, size_t i);

/**
 * Ensures that the array has at least i elements.
 */
void planDataArrResize(plan_data_arr_t *arr, size_t i);

/**** INLINES ****/
_bor_inline void *planDataArrGet(plan_data_arr_t *arr, size_t i)
{
    if (i >= arr->num_els){
        planDataArrResize(arr, i);
    }

    return borSegmArrGet(arr->arr, i);
}

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_DATAARR_H__ */
