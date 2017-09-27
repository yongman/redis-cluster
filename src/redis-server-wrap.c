#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <signal.h>

#include <jemalloc/jemalloc.h>

#include "server.h"

#define LIB_REDIS_PATH "./redis-server.so"

#define malloc(size) je_malloc(size)
#define calloc(count,size) je_calloc(count,size)
#define realloc(ptr,size) je_realloc(ptr,size)
#define free(ptr) je_free(ptr)
#define mallocx(size,flags) je_mallocx(size,flags)
#define dallocx(ptr,flags) je_dallocx(ptr,flags)

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
    char *mem = NULL;

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

    for(;;) {
        fprintf(stderr, "load dynamic library\n");
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
