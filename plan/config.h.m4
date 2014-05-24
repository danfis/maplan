#ifndef __PLAN_CONFIG_H__
#define __PLAN_CONFIG_H__

ifdef(`USE_MEMCHECK', `#define FD_MEMCHECK')
ifdef(`DEBUG', `#define FD_DEBUG')

#endif /* __PLAN_CONFIG_H__ */
