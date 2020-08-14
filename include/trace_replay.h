/****************************************************************************
 * Block I/O Trace Replayer 
 * Yongseok Oh (ysoh@uos.ac.kr) 2013 - 2014

 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/

#ifndef _TRACE_REPLAY_H
#define _TRACE_REPLAY_H

#include <stdio.h>
#include <libaio.h>
#include <flist.h>

#define USE_MAINWORKER 0

#define LINUX 0
#define TEST_OS (LINUX)

#define KB (1024)
#define MB (1024 * 1024)
#define GB (1024 * 1024 * 1024)
#define MAX_QDEPTH (128 * 16)
#define MAX_THREADS 512
#define STR_SIZE 128

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif
#define SECTOR_SIZE 512
#define SPP (PAGE_SIZE / SECTOR_SIZE)
#define MAX_BYTES (1024 * 1024 * 1024)

#define PAGE_TO_MB(x) (x * PAGE_SIZE / MB)

struct io_stat_t {
        pthread_spinlock_t stat_lock;
        double latency_sum;
        double latency_sum_sqr;
        double latency_min;
        double latency_max;
        unsigned int latency_count;
        unsigned long long total_operations;
        unsigned long long total_bytes;
        unsigned long long total_rbytes;
        unsigned long long total_wbytes;
        unsigned long long cur_bytes;
        unsigned long long cur_rbytes;
        unsigned long long cur_wbytes;
        unsigned long long total_error_bytes;
        struct timeval start_time, end_time;
        double execution_time;
        int trace_repeat_count;
        double time_diff;
        unsigned int time_diff_cnt;
};

struct trace_io_req {
#if 0
	char line[201];
#else
        double arrival_time;
        int devno;
        int blkno;
        int bcount;
        int flags;
#endif
};

struct trace_info_t {
        pthread_spinlock_t trace_lock;
        FILE *trace_fp;
        int trace_buf_size;
        struct trace_io_req *trace_buf;
        int trace_io_cnt;
        int trace_io_cur;
        int trace_io_issue_count;
        char tracename[STR_SIZE];
        char filename[STR_SIZE];
        int fd;
        int synthetic;
        long long wanted_io_count;
        int utilization; // % Percent
        int working_set_size; // in MB
        int working_set_pages; // in pages
        int io_size; // in bytes
        int io_pages; // in pages

        int synth_rand;
        int synth_read;
        int synth_write;
        int synth_mixed;

        int trace_repeat_count;
        int trace_repeat_num;
        long long total_capacity;
        long long total_pages;
        long long total_sectors;
        long long start_partition;
        long long start_page;
        double trace_timescale;
        double timeout;
};

struct thread_info_t {
        int tid;

        //	struct flist_head queue;
        pthread_mutex_t mutex;
        pthread_cond_t cond_main, cond_sub;
        io_context_t io_ctx;
        struct io_event events[MAX_QDEPTH];

        int queue_depth;
        int queue_count;
        int active_count;
        int fd;
        int fsync_period;

        struct io_job *th_jobs[MAX_QDEPTH];
        void *th_buf[MAX_QDEPTH];
        int buf_cur;

        struct io_stat_t io_stat;

        struct trace_info_t *trace;

        int done;
};

struct io_job {
        struct iocb iocb;
        struct flist_head list;
        struct timeval start_time, stop_time;
        long long offset; // in bytes
        size_t bytes;
        int rw; // is read
        char *buf;
};

#define BASE_KEY_PATHNAME_LEN 33
#define MSGQ_KEY_PATHNAME "/tmp/trace_replay_msgq"
#define SHM_KEY_PATHNAME "/tmp/trace_replay_shm"
#define SEM_KEY_PATHNAME "/tmp/trace_replay_sem"
#define PROJECT_ID 'M'
#define PROJECT_PERM 0640

enum realtime_log_type { TIMEOUT = 0, WANTED_IO_COUNT, NONE, FIN };

struct realtime_log {
        int type;
        double time;
        double remaining;
        double remaining_percentage;
        double avg_bw;
        double cur_bw;
        double lat;
        double time_diff;
};

struct realtime_msg {
        long mtype;
        struct realtime_log log;
};

struct trace {
        double start_partition; // in GB
        double total_size; // in GB
        long long start_page;
        long long total_pages;
};

struct config {
        int qdepth;
        double timeout;
        int nr_trace;
        int nr_thread;
        int per_thread;
        char result_file[201];
        struct trace traces[MAX_THREADS];
};

struct synthetic {
        int working_set_size; // in MB
        int utilization; // % Percent
        int touched_working_set_size; // in MB
        int io_size; // in KB
};

struct trace_stat {
        double exec_time;
        double avg_lat;
        double avg_lat_var;
        double lat_min;
        double lat_max;
        double iops;
        double total_bw; // MB/s
        double read_bw; // MB/s
        double write_bw; // MB/s
        double total_traffic; // in MB
        double read_traffic; // in MB
        double write_traffic; // in MB
        double read_ratio; // % Percent
        double total_avg_req_size; // in KB
        double read_avg_req_size; // in KB
        double write_avg_req_size; // in KB
};

struct trace_result {
        char name[STR_SIZE];

        int issynthetic;
        struct synthetic synthetic;

        struct trace_stat stats;

        int trace_reset_count;
};

struct aggr_result {
        struct trace_stat stats;
};

struct result {
        struct trace_result per_trace[MAX_THREADS];
        struct aggr_result aggr_result;
};

struct total_results {
        struct config config;
        struct result results;
};

#ifndef __s8
typedef char __s8;
#endif
typedef short __s16;
typedef int __s32;

#if TEST_OS == LINUX
typedef long long __s64;
#endif

typedef unsigned char __u8;
typedef unsigned short __u16;
typedef unsigned int __u32;

#if TEST_OS == LINUX
typedef unsigned long long __u64;
#endif

long long get_total_bytes(int nr_trace, int nr_thread);
void synthetic_mix(struct trace_info_t *trace);

#endif
