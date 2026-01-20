#include <sys/resource.h>
#include <sys/time.h>
#include "htab.h"

int yak_verbose = 3;

static double yak_realtime0;

double yak_cputime(void)
{
	struct rusage r;
	getrusage(RUSAGE_SELF, &r);
	return r.ru_utime.tv_sec + r.ru_stime.tv_sec + 1e-6 * (r.ru_utime.tv_usec + r.ru_stime.tv_usec);
}

static inline double yak_realtime_core(void)
{
	struct timeval tp;
	struct timezone tzp;
	gettimeofday(&tp, &tzp);
	return tp.tv_sec + tp.tv_usec * 1e-6;
}

void yak_reset_realtime(void)
{
	yak_realtime0 = yak_realtime_core();
}

double yak_realtime(void)
{
	return yak_realtime_core() - yak_realtime0;
}

double yak_realtime_0(void)
{
	return yak_realtime_core();
}


long yak_peakrss(void)
{
	struct rusage r;
	getrusage(RUSAGE_SELF, &r);
#ifdef __linux__
	return r.ru_maxrss * 1024;
#else
	return r.ru_maxrss;
#endif
}

double yak_peakrss_in_gb(void)
{
	return yak_peakrss() / 1073741824.0;
}

double yak_cpu_usage(void)
{
	return (yak_cputime() + 1e-9) / (yak_realtime() + 1e-9);
}

double yak_current_rss(void)
{
	long rss = 0;
    
#if defined(__linux__) || defined(__unix__)
    // Linux/Unix系统使用/proc
    FILE* fp = fopen("/proc/self/statm", "r");
    if (fp != NULL) {
        if (fscanf(fp, "%*d %ld", &rss) == 1) {
            rss = rss * sysconf(_SC_PAGESIZE);
        }
        fclose(fp);
    }
#elif defined(__APPLE__) && defined(__MACH__)
    // macOS可以使用task_info
    struct task_basic_info t_info;
    mach_msg_type_number_t t_info_count = TASK_BASIC_INFO_COUNT;
    
    if (task_info(mach_task_self(), TASK_BASIC_INFO,
                  (task_info_t)&t_info, &t_info_count) == KERN_SUCCESS) {
        rss = t_info.resident_size;
    }
#elif defined(_WIN32)
    // Windows系统
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        rss = pmc.WorkingSetSize;
    }
#endif
    
    return rss/1073741824.0;
	
}