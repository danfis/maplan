/***
 * maplan
 * -------
 * Copyright (c)2015 Daniel Fiser <danfis@danfis.cz>,
 * Agent Technology Center, Department of Computer Science,
 * Faculty of Electrical Engineering, Czech Technical University in Prague.
 * All rights reserved.
 *
 * This file is part of maplan.
 *
 * Distributed under the OSI-approved BSD License (the "License");
 * see accompanying file BDS-LICENSE for details or see
 * <http://www.opensource.org/licenses/bsd-license.php>.
 *
 * This software is distributed WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the License for more information.
 */

#include <boruvka/alloc.h>
#include <plan/msg_schema.h>

static int byte_size[4] = {
    /* PLAN_MSG_SCHEMA_INT32 */ 4,
    /* PLAN_MSG_SCHEMA_INT64 */ 8,
    /* PLAN_MSG_SCHEMA_STR */   2,
    /* PLAN_MSG_SCHEMA_BYTES */ 2,
};

#define FIELD(msg_struct, schema, type) \
    (*(type *)(((unsigned char *)(msg_struct)) + (schema).offset))


void planMsgBufFree(plan_msg_buf_t *buf)
{
    if (buf->buf && buf->buf_alloc)
        BOR_FREE(buf->buf);
}

void planMsgBufEncode(plan_msg_buf_t *buf, const plan_msg_schema_t *_schema,
                      uint32_t enable_flags, const void *msg_struct)
{
    int schema_size = _schema->size;
    const plan_msg_schema_field_t *schema = _schema->schema;
    int type, i;
    unsigned char *wbuf;
    int16_t val_int16;
    int32_t val_int32;
    int64_t val_int64;
    char *val_char;

    buf->header = 0u;
    buf->bufsize = sizeof(uint32_t);
    for (i = 0; i < schema_size; ++i){
        if (enable_flags & 0x1u){
            type = schema[i].type;
            buf->header |= (0x1u << i);
            buf->bufsize += byte_size[type];

            if (type == PLAN_MSG_SCHEMA_STR){
                buf->bufsize += strlen(FIELD(msg_struct, schema[i], char *));
                // TODO: bytes, msg, repeated field ...
            }
        }

        enable_flags >>= 1;
    }

    buf->buf = BOR_ALLOC_ARR(unsigned char, buf->bufsize);
    buf->buf_alloc = 1;

    wbuf = buf->buf;
    memcpy(wbuf, &buf->header, 4);
    wbuf += 4;
    enable_flags = buf->header;
    for (i = 0; i < schema_size; ++i){
        if (enable_flags & 0x1u){
            type = schema[i].type;
            if (type == PLAN_MSG_SCHEMA_INT32){
                val_int32 = FIELD(msg_struct, schema[i], int32_t);
                memcpy(wbuf, &val_int32, 4);
                wbuf += 4;

            }else if (type == PLAN_MSG_SCHEMA_INT64){
                val_int64 = FIELD(msg_struct, schema[i], int64_t);
                memcpy(wbuf, &val_int64, 8);
                wbuf += 8;

            }else if (type == PLAN_MSG_SCHEMA_STR){
                val_char = FIELD(msg_struct, schema[i], char *);
                val_int16 = strlen(val_char);
                memcpy(wbuf, &val_int16, 2);
                wbuf += 2;
                memcpy(wbuf, val_char, val_int16);
                wbuf += val_int16;
            }
        }

        enable_flags >>= 1;
    }
}

void planMsgBufDecode(void *msg_struct, const plan_msg_schema_t *_schema,
                      const void *buf)
{
    int schema_size = _schema->size;
    const plan_msg_schema_field_t *schema = _schema->schema;
    uint32_t header;
    const unsigned char *rbuf;
    int type, i, len;
    char *strbuf;

    rbuf = buf;
    header = ((uint32_t *)rbuf)[0];
    rbuf += 4;
    for (i = 0; i < schema_size; ++i){
        if (header & 0x1u){
            type = schema[i].type;

            if (type == PLAN_MSG_SCHEMA_INT32){
                FIELD(msg_struct, schema[i], int32_t) = *(int32_t *)rbuf;
                rbuf += 4;

            }else if (type == PLAN_MSG_SCHEMA_INT64){
                FIELD(msg_struct, schema[i], int64_t) = *(int64_t *)rbuf;
                rbuf += 8;

            }else if (type == PLAN_MSG_SCHEMA_STR){
                len = *(int16_t *)rbuf;
                rbuf += 2;
                strbuf = BOR_ALLOC_ARR(char, len + 1);
                memcpy(strbuf, rbuf, len);
                strbuf[len] = 0;
                FIELD(msg_struct, schema[i], char *) = strbuf;
            }
        }

        header >>= 1;
    }
}
