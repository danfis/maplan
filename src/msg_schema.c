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

static int byte_size[5] = {
    /* PLAN_MSG_SCHEMA_INT8 */  1,
    /* PLAN_MSG_SCHEMA_INT32 */ 4,
    /* PLAN_MSG_SCHEMA_INT64 */ 8,
    /* PLAN_MSG_SCHEMA_STR */   2,
    /* PLAN_MSG_SCHEMA_BYTES */ 2,
};

#define HEADER(msg_struct, schema) \
    (*(uint32_t *)(((unsigned char *)(msg_struct)) + (schema)->header_offset))
#define FIELD_PTR(msg_struct, offset) \
    (void *)(((unsigned char *)(msg_struct)) + offset)
#define FIELD(msg_struct, schema, type) \
    (*(type *)FIELD_PTR((msg_struct), (schema).offset))

_bor_inline void wField(unsigned char **wbuf, const void *msg, int offset,
                        size_t size)
{
    void *v = FIELD_PTR(msg, offset);
    memcpy(*wbuf, v, size);
    *wbuf += size;
}

_bor_inline void wInt8(unsigned char **wbuf, const void *msg,
                        const plan_msg_schema_field_t *field)
{
    wField(wbuf, msg, field->offset, 1);
}

_bor_inline void wInt32(unsigned char **wbuf, const void *msg,
                        const plan_msg_schema_field_t *field)
{
    wField(wbuf, msg, field->offset, 4);
}

_bor_inline void wInt64(unsigned char **wbuf, const void *msg,
                        const plan_msg_schema_field_t *field)
{
    wField(wbuf, msg, field->offset, 8);
}

_bor_inline void wStr(unsigned char **wbuf, const void *msg,
                      const plan_msg_schema_field_t *field)
{
    char *str = FIELD(msg, *field, char *);
    int16_t len = strlen(str);
    memcpy(*wbuf, &len, 2);
    *wbuf += 2;
    memcpy(*wbuf, str, len);
    *wbuf += len;
}


_bor_inline void rInt8(unsigned char **rbuf, void *msg,
                       const plan_msg_schema_field_t *field)
{
    FIELD(msg, *field, int8_t) = *(int8_t *)*rbuf;
    *rbuf += 1;
}

_bor_inline void rInt32(unsigned char **rbuf, void *msg,
                        const plan_msg_schema_field_t *field)
{
    FIELD(msg, *field, int32_t) = *(int32_t *)*rbuf;
    *rbuf += 4;
}

_bor_inline void rInt64(unsigned char **rbuf, void *msg,
                        const plan_msg_schema_field_t *field)
{
    FIELD(msg, *field, int64_t) = *(int64_t *)*rbuf;
    *rbuf += 8;
}

_bor_inline void rStr(unsigned char **rbuf, void *msg,
                      const plan_msg_schema_field_t *field)
{
    char *strbuf;
    int len;

    len = *(int16_t *)*rbuf;
    *rbuf += 2;
    strbuf = BOR_ALLOC_ARR(char, len + 1);
    memcpy(strbuf, *rbuf, len);
    *rbuf += len;
    strbuf[len] = 0;
    FIELD(msg, *field, char *) = strbuf;
}

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
            switch (type) {
                case PLAN_MSG_SCHEMA_INT8:
                    wInt8(&wbuf, msg, schema + i);
                    break;
                case PLAN_MSG_SCHEMA_INT32:
                    wInt32(&wbuf, msg, schema + i);
                    break;
                case PLAN_MSG_SCHEMA_INT64:
                    wInt64(&wbuf, msg, schema + i);
                    break;
                case PLAN_MSG_SCHEMA_STR:
                    wStr(&wbuf, msg, schema + i);
                    break;
            }
        }

        enable >>= 1;
    }

    return buf;
}

void planMsgBufDecode(void *msg, const plan_msg_schema_t *_schema,
                      const void *buf)
{
    int schema_size = _schema->size;
    const plan_msg_schema_field_t *schema = _schema->schema;
    uint32_t header;
    unsigned char *rbuf;
    int type, i;

    rbuf = (unsigned char *)buf;
    header = ((uint32_t *)rbuf)[0];
    rbuf += 4;
    for (i = 0; i < schema_size; ++i){
        if (header & 0x1u){
            type = schema[i].type;
            switch (type) {
                case PLAN_MSG_SCHEMA_INT8:
                    rInt8(&rbuf, msg, schema + i);
                    break;
                case PLAN_MSG_SCHEMA_INT32:
                    rInt32(&rbuf, msg, schema + i);
                    break;
                case PLAN_MSG_SCHEMA_INT64:
                    rInt64(&rbuf, msg, schema + i);
                    break;
                case PLAN_MSG_SCHEMA_STR:
                    rStr(&rbuf, msg, schema + i);
                    break;
            }
        }

        header >>= 1;
    }
}
