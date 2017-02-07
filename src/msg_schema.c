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
#include <endian.h>
#include <boruvka/alloc.h>
#include <plan/msg_schema.h>

#define ARR_LEN_TYPE uint32_t
#define ARR_LEN_TYPE_SIZE sizeof(ARR_LEN_TYPE)
#define ARR_LEN_TO_LE htole32
#define ARR_LEN_TO_H le32toh

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
    /* _PLAN_MSG_SCHEMA_INT8 */  ARR_LEN_TYPE_SIZE,
    /* _PLAN_MSG_SCHEMA_INT32 */ ARR_LEN_TYPE_SIZE,
    /* _PLAN_MSG_SCHEMA_INT64 */ ARR_LEN_TYPE_SIZE,
    /* */ 0,
    /* */ 0,
    /* */ 0,
    /* */ 0,
    /* */ 0,
    /* */ 0,
    /* _PLAN_MSG_SCHEMA_MSG */  ARR_LEN_TYPE_SIZE,
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

#ifdef BOR_LITTLE_ENDIAN
# define SET_ENDIAN(header) (header) |= (0x1u << 31u)
# define CHECK_ENDIAN(header) (((header) & (0x1u << 31u)) == (0x1u << 31u))
# define CONV_END_int32_t be32toh
# define CONV_END_int64_t be64toh
#endif

#ifdef BOR_BIG_ENDIAN
# define SET_ENDIAN(header) (header) &= ~(0x1u << 31u)
# define CHECK_ENDIAN(header) (((header) & (0x1u << 31u)) == 0u)
# define CONV_END_int32_t le32toh
# define CONV_END_int64_t le64toh
#endif

#define CONV_ENDIAN(msg, offset, type) \
    FIELD((msg), (offset), type) = CONV_END_##type(FIELD((msg), (offset), type))

#if !defined(BOR_LITTLE_ENDIAN) && !defined(BOR_BIG_ENDIAN)
# error "Cannot determie endianness!!"
#endif

_bor_inline void wHeader(unsigned char **wbuf, uint32_t header)
{
    SET_ENDIAN(header);
#ifdef BOR_BIG_ENDIAN
    header = htole32(header);
#endif
    *((uint32_t *)*wbuf) = header;
    *wbuf += 4;
}

_bor_inline uint32_t rHeader(unsigned char **rbuf)
{
    uint32_t header;
    header = *(uint32_t *)*rbuf;
#ifdef BOR_BIG_ENDIAN
    header = le32toh(header);
#endif
    *rbuf += 4;
    return header;
}

_bor_inline void wArrLen(unsigned char **wbuf, int len)
{
    ARR_LEN_TYPE len32 = len;

#ifdef BOR_BIG_ENDIAN
    len32 = ARR_LEN_TO_LE(len32);
#endif
    memcpy(*wbuf, &len32, ARR_LEN_TYPE_SIZE);
    *wbuf += ARR_LEN_TYPE_SIZE;
}

_bor_inline int rArrLen(unsigned char **rbuf)
{
    int len;
#ifdef BOR_BIG_ENDIAN
    len = ARR_LEN_TO_H(*(ARR_LEN_TYPE *)*rbuf);
#else
    len = *(ARR_LEN_TYPE *)*rbuf;
#endif
    *rbuf += ARR_LEN_TYPE_SIZE;
    return len;
}

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

    len = FIELD(msg, size_offset, int);
    wArrLen(wbuf, len);
    memcpy(*wbuf, FIELD(msg, offset, void *), size * len);
    *wbuf += size * len;
}

_bor_inline void wMsgArr(unsigned char **wbuf, const void *msg, int offset,
                         int size_offset, const plan_msg_schema_t *sub_schema)
{
    int i, len, size;
    void *submsg;

    size = sub_schema->struct_bytesize;
    len = FIELD(msg, size_offset, int);
    wArrLen(wbuf, len);

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

    len = rArrLen(rbuf);
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
    len = rArrLen(rbuf);

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

    wHeader(wbuf, header);
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

void *planMsgEncode(const void *msg,
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

int planMsgEncode2(const void *msg,
                   const plan_msg_schema_t *_schema,
                   void *buf, int *size)
{
    unsigned char *wbuf;
    int bufsize;

    bufsize = wBufSize(msg, _schema);
    if (bufsize > *size){
        *size = bufsize;
        return -1;
    }

    *size = bufsize;
    wbuf = (unsigned char *)buf;
    encode(&wbuf, msg, _schema);
    return 0;
}

_bor_inline void convEndian(int type, void *msg, int offset)
{
    switch (type) {
        case _PLAN_MSG_SCHEMA_INT32:
            CONV_ENDIAN(msg, offset, int32_t);
            break;
        case _PLAN_MSG_SCHEMA_INT64:
            CONV_ENDIAN(msg, offset, int64_t);
            break;
    }
}

_bor_inline void convEndianArr(int type, void *msg, int off, int size_off)
{
    int len, i, size;
    void *arr;

    type -= _PLAN_MSG_SCHEMA_ARR_BASE;
    if (type != _PLAN_MSG_SCHEMA_INT32
            && type != _PLAN_MSG_SCHEMA_INT64)
        return;

    size = byte_size[type];
    len = FIELD(msg, size_off, int);
    arr = FIELD(msg, off, void *);
    for (i = 0; i < len; ++i){
        convEndian(type, arr, 0);
        arr = ((char *)arr) + size;
    }
}

static void changeEndianness(void *msg, uint32_t header,
                             const plan_msg_schema_t *_schema)
{
    int schema_size = _schema->size;
    const plan_msg_schema_field_t *schema = _schema->schema;
    int i, type;

    for (i = 0; i < schema_size; ++i){
        if (header & 0x1u){
            type = schema[i].type;

            if (type < _PLAN_MSG_SCHEMA_ARR_BASE){
                convEndian(type, msg, schema[i].offset);

            }else{
                convEndianArr(type, msg, schema[i].offset,
                              schema[i].size_offset);
            }
        }

        header >>= 1;
    }
}

static void decode(unsigned char **rbuf, void *msg,
                   const plan_msg_schema_t *_schema)
{
    int schema_size = _schema->size;
    const plan_msg_schema_field_t *schema = _schema->schema;
    uint32_t header, enable;
    void *sub_msg;
    int type, i;

    bzero(msg, _schema->struct_bytesize);
    enable = header = rHeader(rbuf);
    FIELD(msg, _schema->header_offset, uint32_t) = header & ~(0x1 << 31);
    for (i = 0; i < schema_size; ++i){
        if (enable & 0x1u){
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

        enable >>= 1;
    }

    if (!CHECK_ENDIAN(header)){
        changeEndianness(msg, header, _schema);
    }
}

void planMsgDecode(void *msg, const plan_msg_schema_t *_schema,
                      const void *buf)
{
    unsigned char *rbuf;
    rbuf = (unsigned char *)buf;
    decode(&rbuf, msg, _schema);
}
