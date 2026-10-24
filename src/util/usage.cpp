/****************************************************************************
  PackageName  [ util ]
  Synopsis     [ Report the runtime and memory usage ]
  Author       [ Design Verification Lab ]
  Copyright    [ Copyright(c) 2023 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#include "./usage.hpp"

#include <fmt/core.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#include <chrono>
#else
#include <sys/resource.h>
#include <sys/times.h>
#include <unistd.h>
#endif

#include <gsl/narrow>

using namespace dvlab::utils;

void Usage::reset_period() {
    _period_used_time = 0.0;
}

void Usage::start_tick() { _current_tick = _check_tick(); }

void Usage::end_tick() {
    auto const this_tick = _check_tick();
#if defined(_WIN32)
    auto const period = (this_tick - _current_tick) / 1000.0;
#else
    auto const period    = (this_tick - _current_tick) / double(sysconf(_SC_CLK_TCK));
#endif
    _period_used_time += period;
    _total_used_time += period;
    _current_memory = _check_memory() - _initial_memory;
}

void Usage::report(bool report_time, bool report_mem) {
    if (report_time) {
        fmt::println("Period time used : {:.4f} seconds", _period_used_time);
        fmt::println("Total time used  : {:.4f} seconds", _total_used_time);
        reset_period();
    }
    if (report_mem) {
        fmt::println("Total memory used: {:.4f} MiBs", _current_memory);
    }
}

double Usage::_check_memory() {
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS info;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info))) {
        return static_cast<double>(info.PeakWorkingSetSize) / double(1 << 20);
    }
    return 0;
#else
    rusage usage{};
    if (0 == getrusage(RUSAGE_SELF, &usage))
#ifdef __APPLE__
        // macOS reports memory usage in bytes
        return static_cast<double>(usage.ru_maxrss) / double(1 << 20);  // MiB // NOLINT(cppcoreguidelines-pro-type-union-access) : conform to POSIX
#else
        // Linux reports memory usage in kibibytes
        return static_cast<double>(usage.ru_maxrss) / double(1 << 10);  // KiB // NOLINT(cppcoreguidelines-pro-type-union-access) : conform to POSIX
#endif
    else
        return 0;
#endif
}
double Usage::_check_tick() {
#if defined(_WIN32)
    return static_cast<double>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
#else
    tms buffer{};
    times(&buffer);
    return static_cast<double>(buffer.tms_utime);
#endif
}

