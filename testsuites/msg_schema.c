#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <cu/cu.h>
#include <plan/msg_schema.h>
#include <boruvka/compiler.h>
#include <boruvka/alloc.h>

struct _msg_t {
    int32_t int32;
    int64_t int64;
    char *str;
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
PLAN_MSG_SCHEMA_END(schema_main)

TEST(testMsgSchema)
{
    msg_t msg, msg2;
    plan_msg_buf_t msg_buf;
    uint32_t enable;
    int i, len;

    assertEquals(schema_main.size, 3);
    assertEquals(schema_main.schema[0].type, PLAN_MSG_SCHEMA_INT32);
    assertEquals(schema_main.schema[0].offset, 0);
    assertEquals(schema_main.schema[1].type, PLAN_MSG_SCHEMA_INT64);
    assertEquals(schema_main.schema[1].offset, offsetof(msg_t, int64));
    assertEquals(schema_main.schema[2].type, PLAN_MSG_SCHEMA_STR);
    assertEquals(schema_main.schema[2].offset, offsetof(msg_t, str));

    for (i = 0; i < 1000; ++i){
        bzero(&msg, sizeof(msg));
        bzero(&msg2, sizeof(msg2));
        msg.int32 = rand();
        msg.int64 = rand();
        enable = 3u;
        planMsgBufEncode(&msg_buf, &schema_main, enable, &msg);

        planMsgBufDecode(&msg2, &schema_main, msg_buf.buf);
        assertEquals(msg.int32, msg2.int32);
        assertEquals(msg.int64, msg2.int64);

        planMsgBufFree(&msg_buf);
    }

    for (i = 0; i < 1000; ++i){
        bzero(&msg, sizeof(msg));
        bzero(&msg2, sizeof(msg2));
        msg.int64 = rand();
        len = rand() % text_size;
        msg.str = strndup(text, len);
        enable = 6u;
        planMsgBufEncode(&msg_buf, &schema_main, enable, &msg);

        planMsgBufDecode(&msg2, &schema_main, msg_buf.buf);
        assertEquals(msg.int64, msg2.int64);
        assertEquals(strlen(msg.str), strlen(msg2.str));
        assertEquals(strcmp(msg.str, msg2.str), 0);
        free(msg.str);
        BOR_FREE(msg2.str);

        planMsgBufFree(&msg_buf);
    }

    /*
    for (int i = 0; i < schema_main.size; ++i){
        printf("[%d]: type: %d, offset: %d\n", i,
               schema_main.schema[i].type,
               schema_main.schema[i].offset);
    }
    */
}
