#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

#define LIB_REDIS_PATH "./redis-server.so"

typedef int (*ENTRY)(int, char **, char *, char *);
typedef int (*SHUTDOWN)(int);
typedef char *(*GETDB)();

int main(int argc, char ** argv)
{
    void *handle;
    char *error;
    ENTRY entry = NULL;
    SHUTDOWN shutdown = NULL;
    GETDB getDb = NULL;

    char *db_addr = NULL;
    int ret;

    for(;;) {
        printf("load dynamic library\n");
        handle = dlopen(LIB_REDIS_PATH, RTLD_LAZY);
        if (!handle) {
            fprintf(stderr, "%s\n", dlerror());
            exit(EXIT_FAILURE);
        }

        entry = (ENTRY)dlsym(handle, "server_run");
        if (entry == NULL) {
            fprintf(stderr, "%s\n", dlerror());
            exit(EXIT_FAILURE);
        }
        entry(argc, argv, db_addr, "1234567890123456");

        getDb = (GETDB)dlsym(handle, "getDb");
        if (getDb == NULL) {
            fprintf(stderr, "%s\n", dlerror());
            exit(EXIT_FAILURE);
        }
        db_addr = getDb();

        shutdown = (SHUTDOWN)dlsym(handle, "prepareForShutdown");
        if (shutdown == NULL) {
            fprintf(stderr, "%s\n", dlerror());
            exit(EXIT_FAILURE);
        }
        shutdown(0);


        ret = dlclose(handle);
        if (ret != 0) {
            fprintf(stderr, "dlclose failed\n");
            exit(EXIT_FAILURE);
        }
        sleep(5);
    }
    exit(EXIT_SUCCESS);
}
