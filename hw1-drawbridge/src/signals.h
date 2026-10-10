#ifndef SIGNALS_H
#define SIGNALS_H

#include <signal.h>

void signals_install(volatile sig_atomic_t *flag);

#endif /* SIGNALS_H */
