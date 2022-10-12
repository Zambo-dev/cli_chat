#include "include.h"

pthread_mutex_t fd_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sock_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t errno_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t tdata_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_t pool[CONNLIMIT];
int running = 1;
