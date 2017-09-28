#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <signal.h>
#include <unistd.h>

#include <jemalloc/jemalloc.h>

#include "server.h"

#define LIB_REDIS_PATH "./redis-server.so"
#define LIB_REDIS_BUSY_PATH "./redis-server.so.busy"

#define malloc(size) je_malloc(size)
#define free(ptr) je_free(ptr)

typedef int (*ENTRY)(int, char **, struct wrapperContext *);
typedef int (*RELOAD)(int);

int main(int argc, char ** argv)
{
    struct wrapperContext ctx;
    void *handle;
    ENTRY entry = NULL;
    RELOAD reload = NULL;
    int ret;
    char *mem = NULL;
    int err = 0;

    ctx.reload = 0;

    strncpy(ctx.hashseed, "1234567890123456", 16);
    ctx.reloadTimes = 0;

    fprintf(stderr, "jemalloc init\n");
    mem = malloc(1);
    if (mem == NULL) {
        fprintf(stderr, "memory alloc failed\n");
        exit(EXIT_FAILURE);
    }
    free(mem);

    /* check so file */
    if (access(LIB_REDIS_BUSY_PATH, R_OK) != 0) {
        err = errno;
        fprintf(stderr, "file %s access error:%s\n", LIB_REDIS_BUSY_PATH, strerror(err));

        if (access(LIB_REDIS_PATH, R_OK) != 0) {
            err = errno;
            fprintf(stderr, "file %s access error:%s\n", LIB_REDIS_PATH, strerror(err));
            fprintf(stderr, "dynamic library file not exists, please check.\n");
            exit(EXIT_FAILURE);
        } else {
            /* rename so file to busy */
            if (rename(LIB_REDIS_PATH, LIB_REDIS_BUSY_PATH) != 0) {
                err = errno;
                fprintf(stderr, "rename file failed:%s\n", strerror(err));
                exit(EXIT_FAILURE);
            }
        }
    }


    for(;;) {
        fprintf(stderr, "load dynamic library\n");
        handle = dlopen(LIB_REDIS_BUSY_PATH, RTLD_LAZY);
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

        /* use new so file */
        if (access(LIB_REDIS_PATH, R_OK) == 0) {
            /* rename so file */
            if (rename(LIB_REDIS_PATH, LIB_REDIS_BUSY_PATH) != 0) {
                err = errno;
                fprintf(stderr, "rename %s to %s failed: %s", LIB_REDIS_PATH,
                        LIB_REDIS_BUSY_PATH, strerror(err));
                fprintf(stderr, "use old so file instead");
            }
        } else {
            fprintf(stderr, "no newer so file found, using old so file instead");
        }
    }
    exit(EXIT_SUCCESS);
}
