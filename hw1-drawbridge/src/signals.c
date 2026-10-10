#include "signals.h"
#include <string.h>

static volatile sig_atomic_t *g_flag = NULL;

static void handle_signal(int sig) {
    (void) sig;
    if (g_flag) *g_flag = 1;
}

void signals_install(volatile sig_atomic_t *flag) {
    g_flag = flag;

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);

    sa.sa_flags = 0;

    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}
