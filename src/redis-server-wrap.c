#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <signal.h>

#include "server.h"

#define LIB_REDIS_PATH "./redis-server.so"

typedef int (*ENTRY)(int, char **, struct wrapperContext *);
typedef int (*RELOAD)(int);
typedef char *(*GETDB)();

int main(int argc, char ** argv)
{
    struct wrapperContext ctx;
    void *handle;
    ENTRY entry = NULL;
    RELOAD reload = NULL;
    int ret;

    ctx.reload = 0;

    strncpy(ctx.hashseed, "1234567890123456", 16);
    ctx.reloadTimes = 0;

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
        entry(argc, argv, &ctx);

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
        ctx.reload = 1;
        ctx.reloadTimes++;
    }
    exit(EXIT_SUCCESS);
}
