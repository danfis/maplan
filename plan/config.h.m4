#ifndef __PLAN_CONFIG_H__
#define __PLAN_CONFIG_H__

ifdef(`DEBUG', `#define PLAN_DEBUG')
ifdef(`USE_ZMQ', `#define PLAN_ZMQ')
ifdef(`USE_NANOMSG', `#define PLAN_NANOMSG')

#endif /* __PLAN_CONFIG_H__ */
