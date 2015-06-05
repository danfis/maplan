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

#include <stdio.h>
#include <boruvka/alloc.h>
#include <plan/msg_schema.h>

static int byte_size[20] = {
    /* _PLAN_MSG_SCHEMA_INT8 */  1,
    /* _PLAN_MSG_SCHEMA_INT32 */ 4,
    /* _PLAN_MSG_SCHEMA_INT64 */ 8,
    /* */ 0,
    /* */ 0,
    /* */ 0,
    /* */ 0,
    /* */ 0,
    /* */ 0,
    /* _PLAN_MSG_SCHEMA_MSG */  0,
/* ARR: */
    /* _PLAN_MSG_SCHEMA_INT8 */  2,
    /* _PLAN_MSG_SCHEMA_INT32 */ 2,
    /* _PLAN_MSG_SCHEMA_INT64 */ 2,
    /* */ 0,
    /* */ 0,
    /* */ 0,
    /* */ 0,
    /* */ 0,
    /* */ 0,
    /* _PLAN_MSG_SCHEMA_MSG */  2,
};

#define HEADER(msg_struct, schema) \
    (*(uint32_t *)(((unsigned char *)(msg_struct)) + (schema)->header_offset))
#define FIELD_PTR(msg_struct, offset) \
    (void *)(((unsigned char *)(msg_struct)) + (offset))
#define FIELD(msg_struct, offset, type) \
    (*(type *)FIELD_PTR((msg_struct), (offset)))

static void encode(unsigned char **wbuf, const void *msg,
                   const plan_msg_schema_t *_schema);
static void decode(unsigned char **rbuf, void *msg,
                   const plan_msg_schema_t *_schema);

_bor_inline void wField(unsigned char **wbuf, const void *msg, int offset,
                        int size)
{
    void *v = FIELD_PTR(msg, offset);
    memcpy(*wbuf, v, size);
    *wbuf += size;
}

_bor_inline void wArr(unsigned char **wbuf, const void *msg, int offset,
                      int size_offset, int size)
{
    int len;
    uint16_t len16;

    len16 = len = FIELD(msg, size_offset, int);
    memcpy(*wbuf, &len16, 2);
    *wbuf += 2;
    memcpy(*wbuf, FIELD(msg, offset, void *), size * len);
    *wbuf += size * len;
}

_bor_inline void wMsgArr(unsigned char **wbuf, const void *msg, int offset,
                         int size_offset, const plan_msg_schema_t *sub_schema)
{
    int i, len, size;
    uint16_t len16;
    void *submsg;

    size = sub_schema->struct_bytesize;
    len16 = len = FIELD(msg, size_offset, int);
    memcpy(*wbuf, &len16, 2);
    *wbuf += 2;

    submsg = FIELD(msg, offset, void *);
    for (i = 0; i < len; ++i){
        encode(wbuf, submsg, sub_schema);
        submsg = ((char *)submsg) + size;
    }
}


_bor_inline void rField(unsigned char **rbuf, void *msg, int off, int size)
{
    memcpy(FIELD_PTR(msg, off), *rbuf, size);
    *rbuf += size;
}

_bor_inline void rArr(unsigned char **rbuf, void *msg, int off,
                      int size_off, int size)
{
    int len;
    void *buf;

    len = *(uint16_t *)*rbuf;
    *rbuf += 2;

    buf = BOR_ALLOC_ARR(char, size * len);
    memcpy(buf, *rbuf, size * len);
    *rbuf += size * len;

    FIELD(msg, size_off, int) = len;
    FIELD(msg, off, void *) = buf;
}

_bor_inline void rMsgArr(unsigned char **rbuf, void *msg, int off,
                         int size_off, const plan_msg_schema_t *sub_schema)
{
    int i, len, size;
    void *buf, *wbuf;

    size = sub_schema->struct_bytesize;
    len = *(uint16_t *)*rbuf;
    *rbuf += 2;

    buf = BOR_ALLOC_ARR(char, size * len);
    wbuf = buf;
    for (i = 0; i < len; ++i){
        decode(rbuf, wbuf, sub_schema);
        wbuf = (((char *)wbuf) + size);
    }

    FIELD(msg, size_off, int) = len;
    FIELD(msg, off, void *) = buf;
}


