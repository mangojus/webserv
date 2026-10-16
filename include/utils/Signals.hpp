#pragma once

#include <csignal>

extern volatile sig_atomic_t g_sig_stop;
extern volatile sig_atomic_t g_sig_child_dead;

void setup_signal(void);
