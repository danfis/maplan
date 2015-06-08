#ifndef MSG_SCHEMA_H
#define MSG_SCHEMA_H

struct _sub_msg_t{
    uint32_t header;

    int32_t i32;
    int64_t *i64;
    int i64_size;
};
typedef struct _sub_msg_t sub_msg_t;

struct _msg_t {
    uint32_t header;

    int32_t int32;
    int64_t int64;
    char *str;
    int str_size;
    int8_t int8;

    int8_t *bytes;
    int bytes_size;

    sub_msg_t *subarr;
    int subarr_size;

    int32_t *arr32;
    int arr32_size;

    sub_msg_t sub;
    int32_t x;
};
typedef struct _msg_t msg_t;

PLAN_MSG_SCHEMA_BEGIN(schema_sub)
PLAN_MSG_SCHEMA_ADD(sub_msg_t, i32, INT32)
PLAN_MSG_SCHEMA_ADD_ARR(sub_msg_t, i64, i64_size, INT64)
PLAN_MSG_SCHEMA_END(schema_sub, sub_msg_t, header)

PLAN_MSG_SCHEMA_BEGIN(schema_main)
PLAN_MSG_SCHEMA_ADD(msg_t, int32, INT32)
PLAN_MSG_SCHEMA_ADD(msg_t, int64, INT64)
PLAN_MSG_SCHEMA_ADD_ARR(msg_t, str, str_size, INT8)
PLAN_MSG_SCHEMA_ADD(msg_t, int8, INT8)
PLAN_MSG_SCHEMA_ADD_ARR(msg_t, bytes, bytes_size, INT8)
PLAN_MSG_SCHEMA_ADD_ARR(msg_t, arr32, arr32_size, INT32)
PLAN_MSG_SCHEMA_ADD_MSG(msg_t, sub, &schema_sub)
PLAN_MSG_SCHEMA_ADD(msg_t, x, INT32)
PLAN_MSG_SCHEMA_ADD_MSG_ARR(msg_t, subarr, subarr_size, &schema_sub)
PLAN_MSG_SCHEMA_END(schema_main, msg_t, header)

static void wrSub(const sub_msg_t *msg, FILE *fout)
{
    int i;

    fprintf(fout, "  header: %lx\n", (long)msg->header);
    fprintf(fout, "  i32: %ld\n", (long)msg->i32);
    fprintf(fout, "  i64_size: %d\n", msg->i64_size);
    if (msg->i64 != NULL){
        fprintf(fout, "  i64:");
        for (i = 0; i < msg->i64_size; ++i)
            fprintf(fout, " %ld", (long)msg->i64[i]);
        fprintf(fout, "\n");
    }
}

static void wr(const msg_t *msg, FILE *fout)
{
    int i;

    fprintf(fout, "header: %lx\n", (long)msg->header);
    fprintf(fout, "int32: %ld\n", (long)msg->int32);
    fprintf(fout, "int64: %ld\n", (long)msg->int64);
    fprintf(fout, "str_size: %d\n", msg->str_size);
    if (msg->str != NULL)
        fprintf(fout, "str: `%s'\n", msg->str);
    fprintf(fout, "int8: %d\n", (int)msg->int8);
    fprintf(fout, "bytes_size: %d\n", msg->bytes_size);
    if (msg->bytes != NULL){
        fprintf(fout, "bytes:");
        for (i = 0; i < msg->bytes_size; ++i)
            fprintf(fout, " %d", (int)msg->bytes[i]);
        fprintf(fout, "\n");
    }
    fprintf(fout, "subarr_size: %d\n", msg->subarr_size);
    if (msg->subarr != NULL){
        for (i = 0; i < msg->subarr_size; ++i)
            wrSub(msg->subarr + i, fout);
    }
    fprintf(fout, "arr32_size: %d\n", msg->arr32_size);
    if (msg->arr32 != NULL){
        fprintf(fout, "arr32:");
        for (i = 0; i < msg->arr32_size; ++i)
            fprintf(fout, " %ld", (long)msg->arr32[i]);
        fprintf(fout, "\n");
    }
    wrSub(&msg->sub, fout);
    fprintf(fout, "x: %ld\n", (long)msg->x);
}

#endif /* MSG_SCHEMA_H */