static int wBufSize(const void *msg, const plan_msg_schema_t *_schema)
{
    int schema_size = _schema->size;
    const plan_msg_schema_field_t *schema = _schema->schema;
    uint32_t enable = HEADER(msg, _schema);
    int bufsize, i, j, siz, type;
    const void *sub;

    bufsize = sizeof(uint32_t);
    for (i = 0; i < schema_size; ++i){
        if (enable & 0x1u){
            type = schema[i].type;
            bufsize += byte_size[type];

            if (type == _PLAN_MSG_SCHEMA_MSG){
                sub = FIELD_PTR(msg, schema[i].offset);
                bufsize += wBufSize(sub, schema[i].sub);

            }else if (type == _PLAN_MSG_SCHEMA_MSG_ARR){
                siz = FIELD(msg, schema[i].size_offset, int);
                sub = FIELD(msg, schema[i].offset, void *);
                for (j = 0; j < siz; ++j){
                    bufsize += wBufSize(sub, schema[i].sub);
                    sub = ((char *)sub) + schema[i].sub->struct_bytesize;
                }

            }else if (type >= _PLAN_MSG_SCHEMA_ARR_BASE){
                siz = FIELD(msg, schema[i].size_offset, int);
                siz *= byte_size[type - _PLAN_MSG_SCHEMA_ARR_BASE];
                bufsize += siz;
            }
        }

        enable >>= 1;
    }

    return bufsize;
}

static void encode(unsigned char **wbuf, const void *msg,
                   const plan_msg_schema_t *_schema)
{
    int schema_size = _schema->size;
    const plan_msg_schema_field_t *schema = _schema->schema;
    uint32_t header = HEADER(msg, _schema);
    const void *sub_msg;
    int i, type;

    memcpy(*wbuf, &header, 4);
    *wbuf += 4;
    for (i = 0; i < schema_size; ++i){
        if (header & 0x1u){
            type = schema[i].type;
            if (type == _PLAN_MSG_SCHEMA_MSG){
                sub_msg = FIELD_PTR(msg, schema[i].offset);
                encode(wbuf, sub_msg, schema[i].sub);

            }else if (type == _PLAN_MSG_SCHEMA_MSG_ARR){
                wMsgArr(wbuf, msg, schema[i].offset, schema[i].size_offset,
                        schema[i].sub);

            }else if (type < _PLAN_MSG_SCHEMA_ARR_BASE){
                wField(wbuf, msg, schema[i].offset, byte_size[type]);

            }else{
                wArr(wbuf, msg, schema[i].offset, schema[i].size_offset,
                     byte_size[type - _PLAN_MSG_SCHEMA_ARR_BASE]);
            }
        }

        header >>= 1;
    }
}

unsigned char *planMsgBufEncode(const void *msg,
                                const plan_msg_schema_t *_schema,
                                int *size)
{
    unsigned char *buf, *wbuf;
    int bufsize;

    bufsize = wBufSize(msg, _schema);
    buf = BOR_ALLOC_ARR(unsigned char, bufsize);
    *size = bufsize;

    wbuf = buf;
    encode(&wbuf, msg, _schema);
    return buf;
}

static void decode(unsigned char **rbuf, void *msg,
                   const plan_msg_schema_t *_schema)
{
    int schema_size = _schema->size;
    const plan_msg_schema_field_t *schema = _schema->schema;
    uint32_t header;
    void *sub_msg;
    int type, i;

    header = ((uint32_t *)*rbuf)[0];
    *rbuf += 4;
    for (i = 0; i < schema_size; ++i){
        if (header & 0x1u){
            type = schema[i].type;

            if (type == _PLAN_MSG_SCHEMA_MSG){
                sub_msg = FIELD_PTR(msg, schema[i].offset);
                decode(rbuf, sub_msg, schema[i].sub);

            }else if (type == _PLAN_MSG_SCHEMA_MSG_ARR){
                rMsgArr(rbuf, msg, schema[i].offset, schema[i].size_offset,
                        schema[i].sub);

            }else if (type < _PLAN_MSG_SCHEMA_ARR_BASE){
                rField(rbuf, msg, schema[i].offset, byte_size[type]);

            }else{
                rArr(rbuf, msg, schema[i].offset, schema[i].size_offset,
                     byte_size[type - _PLAN_MSG_SCHEMA_ARR_BASE]);
            }
        }

        header >>= 1;
    }
}

void planMsgBufDecode(void *msg, const plan_msg_schema_t *_schema,
                      const void *buf)
{
    unsigned char *rbuf;
    rbuf = (unsigned char *)buf;
    decode(&rbuf, msg, _schema);
}
