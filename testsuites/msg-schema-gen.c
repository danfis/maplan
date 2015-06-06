#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <plan/msg_schema.h>
#include <boruvka/alloc.h>

#include "msg-schema.h"

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

int main(int argc, char *argv[])
{
    msg_t msg;
    unsigned char *buf;
    int i, j, size;
    FILE *fout, *fout2;

    if (argc != 3){
        fprintf(stderr, "Usage: %s out.bin out.txt\n", argv[0]);
        return -1;
    }

    srand(1000);

    fout = fopen(argv[1], "wb");
    fout2 = fopen(argv[2], "w");

    for (i = 0; i < 1000; ++i){
        bzero(&msg, sizeof(msg));
        msg.int32 = rand();
        msg.int64 = rand();
        msg.header = 3u;
        wr(&msg, fout2);
        buf = planMsgBufEncode(&msg, &schema_main, &size);
        uint32_t siz = htole32(size);
        fwrite(&siz, 4, 1, fout);
        fwrite(buf, 1, size, fout);

        BOR_FREE(buf);
    }

    for (i = 0; i < 1000; ++i){
        bzero(&msg, sizeof(msg));
        msg.int64 = rand();
        msg.str_size = rand() % (text_size - 1);
        msg.str_size += 1;
        msg.str = strndup(text, msg.str_size - 1);
        msg.header = 6u;
        wr(&msg, fout2);
        buf = planMsgBufEncode(&msg, &schema_main, &size);
        uint32_t siz = htole32(size);
        fwrite(&siz, 4, 1, fout);
        fwrite(buf, 1, size, fout);
        free(msg.str);
        BOR_FREE(buf);
    }

    for (i = 0; i < 1000; ++i){
        bzero(&msg, sizeof(msg));
        msg.int64 = rand();
        msg.str_size = rand() % (text_size - 1);
        msg.str_size += 1;
        msg.str = strndup(text, msg.str_size - 1);
        msg.int8 = rand();
        msg.header = 14u;
        wr(&msg, fout2);
        buf = planMsgBufEncode(&msg, &schema_main, &size);
        uint32_t siz = htole32(size);
        fwrite(&siz, 4, 1, fout);
        fwrite(buf, 1, size, fout);

        free(msg.str);
        BOR_FREE(buf);
    }

    for (i = 0; i < 1000; ++i){
        bzero(&msg, sizeof(msg));
        msg.int32 = rand();
        msg.str_size = rand() % (text_size - 1);
        msg.str_size += 1;
        msg.str = strndup(text, msg.str_size - 1);
        msg.arr32_size = rand() % 100 + 1;
        msg.arr32 = BOR_ALLOC_ARR(int32_t, msg.arr32_size);
        for (j = 0; j < msg.arr32_size; ++j)
            msg.arr32[j] = rand();
        msg.header = 0x1 | 0x4 | 0x20;
        wr(&msg, fout2);
        buf = planMsgBufEncode(&msg, &schema_main, &size);
        uint32_t siz = htole32(size);
        fwrite(&siz, 4, 1, fout);
        fwrite(buf, 1, size, fout);

        free(msg.str);
        BOR_FREE(msg.arr32);
        BOR_FREE(buf);
    }

    for (i = 0; i < 1000; ++i){
        bzero(&msg, sizeof(msg));
        msg.int32 = rand();
        msg.sub.header = 0x1;
        msg.sub.i32 = rand();
        msg.header = 0x1 | (0x1 << 6);
        wr(&msg, fout2);
        buf = planMsgBufEncode(&msg, &schema_main, &size);
        uint32_t siz = htole32(size);
        fwrite(&siz, 4, 1, fout);
        fwrite(buf, 1, size, fout);

        BOR_FREE(buf);
    }

    for (i = 0; i < 1000; ++i){
        bzero(&msg, sizeof(msg));
        msg.x = rand();
        msg.sub.header = 0x3;
        msg.sub.i32 = rand();
        msg.sub.i64_size = rand() % 100 + 1;
        msg.sub.i64 = BOR_ALLOC_ARR(int64_t, msg.sub.i64_size);
        for (j = 0; j < msg.sub.i64_size; ++j)
            msg.sub.i64[j] = rand();
        msg.header = (0x1 << 6) | (0x1 << 7);
        wr(&msg, fout2);
        buf = planMsgBufEncode(&msg, &schema_main, &size);
        uint32_t siz = htole32(size);
        fwrite(&siz, 4, 1, fout);
        fwrite(buf, 1, size, fout);

        BOR_FREE(msg.sub.i64);
        BOR_FREE(buf);
    }

    for (i = 0; i < 1000; ++i){
        bzero(&msg, sizeof(msg));
        msg.int32 = rand();
        msg.subarr_size = rand() % 20 + 1;
        msg.subarr = BOR_CALLOC_ARR(sub_msg_t, msg.subarr_size);
        for (j = 0; j < msg.subarr_size; ++j){
            msg.subarr[j].header = 0x1;
            msg.subarr[j].i32 = rand();
        }
        msg.header = 0x1 | (0x1 << 8);
        wr(&msg, fout2);
        buf = planMsgBufEncode(&msg, &schema_main, &size);
        uint32_t siz = htole32(size);
        fwrite(&siz, 4, 1, fout);
        fwrite(buf, 1, size, fout);

        BOR_FREE(msg.subarr);
        BOR_FREE(buf);
    }

    bzero(&msg, sizeof(msg));
    msg.int32 = rand();
    msg.subarr_size = 2;
    msg.subarr = BOR_CALLOC_ARR(sub_msg_t, msg.subarr_size);
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
    wr(&msg, fout2);
    buf = planMsgBufEncode(&msg, &schema_main, &size);
    uint32_t siz = htole32(size);
    fwrite(&siz, 4, 1, fout);
    fwrite(buf, 1, size, fout);

    BOR_FREE(msg.subarr[1].i64);
    BOR_FREE(msg.subarr);
    BOR_FREE(buf);

    fclose(fout);
    fclose(fout2);

    return 0;
}
