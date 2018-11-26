#ifndef DUMMY_H
#define DUMMY_H
#include <linux/ioctl.h>
 
typedef struct
{
    int status, dignity, ego;
} dummy_arg_t;
 
#define DUMMY_GET_VARIABLES _IOR('q', 1, dummy_arg_t *)
#define DUMMY_CLR_VARIABLES _IO('q', 2)
#define DUMMY_SET_VARIABLES _IOW('q', 3, dummy_arg_t *)
 
#endif