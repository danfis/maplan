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

#define HEADER(msg_struct, schema) \
    (*(uint32_t *)(((unsigned char *)(msg_struct)) + (schema)->header_offset))
#define FIELD(msg_struct, schema, type) \
    (*(type *)(((unsigned char *)(msg_struct)) + (schema).offset))



unsigned char *planMsgBufEncode(const void *msg,
                                const plan_msg_schema_t *_schema,
                                int *size)
{
    int schema_size = _schema->size;
    const plan_msg_schema_field_t *schema = _schema->schema;
    uint32_t header = HEADER(msg, _schema);
    uint32_t enable;
    unsigned char *buf, *wbuf;
    int type, i, bufsize;
    int16_t val_int16;
    int32_t val_int32;
    int64_t val_int64;
    char *val_char;

    bufsize = sizeof(uint32_t);
    enable = header;
    for (i = 0; i < schema_size; ++i){
        if (enable & 0x1u){
            type = schema[i].type;
            bufsize += byte_size[type];

            if (type == PLAN_MSG_SCHEMA_STR){
                bufsize += strlen(FIELD(msg, schema[i], char *));
                // TODO: bytes, msg, repeated field ...
            }
        }

        enable >>= 1;
    }

    buf = BOR_ALLOC_ARR(unsigned char, bufsize);
    *size = bufsize;

    wbuf = buf;
    memcpy(wbuf, &header, 4);
    wbuf += 4;
    enable = header;
    for (i = 0; i < schema_size; ++i){
        if (enable & 0x1u){
            type = schema[i].type;
            if (type == PLAN_MSG_SCHEMA_INT32){
                val_int32 = FIELD(msg, schema[i], int32_t);
                memcpy(wbuf, &val_int32, 4);
                wbuf += 4;

            }else if (type == PLAN_MSG_SCHEMA_INT64){
                val_int64 = FIELD(msg, schema[i], int64_t);
                memcpy(wbuf, &val_int64, 8);
                wbuf += 8;

            }else if (type == PLAN_MSG_SCHEMA_STR){
                val_char = FIELD(msg, schema[i], char *);
                val_int16 = strlen(val_char);
                memcpy(wbuf, &val_int16, 2);
                wbuf += 2;
                memcpy(wbuf, val_char, val_int16);
                wbuf += val_int16;
            }
        }

        enable >>= 1;
    }

    return buf;
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
