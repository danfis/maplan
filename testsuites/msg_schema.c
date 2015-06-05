#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <cu/cu.h>
#include <plan/msg_schema.h>
#include <boruvka/compiler.h>
#include <boruvka/alloc.h>

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

static char text[] = "Non eram nescius, Brute, cum, quae summis ingeniis"
" exquisitaque doctrina philosophi Graeco sermone tractavissent, ea Latinis"
" litteris mandaremus, fore ut hic noster labor in varias reprehensiones"
" incurreret. nam quibusdam, et iis quidem non admodum indoctis, totum hoc"
" displicet philosophari. quidam autem non tam id reprehendunt, si remissius"
" agatur, sed tantum studium tamque multam operam ponendam in eo non"
" arbitrantur. erunt etiam, et ii quidem eruditi Graecis litteris,"
" contemnentes Latinas, qui se dicant in Graecis legendis operam malle"
" consumere. postremo aliquos futuros suspicor, qui me ad alias litteras"
" vocent, genus hoc scribendi, etsi sit elegans, personae tamen et"
" dignitatis esse negent.";
static int text_size = sizeof(text) - 1;

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

TEST(testMsgSchema)
{
    msg_t msg, msg2;
    unsigned char *buf;
    int i, j, size;

    assertEquals(schema_main.size, 9);
    assertEquals(schema_main.schema[0].type, _PLAN_MSG_SCHEMA_INT32);
    assertEquals(schema_main.schema[0].offset, offsetof(msg_t, int32));
    assertEquals(schema_main.schema[0].sub, NULL);
    assertEquals(schema_main.schema[1].type, _PLAN_MSG_SCHEMA_INT64);
    assertEquals(schema_main.schema[1].offset, offsetof(msg_t, int64));
    assertEquals(schema_main.schema[2].type, _PLAN_MSG_SCHEMA_ARR_BASE + _PLAN_MSG_SCHEMA_INT8);
    assertEquals(schema_main.schema[2].offset, offsetof(msg_t, str));
    assertEquals(schema_main.schema[3].type, _PLAN_MSG_SCHEMA_INT8);
    assertEquals(schema_main.schema[3].offset, offsetof(msg_t, int8));
    assertEquals(schema_main.schema[4].type, _PLAN_MSG_SCHEMA_ARR_BASE + _PLAN_MSG_SCHEMA_INT8);
    assertEquals(schema_main.schema[4].offset, offsetof(msg_t, bytes));
    assertEquals(schema_main.schema[4].size_offset, offsetof(msg_t, bytes_size));
    assertEquals(schema_main.schema[5].type, _PLAN_MSG_SCHEMA_ARR_BASE + _PLAN_MSG_SCHEMA_INT32);
    assertEquals(schema_main.schema[5].offset, offsetof(msg_t, arr32));
    assertEquals(schema_main.schema[5].size_offset, offsetof(msg_t, arr32_size));
    assertEquals(schema_main.schema[6].type, _PLAN_MSG_SCHEMA_MSG);
    assertEquals(schema_main.schema[6].offset, offsetof(msg_t, sub));
    assertEquals(schema_main.schema[6].sub, &schema_sub);
    assertEquals(schema_main.schema[8].type, _PLAN_MSG_SCHEMA_ARR_BASE + _PLAN_MSG_SCHEMA_MSG);
    assertEquals(schema_main.schema[8].offset, offsetof(msg_t, subarr));
    assertEquals(schema_main.schema[8].size_offset, offsetof(msg_t, subarr_size));
    assertEquals(schema_main.schema[8].sub, &schema_sub);

    for (i = 0; i < 1000; ++i){
        bzero(&msg, sizeof(msg));
        bzero(&msg2, sizeof(msg2));
        msg.int32 = rand();
        msg.int64 = rand();
        msg.header = 3u;
        buf = planMsgBufEncode(&msg, &schema_main, &size);

        planMsgBufDecode(&msg2, &schema_main, buf);
        assertEquals(msg.int32, msg2.int32);
        assertEquals(msg.int64, msg2.int64);

        BOR_FREE(buf);
    }

    for (i = 0; i < 1000; ++i){
        bzero(&msg, sizeof(msg));
        bzero(&msg2, sizeof(msg2));
        msg.int64 = rand();
        msg.str_size = rand() % (text_size - 1);
        msg.str_size += 1;
        msg.str = strndup(text, msg.str_size - 1);
        msg.header = 6u;
        buf = planMsgBufEncode(&msg, &schema_main, &size);

        planMsgBufDecode(&msg2, &schema_main, buf);
        assertEquals(msg.int64, msg2.int64);
        assertEquals(strlen(msg.str), strlen(msg2.str));
        assertEquals(strcmp(msg.str, msg2.str), 0);
        assertEquals(msg.str_size, msg2.str_size);
        free(msg.str);
        BOR_FREE(msg2.str);

        BOR_FREE(buf);
    }

    for (i = 0; i < 1000; ++i){
        bzero(&msg, sizeof(msg));
        bzero(&msg2, sizeof(msg2));
        msg.int64 = rand();
        msg.str_size = rand() % (text_size - 1);
        msg.str_size += 1;
        msg.str = strndup(text, msg.str_size - 1);
        msg.int8 = rand();
        msg.header = 14u;
        buf = planMsgBufEncode(&msg, &schema_main, &size);

        planMsgBufDecode(&msg2, &schema_main, buf);
        assertEquals(msg.int64, msg2.int64);
        assertEquals(strlen(msg.str), strlen(msg2.str));
        assertEquals(strcmp(msg.str, msg2.str), 0);
        assertEquals(msg.str_size, msg2.str_size);
        assertEquals(msg.int8, msg2.int8);
        free(msg.str);
        BOR_FREE(msg2.str);

        BOR_FREE(buf);
    }

    for (i = 0; i < 1000; ++i){
        bzero(&msg, sizeof(msg));
        bzero(&msg2, sizeof(msg2));
        msg.int32 = rand();
        msg.str_size = rand() % (text_size - 1);
        msg.str_size += 1;
        msg.str = strndup(text, msg.str_size - 1);
        msg.arr32_size = rand() % 100 + 1;
        msg.arr32 = BOR_ALLOC_ARR(int32_t, msg.arr32_size);
        for (j = 0; j < msg.arr32_size; ++j)
            msg.arr32[j] = rand();
        msg.header = 0x1 | 0x4 | 0x20;
        buf = planMsgBufEncode(&msg, &schema_main, &size);

        planMsgBufDecode(&msg2, &schema_main, buf);
        assertEquals(msg.int32, msg2.int32);
        assertEquals(strlen(msg.str), strlen(msg2.str));
        assertEquals(strcmp(msg.str, msg2.str), 0);
        assertEquals(msg.str_size, msg2.str_size);
        assertEquals(msg.int8, msg2.int8);
        assertEquals(msg.arr32_size, msg2.arr32_size);
        assertEquals(memcmp(msg.arr32, msg2.arr32, 4 * msg2.arr32_size), 0);
        free(msg.str);
        BOR_FREE(msg2.str);
        BOR_FREE(msg.arr32);
        BOR_FREE(msg2.arr32);

        BOR_FREE(buf);
    }

    for (i = 0; i < 1000; ++i){
        bzero(&msg, sizeof(msg));
        bzero(&msg2, sizeof(msg2));
        msg.int32 = rand();
        msg.sub.header = 0x1;
        msg.sub.i32 = rand();
        msg.header = 0x1 | (0x1 << 6);
        buf = planMsgBufEncode(&msg, &schema_main, &size);

        planMsgBufDecode(&msg2, &schema_main, buf);
        assertEquals(msg.int32, msg2.int32);
        assertEquals(msg.sub.i32, msg2.sub.i32);

        BOR_FREE(buf);
    }

    for (i = 0; i < 1000; ++i){
        bzero(&msg, sizeof(msg));
        bzero(&msg2, sizeof(msg2));
        msg.x = rand();
        msg.sub.header = 0x3;
        msg.sub.i32 = rand();
        msg.sub.i64_size = rand() % 100 + 1;
        msg.sub.i64 = BOR_ALLOC_ARR(int64_t, msg.sub.i64_size);
        for (j = 0; j < msg.sub.i64_size; ++j)
            msg.sub.i64[j] = rand();
        msg.header = (0x1 << 6) | (0x1 << 7);
        buf = planMsgBufEncode(&msg, &schema_main, &size);

        planMsgBufDecode(&msg2, &schema_main, buf);
        assertEquals(msg.x, msg2.x);
        assertEquals(msg.sub.i32, msg2.sub.i32);
        assertEquals(msg.sub.i64_size, msg2.sub.i64_size);
        assertEquals(memcmp(msg.sub.i64, msg2.sub.i64, 8 * msg.sub.i64_size), 0);

        BOR_FREE(msg.sub.i64);
        BOR_FREE(msg2.sub.i64);
        BOR_FREE(buf);
    }

    for (i = 0; i < 1000; ++i){
        bzero(&msg, sizeof(msg));
        bzero(&msg2, sizeof(msg2));
        msg.int32 = rand();
        msg.subarr_size = rand() % 20 + 1;
        msg.subarr = BOR_ALLOC_ARR(sub_msg_t, msg.subarr_size);
        for (j = 0; j < msg.subarr_size; ++j){
            msg.subarr[j].header = 0x1;
            msg.subarr[j].i32 = rand();
        }
        msg.header = 0x1 | (0x1 << 8);
        buf = planMsgBufEncode(&msg, &schema_main, &size);

        planMsgBufDecode(&msg2, &schema_main, buf);
        assertEquals(msg.int32, msg2.int32);
        assertEquals(msg.subarr_size, msg2.subarr_size);

        BOR_FREE(msg.subarr);
        BOR_FREE(msg2.subarr);
        BOR_FREE(buf);
    }

    bzero(&msg, sizeof(msg));
    bzero(&msg2, sizeof(msg2));
    msg.int32 = rand();
    msg.subarr_size = 2;
    msg.subarr = BOR_ALLOC_ARR(sub_msg_t, msg.subarr_size);
    msg.subarr[0].header = 0x1;
    msg.subarr[0].i32 = rand();
    msg.subarr[1].header = 0x3;
    msg.subarr[1].i32 = rand();
    msg.subarr[1].i64_size = 3;
    msg.subarr[1].i64 = BOR_ALLOC_ARR(int64_t, 3);
    msg.subarr[1].i64[0] = 1;
    msg.subarr[1].i64[1] = 1012341553;
    msg.subarr[1].i64[2] = 1012341553345;
    msg.header = 0x1 | (0x1 << 8);
    buf = planMsgBufEncode(&msg, &schema_main, &size);

    planMsgBufDecode(&msg2, &schema_main, buf);
    assertEquals(msg.int32, msg2.int32);
    assertEquals(msg.subarr_size, msg2.subarr_size);
    assertEquals(msg.subarr[0].i32, msg2.subarr[0].i32);
    assertEquals(msg.subarr[1].i32, msg2.subarr[1].i32);
    assertEquals(msg.subarr[1].i64_size, msg2.subarr[1].i64_size);
    assertEquals(memcmp(msg.subarr[1].i64, msg2.subarr[1].i64, 3 * 8), 0);

    BOR_FREE(msg.subarr[1].i64);
    BOR_FREE(msg.subarr);
    BOR_FREE(msg2.subarr[1].i64);
    BOR_FREE(msg2.subarr);
    BOR_FREE(buf);

    /*
    for (int i = 0; i < schema_main.size; ++i){
        printf("[%d]: type: %d, offset: %d\n", i,
               schema_main.schema[i].type,
               schema_main.schema[i].offset);
    }
    */
}
