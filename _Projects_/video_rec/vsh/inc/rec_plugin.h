#ifndef __REC_PLUGIN_H__
#define __REC_PLUGIN_H__

#include <stdint.h>

typedef struct {
    uint32_t (*DoUnk0)(void);
    uint32_t (*start)(void);              /* RecStart */
    uint32_t (*stop)(void);               /* RecStop  */
    uint32_t (*close)(int32_t discard);   /* discard=1 descarta o arquivo */
    uint32_t (*getInfo)(int32_t param);   /* RecGetInfo */
    uint32_t (*setInfo)(void *arg, int32_t param); /* RecSetInfo */
    uint32_t (*setStartTime)(int32_t msec);
    uint32_t (*setEndTime)(int32_t msec);                  } rec_plugin_interface;

/* declarado extern — definido em main.c */
extern rec_plugin_interface *rec_interface;

#endif /* __REC_PLUGIN_H__ */
