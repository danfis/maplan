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

#ifndef __PLAN_MSG_SCHEMA_H__
#define __PLAN_MSG_SCHEMA_H__

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define PLAN_MSG_SCHEMA_INT32 0
#define PLAN_MSG_SCHEMA_INT64 1
#define PLAN_MSG_SCHEMA_STR   2
#define PLAN_MSG_SCHEMA_BYTES 3

struct _plan_msg_schema_field_t {
    int type;
    int offset;
};
typedef struct _plan_msg_schema_field_t plan_msg_schema_field_t;

struct _plan_msg_schema_t {
    int size;
    const plan_msg_schema_field_t *schema;
};
typedef struct _plan_msg_schema_t plan_msg_schema_t;

#define _PLAN_MSG_SCHEMA_OFFSET(TYPE, MEMBER) offsetof(TYPE, MEMBER)
/*#define _PLAN_MSG_SCHEMA_OFFSET(TYPE, MEMBER) ((size_t) &((TYPE * *)0)->MEMBER)*/

#define PLAN_MSG_SCHEMA_BEGIN(name) \
    static plan_msg_schema_field_t __xxx_##name[] = {

#define PLAN_MSG_SCHEMA_ADD(base_struct, member, type) \
    { PLAN_MSG_SCHEMA_##type, \
      _PLAN_MSG_SCHEMA_OFFSET(base_struct, member) },

#define PLAN_MSG_SCHEMA_END(name) \
    }; \
    static plan_msg_schema_t name = { \
        sizeof(__xxx_##name) / sizeof(plan_msg_schema_field_t), \
        (plan_msg_schema_field_t *)&__xxx_##name \
    };


struct _plan_msg_buf_t {
    uint32_t header;
    int bufsize;
    unsigned char *buf;
    int buf_alloc;
};
typedef struct _plan_msg_buf_t plan_msg_buf_t;

void planMsgBufEncode(plan_msg_buf_t *buf, const plan_msg_schema_t *schema,
                      uint32_t enable_flags, const void *msg_struct);
void planMsgBufDecode(void *msg_struct, const plan_msg_schema_t *schema,
                      const void *buf);
void planMsgBufFree(plan_msg_buf_t *buf);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_MSG_SCHEMA_H__ */
