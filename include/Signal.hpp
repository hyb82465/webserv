#ifndef SIGNAL_HPP
#define SIGNAL_HPP

#include <csignal>

extern volatile sig_atomic_t g_running;

void handleSignal(int signal);

#endif