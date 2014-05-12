#ifndef __FD_CONFIG_H__
#define __FD_CONFIG_H__

ifdef(`USE_MEMCHECK', `#define FD_MEMCHECK')
ifdef(`DEBUG', `#define FD_DEBUG')

#endif /* __FD_CONFIG_H__ */
