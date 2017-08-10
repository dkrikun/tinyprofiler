#pragma once

#ifndef USE_TINYPROFILER

static inline void profAlloc(size_t sample_count_per_thread) {}
static inline void profPrintAndFree() {}
static inline void profB(char * name) {}
static inline void profE(char * name) {}
static inline void profBmt(int tid, char * name) {}
static inline void profEmt(int tid, char * name) {}

#else // USE_TINYPROFILER

#ifndef TINYPROFILER_MAX_NUM_OF_THREADS
#define TINYPROFILER_MAX_NUM_OF_THREADS 4
#endif

#ifndef TINYPROFILER_MAX_STRING_LENGTH
#define TINYPROFILER_MAX_STRING_LENGTH 100
#endif

#ifndef TINYPROFILER_MAX_JSON_LINE_LENGTH
#define TINYPROFILER_MAX_JSON_LINE_LENGTH 10000
#endif

#ifndef TINYPROFILER_OUTPUT_STRING
#ifdef _WIN32
#define TINYPROFILER_OUTPUT_STRING(x) OutputDebugStringA(x)
#else
#define TINYPROFILER_OUTPUT_STRING(x) fprintf(stderr, x)
#endif
#endif

#ifndef TINYPROFILER_TIME_TO_TS
#ifdef _WIN32
#define TINYPROFILER_TIME_TO_TS(x) (unsigned long)(((double)(x) / (double)_tinyprofiler_perf_freq) * 1000000.0)
#else
#define TINYPROFILER_TIME_TO_TS(x) ((x) - _tinyprofiler_prof_time_start * 1000000UL)
#endif
#endif

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

struct _tinyprofiler_sample_t {
  unsigned long ts;
  int pid;
  int tid;
  char ph;
  char name[TINYPROFILER_MAX_STRING_LENGTH];
};

struct {
  struct _tinyprofiler_sample_t * s;
  size_t sample_count;
  int i;
} _prof_data[TINYPROFILER_MAX_NUM_OF_THREADS] = {0};

#ifdef _WIN32
unsigned long long _tinyprofiler_perf_freq = 0;
#else
unsigned long _tinyprofiler_prof_time_start = 0;
#endif

#ifdef _WIN32
static inline unsigned long long _prof_time() {
  unsigned long long counter;
  QueryPerformanceCounter((LARGE_INTEGER *)&counter);
  return counter;
}
#else
static inline unsigned long _prof_time() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000UL + tv.tv_usec;
}
#endif

static inline void _prof(int thread_id, char ph, unsigned long ts, int pid, int tid, size_t size, char * name) {
  int ti = thread_id;
  int i  = _prof_data[ti].i;
  _prof_data[ti].s[i].ts  = ts;
  _prof_data[ti].s[i].pid = pid;
  _prof_data[ti].s[i].tid = tid;
  _prof_data[ti].s[i].ph  = ph;
  memcpy(_prof_data[ti].s[i].name, name, size);
  _prof_data[ti].i += 1;
}

static inline void profAlloc(size_t sample_count_per_thread) {
#ifdef _WIN32
  BOOL query_perf_freq_fail = QueryPerformanceFrequency((LARGE_INTEGER *)&_tinyprofiler_perf_freq);
  assert(query_perf_freq_fail != 0);
#else
  struct timeval tv;
  gettimeofday(&tv, NULL);
  _tinyprofiler_prof_time_start = tv.tv_sec;
#endif
  for (int t = 0; t < TINYPROFILER_MAX_NUM_OF_THREADS; t += 1) {
    _prof_data[t].sample_count = sample_count_per_thread;
    _prof_data[t].s = (struct _tinyprofiler_sample_t *)calloc(sample_count_per_thread, sizeof(struct _tinyprofiler_sample_t));
  }
}

static inline void profPrintAndFree() {
  unsigned long self_t_begin = (unsigned long)_prof_time();
  size_t line_bytes = TINYPROFILER_MAX_JSON_LINE_LENGTH * sizeof(char);
  char * line = (char *)calloc(TINYPROFILER_MAX_JSON_LINE_LENGTH, sizeof(char));
  snprintf(line, line_bytes, "{\"traceEvents\":[{}\n"); TINYPROFILER_OUTPUT_STRING(line);
  for (int t = 0; t < TINYPROFILER_MAX_NUM_OF_THREADS; t += 1) {
    for (size_t i = 0; i < _prof_data[t].sample_count; i += 1) {
      if (_prof_data[t].s[i].ph) {
        snprintf(line, line_bytes, ",{\"ph\":\"%c\",\"ts\":%lu,\"pid\":%d,\"tid\":%d,\"name\":\"%s\"}\n",
                 _prof_data[t].s[i].ph,  TINYPROFILER_TIME_TO_TS(_prof_data[t].s[i].ts),
                 _prof_data[t].s[i].pid, _prof_data[t].s[i].tid, _prof_data[t].s[i].name);
        TINYPROFILER_OUTPUT_STRING(line);
      } else break;
    }
  }
  snprintf(line, line_bytes, ",{\"ph\":\"%c\",\"ts\":%lu,\"pid\":%d,\"tid\":%d,\"name\":\"%s\"}\n", 'B', TINYPROFILER_TIME_TO_TS((unsigned long)self_t_begin), 0, 0, __func__); TINYPROFILER_OUTPUT_STRING(line);
  snprintf(line, line_bytes, ",{\"ph\":\"%c\",\"ts\":%lu,\"pid\":%d,\"tid\":%d,\"name\":\"%s\"}\n", 'E', TINYPROFILER_TIME_TO_TS((unsigned long)_prof_time()), 0, 0, __func__); TINYPROFILER_OUTPUT_STRING(line);
  snprintf(line, line_bytes, "]}\n"); TINYPROFILER_OUTPUT_STRING(line);
  for (int t = 0; t < TINYPROFILER_MAX_NUM_OF_THREADS; t += 1) { free(_prof_data[t].s); }
  free(line);
}

#ifndef profB
#define profB(name) _prof(0, 'B', (unsigned long)_prof_time(), 0, 0, sizeof(name), name)
#endif

#ifndef profE
#define profE(name) _prof(0, 'E', (unsigned long)_prof_time(), 0, 0, sizeof(name), name)
#endif

#ifndef profBmt
#define profBmt(tid, name) _prof(tid, 'B', (unsigned long)_prof_time(), 0, tid, sizeof(name), name)
#endif

#ifndef profEmt
#define profEmt(tid, name) _prof(tid, 'E', (unsigned long)_prof_time(), 0, tid, sizeof(name), name)
#endif

#endif // USE_TINYPROFILER