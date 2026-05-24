//创建子线程

#include<pthread.h>
pthread_t pthread_self(void);
int pthread_creat(pthread_t *thread,const pthread_attr_t* attr,void*(*start_routine)(void*),void*arg);