#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <signal.h>

#include "server.h"

#define LIB_REDIS_PATH "./redis-server.so"

typedef int (*ENTRY)(int, char **, struct redisServer *, struct sharedObjectsStruct *, int, char *);
typedef int (*RELOAD)(int);
typedef char *(*GETDB)();

int main(int argc, char ** argv)
{
    void *handle;
    ENTRY entry = NULL;
    RELOAD reload = NULL;
    GETDB getDb = NULL;

    char *db_addr = NULL;
    int ret;
    struct redisServer *server;
    struct sharedObjectsStruct *shared;
    int isreload = 0;

    // alloc mem for save server struct
    server = (struct redisServer *)malloc(sizeof(struct redisServer));
    shared = (struct sharedObjectsStruct *)malloc(sizeof(struct sharedObjectsStruct));

    for(;;) {
        printf("load dynamic library\n");
        handle = dlopen(LIB_REDIS_PATH, RTLD_LAZY);
        if (!handle) {
            fprintf(stderr, "%s\n", dlerror());
            exit(EXIT_FAILURE);
        }

        entry = (ENTRY)dlsym(handle, "serverRun");
        if (entry == NULL) {
            fprintf(stderr, "%s\n", dlerror());
            exit(EXIT_FAILURE);
        }
        entry(argc, argv, server, shared, isreload, "1234567890123456");

        reload = (RELOAD)dlsym(handle, "prepareForReload");
        if (reload == NULL) {
            fprintf(stderr, "%s\n", dlerror());
            exit(EXIT_FAILURE);
        }
        reload(0);

        ret = dlclose(handle);
        if (ret != 0) {
            fprintf(stderr, "dlclose failed\n");
            exit(EXIT_FAILURE);
        }
        isreload = 1;
    }
    free(server);
    free(shared);
    exit(EXIT_SUCCESS);
}
