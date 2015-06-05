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

#define _PLAN_MSG_SCHEMA_INT8  0
#define _PLAN_MSG_SCHEMA_INT32 1
#define _PLAN_MSG_SCHEMA_INT64 2
#define _PLAN_MSG_SCHEMA_ARR_BASE 10

struct _plan_msg_schema_field_t {
    int type;
    int offset;
    int size_offset;
};
typedef struct _plan_msg_schema_field_t plan_msg_schema_field_t;

struct _plan_msg_schema_t {
    int header_offset;
    int size;
    const plan_msg_schema_field_t *schema;
};
typedef struct _plan_msg_schema_t plan_msg_schema_t;

#define _PLAN_MSG_SCHEMA_OFFSET(TYPE, MEMBER) offsetof(TYPE, MEMBER)
/*#define _PLAN_MSG_SCHEMA_OFFSET(TYPE, MEMBER) ((size_t) &((TYPE * *)0)->MEMBER)*/

#define PLAN_MSG_SCHEMA_BEGIN(name) \
    static plan_msg_schema_field_t ___##name[] = {

#define PLAN_MSG_SCHEMA_ADD(base_struct, member, type) \
    { _PLAN_MSG_SCHEMA_##type, \
      _PLAN_MSG_SCHEMA_OFFSET(base_struct, member), -1 },

#define PLAN_MSG_SCHEMA_ADD_ARR(base_struct, member, member_len, type) \
    { _PLAN_MSG_SCHEMA_ARR_BASE + _PLAN_MSG_SCHEMA_##type, \
      _PLAN_MSG_SCHEMA_OFFSET(base_struct, member), \
      _PLAN_MSG_SCHEMA_OFFSET(base_struct, member_len) },

#define PLAN_MSG_SCHEMA_END(name, base_struct, header_member) \
    }; \
    static plan_msg_schema_t name = { \
        _PLAN_MSG_SCHEMA_OFFSET(base_struct, header_member), \
        sizeof(___##name) / sizeof(plan_msg_schema_field_t), \
        (plan_msg_schema_field_t *)&___##name \
    };


unsigned char *planMsgBufEncode(const void *msg_struct,
                                const plan_msg_schema_t *schema,
                                int *size);
void planMsgBufDecode(void *msg_struct, const plan_msg_schema_t *schema,
                      const void *buf);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __PLAN_MSG_SCHEMA_H__ */
