#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <cu/cu.h>
#include <plan/msg_schema.h>
#include <boruvka/compiler.h>
#include <boruvka/alloc.h>

struct _msg_t {
    uint32_t header;
    int32_t int32;
    int64_t int64;
    char *str;
    int8_t int8;
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

PLAN_MSG_SCHEMA_BEGIN(schema_main)
PLAN_MSG_SCHEMA_ADD(msg_t, int32, INT32)
PLAN_MSG_SCHEMA_ADD(msg_t, int64, INT64)
PLAN_MSG_SCHEMA_ADD(msg_t, str, STR)
PLAN_MSG_SCHEMA_ADD(msg_t, int8, INT8)
PLAN_MSG_SCHEMA_END(schema_main, msg_t, header)

TEST(testMsgSchema)
{
    msg_t msg, msg2;
    unsigned char *buf;
    int i, len, size;

    assertEquals(schema_main.size, 4);
    assertEquals(schema_main.schema[0].type, PLAN_MSG_SCHEMA_INT32);
    assertEquals(schema_main.schema[0].offset, offsetof(msg_t, int32));
    assertEquals(schema_main.schema[1].type, PLAN_MSG_SCHEMA_INT64);
    assertEquals(schema_main.schema[1].offset, offsetof(msg_t, int64));
    assertEquals(schema_main.schema[2].type, PLAN_MSG_SCHEMA_STR);
    assertEquals(schema_main.schema[2].offset, offsetof(msg_t, str));
    assertEquals(schema_main.schema[3].type, PLAN_MSG_SCHEMA_INT8);
    assertEquals(schema_main.schema[3].offset, offsetof(msg_t, int8));

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
        len = rand() % text_size;
        msg.str = strndup(text, len);
        msg.header = 6u;
        buf = planMsgBufEncode(&msg, &schema_main, &size);

        planMsgBufDecode(&msg2, &schema_main, buf);
        assertEquals(msg.int64, msg2.int64);
        assertEquals(strlen(msg.str), strlen(msg2.str));
        assertEquals(strcmp(msg.str, msg2.str), 0);
        free(msg.str);
        BOR_FREE(msg2.str);

        BOR_FREE(buf);
    }

    for (i = 0; i < 1000; ++i){
        bzero(&msg, sizeof(msg));
        bzero(&msg2, sizeof(msg2));
        msg.int64 = rand();
        len = rand() % text_size;
        msg.str = strndup(text, len);
        msg.int8 = rand();
        msg.header = 14u;
        buf = planMsgBufEncode(&msg, &schema_main, &size);

        planMsgBufDecode(&msg2, &schema_main, buf);
        assertEquals(msg.int64, msg2.int64);
        assertEquals(strlen(msg.str), strlen(msg2.str));
        assertEquals(strcmp(msg.str, msg2.str), 0);
        assertEquals(msg.int8, msg2.int8);
        free(msg.str);
        BOR_FREE(msg2.str);

        BOR_FREE(buf);
    }

    /*
    for (int i = 0; i < schema_main.size; ++i){
        printf("[%d]: type: %d, offset: %d\n", i,
               schema_main.schema[i].type,
               schema_main.schema[i].offset);
    }
    */
}
