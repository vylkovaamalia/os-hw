#include "ipc.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

sem_t *g_mutex     = NULL;
sem_t *g_log_mutex = NULL;
sem_t *g_event     = NULL;
sem_t *g_mech      = NULL;

static SharedState *g_shm     = NULL;
static size_t       g_shm_len = 0;

static sem_t *open_sem(const char *name, unsigned value) {
    sem_t *s = sem_open(name, O_CREAT | O_RDWR, 0644, value);
    if (s == SEM_FAILED) {
        fprintf(stderr, "sem_open(%s): %s\n", name, strerror(errno));
        return NULL;
    }
    return s;
}

SharedState *ipc_setup(void) {
    shm_unlink(SHM_NAME);
    sem_unlink(SEM_MUTEX_NAME);
    sem_unlink(SEM_LOG_NAME);
    sem_unlink(SEM_EVENT_NAME);
    sem_unlink(SEM_MECH_NAME);

    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0644);
    if (fd < 0) {
        fprintf(stderr, "shm_open: %s\n", strerror(errno));
        return NULL;
    }
    g_shm_len = sizeof(SharedState);
    if (ftruncate(fd, (off_t)g_shm_len) < 0) {
        fprintf(stderr, "ftruncate: %s\n", strerror(errno));
        close(fd);
        shm_unlink(SHM_NAME);
        return NULL;
    }
    void *p = mmap(NULL, g_shm_len, PROT_READ | PROT_WRITE,
                   MAP_SHARED, fd, 0);
    close(fd);
    if (p == MAP_FAILED) {
        fprintf(stderr, "mmap: %s\n", strerror(errno));
        shm_unlink(SHM_NAME);
        return NULL;
    }
    g_shm = (SharedState *)p;
    memset(g_shm, 0, g_shm_len);

    g_mutex     = open_sem(SEM_MUTEX_NAME, 1);
    g_log_mutex = open_sem(SEM_LOG_NAME,   1);
    g_event     = open_sem(SEM_EVENT_NAME, 0);
    g_mech      = open_sem(SEM_MECH_NAME,  0);

    if (!g_mutex || !g_log_mutex || !g_event || !g_mech) {
        ipc_cleanup();
        return NULL;
    }
    return g_shm;
}

SharedState *ipc_attach(void) {
    if (g_shm) return g_shm;

    int fd = shm_open(SHM_NAME, O_RDWR, 0);
    if (fd < 0) {
        fprintf(stderr, "shm_open(attach): %s\n", strerror(errno));
        return NULL;
    }
    void *p = mmap(NULL, sizeof(SharedState),
                   PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (p == MAP_FAILED) {
        fprintf(stderr, "mmap(attach): %s\n", strerror(errno));
        return NULL;
    }
    g_shm = (SharedState *)p;

    if (!g_mutex)     g_mutex     = sem_open(SEM_MUTEX_NAME, 0);
    if (!g_log_mutex) g_log_mutex = sem_open(SEM_LOG_NAME,   0);
    if (!g_event)     g_event     = sem_open(SEM_EVENT_NAME, 0);
    if (!g_mech)      g_mech      = sem_open(SEM_MECH_NAME,  0);

    return g_shm;
}

void ipc_cleanup(void) {
    if (g_shm) {
        munmap(g_shm, sizeof(SharedState));
        g_shm = NULL;
    }
    if (g_mutex)     { sem_close(g_mutex);     g_mutex     = NULL; }
    if (g_log_mutex) { sem_close(g_log_mutex); g_log_mutex = NULL; }
    if (g_event)     { sem_close(g_event);     g_event     = NULL; }
    if (g_mech)      { sem_close(g_mech);      g_mech      = NULL; }

    shm_unlink(SHM_NAME);
    sem_unlink(SEM_MUTEX_NAME);
    sem_unlink(SEM_LOG_NAME);
    sem_unlink(SEM_EVENT_NAME);
    sem_unlink(SEM_MECH_NAME);
}

void mutex_lock(void) {
    while (sem_wait(g_mutex) < 0 && errno == EINTR) ;
}

void mutex_unlock(void) {
    sem_post(g_mutex);
}