/**
 * @copyright "Container Tracer" which executes the container performance mesurements
 * Copyright (C) 2020 SuhoSon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * @file docker-serializer.c
 * @brief To make structure to JSON format.
 * @author SuhoSon (ngeol564@gmail.com)
 * @version 0.1
 * @date 2020-08-19
 */

#include <stdlib.h>
#include <assert.h>

#include <json.h>
#include <jemalloc/jemalloc.h>

#include <driver/docker-driver.h>
#include <runner.h>

/**
 * @brief To make a `docker_info` structure to `json_object`.
 *
 * @param[in] info `docker_info` data structure which wants to convert `json_object`.
 *
 * @return Structure's `json_object` 
 *
 * @note You must deallocate this returned `json_object` or attach this to the other `json_object`.
 */
static struct json_object *
docker_info_serializer(const struct docker_info *info)
{
        struct json_object *meta;

        assert(NULL != info);
        meta = json_object_new_object();
        json_object_object_add(meta, "pid", json_object_new_int(info->pid));
        json_object_object_add(meta, "time", json_object_new_int(info->time));
        json_object_object_add(meta, "q_depth",
                               json_object_new_int(info->q_depth));
        json_object_object_add(meta, "nr_thread",
                               json_object_new_int(info->nr_thread));
        json_object_object_add(meta, "weight",
                               json_object_new_int(info->weight));
        json_object_object_add(meta, "mqid", json_object_new_int(info->mqid));
        json_object_object_add(meta, "shmid", json_object_new_int(info->shmid));
        json_object_object_add(meta, "semid", json_object_new_int(info->semid));
        json_object_object_add(meta, "trace_repeat",
                               json_object_new_int(info->trace_repeat));
        json_object_object_add(meta, "wss", json_object_new_int(info->wss));
        json_object_object_add(meta, "utilization",
                               json_object_new_int(info->utilization));
        json_object_object_add(meta, "iosize",
                               json_object_new_int(info->iosize));
        json_object_object_add(
                meta, "prefix_cgroup_name",
                json_object_new_string(info->prefix_cgroup_name));
        json_object_object_add(meta, "scheduler",
                               json_object_new_string(info->scheduler));
        json_object_object_add(meta, "cgroup_id",
                               json_object_new_string(info->cgroup_id));
        json_object_object_add(meta, "trace_data_path",
                               json_object_new_string(info->trace_data_path));
        json_object_object_add(meta, "device", json_object_new_int(info->pid));

        return meta;
}

/**
 * @brief To make a `realtime_log` structure to `json_object`.
 *
 * @param[in] log `realtime_log` data structure which wants to convert `json_object`.
 *
 * @return Structure's `json_object`
 *
 * @note You must deallocate this returned `json_object` or attach this to the other `json_object`.
 */
static struct json_object *
docker_realtime_log_serializer(const struct realtime_log *log)
{
        struct json_object *data;

        assert(NULL != log);
        data = json_object_new_object();

        json_object_object_add(data, "type", json_object_new_int(log->type));
        json_object_object_add(data, "time", json_object_new_double(log->time));
        json_object_object_add(data, "remaining",
                               json_object_new_double(log->remaining));
        json_object_object_add(
                data, "remaining_percentage",
                json_object_new_double(log->remaining_percentage));
        json_object_object_add(data, "avg_bw",
                               json_object_new_double(log->avg_bw));
        json_object_object_add(data, "cur_bw",
                               json_object_new_double(log->cur_bw));
        json_object_object_add(data, "lat", json_object_new_double(log->lat));
        json_object_object_add(data, "time_diff",
                               json_object_new_double(log->time_diff));
        return data;
}

/**
 * @brief To make a `trace` structure to `json_object`.
 *
 * @param[in] traces `trace` data structure which wants to convert `json_object`.
 *
 * @return Structure's `json_object`
 *
 * @note You must deallocate this returned `json_object` or attach this to the other `json_object`.
 */
static struct json_object *
docker_total_trace_serializer(const struct trace *traces)
{
        struct json_object *_trace;
        _trace = json_object_new_object();
        json_object_object_add(_trace, "start_partition",
                               json_object_new_double(traces->start_partition));
        json_object_object_add(_trace, "total_size",
                               json_object_new_double(traces->total_size));
        json_object_object_add(_trace, "start_page",
                               json_object_new_int64(traces->start_page));
        json_object_object_add(_trace, "total_pages",
                               json_object_new_int64(traces->total_pages));
        return _trace;
}

/**
 * @brief To make a `total_results` structure's `config` member to `json_object`.
 *
 * @param[in] total `total_results` data structure which wants to convert `config` member to `json_object`.
 * @param[in] jobject Address of `docker_total_json_object` which already occupying the memory.
 *
 * @return `total_results` structure's `config` member's `json_object`
 *
 * @note You must deallocate this returned `json_object` or attach this to the other `json_object`.
 */
static struct json_object *
docker_total_config_serializer(const struct total_results *total,
                               struct docker_total_json_object *jobject)
{
        struct json_object *config;
        struct json_object *traces;
        struct json_object **trace = jobject->trace;
        int i;

        assert(NULL != total);
        assert(NULL != trace);

        config = json_object_new_object();
        json_object_object_add(config, "qdepth",
                               json_object_new_int(total->config.qdepth));
        json_object_object_add(config, "timeout",
                               json_object_new_double(total->config.timeout));
        json_object_object_add(config, "nr_trace",
                               json_object_new_int(total->config.nr_trace));
        json_object_object_add(config, "nr_thread",
                               json_object_new_int(total->config.nr_thread));
        json_object_object_add(config, "per_thread",
                               json_object_new_int(total->config.per_thread));
        json_object_object_add(
                config, "result_file",
                json_object_new_string(total->config.result_file));

        traces = json_object_new_array();

        for (i = 0; i < total->config.nr_trace; i++) {
                trace[i] =
                        docker_total_trace_serializer(&total->config.traces[i]);
                json_object_array_add(traces, trace[i]);
        }
        json_object_object_add(config, "traces", traces);
        return config;
}

/**
 * @brief To make a `synthetic` structure to `json_object`.
 *
 * @param[in] _synthetic `synthetic` data structure which wants to convert `json_object`.
 *
 * @return Structure's `json_object`
 *
 * @note You must deallocate this returned `json_object` or attach this to the other `json_object`.
 */
static struct json_object *
docker_synthetic_serializer(const struct synthetic *_synthetic)
{
        struct json_object *synthetic;
        synthetic = json_object_new_object();
        json_object_object_add(
                synthetic, "working_set_size",
                json_object_new_int(_synthetic->working_set_size));
        json_object_object_add(synthetic, "utilization",
                               json_object_new_int(_synthetic->utilization));
        json_object_object_add(
                synthetic, "touched_working_set_size",
                json_object_new_int(_synthetic->touched_working_set_size));
        json_object_object_add(synthetic, "io_size",
                               json_object_new_int(_synthetic->io_size));
        return synthetic;
}

/**
 * @brief To make a `trace_stat` structure to `json_object`.
 *
 * @param[in] _stats `trace_stat` data structure which wants to convert `json_object`.
 *
 * @return Structure's `json_object`
 *
 * @note You must deallocate this returned `json_object` or attach this to the other `json_object`.
 */
static struct json_object *
docker_stats_serializer(const struct trace_stat *_stats)
{
        struct json_object *stats;
        struct docker_json_field *field = NULL;
        struct docker_json_field *begin, *end;
        struct docker_json_field fields[] = {
                { "exec_time", &_stats->exec_time },
                { "avg_lat", &_stats->avg_lat },
                { "avg_lat_var", &_stats->avg_lat_var },
                { "lat_min", &_stats->lat_min },
                { "lat_max", &_stats->lat_max },
                { "iops", &_stats->iops },
                { "total_bw", &_stats->total_bw },
                { "read_bw", &_stats->read_bw },
                { "write_bw", &_stats->write_bw },
                { "total_traffic", &_stats->total_traffic },
                { "read_traffic", &_stats->read_traffic },
                { "write_traffic", &_stats->write_traffic },
                { "read_ratio", &_stats->read_ratio },
                { "total_avg_req_size", &_stats->total_avg_req_size },
                { "read_avg_req_size", &_stats->read_avg_req_size },
                { "write_avg_req_size", &_stats->write_avg_req_size },
        };

        stats = json_object_new_object();
        begin = &fields[0];
        end = &fields[sizeof(fields) / sizeof(struct docker_json_field)];
        docker_json_field_traverse(field, begin, end)
        {
                struct json_object *current;
                double value;

                assert(NULL != field);
                value = *(double *)field->member;

                current = json_object_new_double(value);
                assert(NULL != current);

                json_object_object_add(stats, field->name, current);
        }
        return stats;
}

/**
 * @brief To make a `total_results` structure's `per_trace` member to `json_object`.
 *
 * @param[in] total `total_results` data structure which wants to convert `per_trace` member to `json_object`.
 * @param[in] jobject Address of `docker_total_json_object` which already occupying the memory.
 *
 * @return `total_results` structure's `per_trace` member's `json_object`
 *
 * @note You must deallocate this returned `json_object` or attach this to the other `json_object`.
 */
static struct json_object *
docker_total_per_trace_serializer(const struct total_results *total,
                                  struct docker_total_json_object *jobject)
{
        struct json_object *per_traces;
        struct json_object **synthetic = jobject->synthetic;
        struct json_object **stats = jobject->stats;
        struct json_object **per_trace = jobject->per_trace;
        int i;

        assert(NULL != total);
        assert(NULL != synthetic);
        assert(NULL != stats);
        assert(NULL != per_trace);

        per_traces = json_object_new_array();
        for (i = 0; i < total->config.nr_trace; i++) {
                struct json_object *_per_trace;
                _per_trace = json_object_new_object();
                json_object_object_add(
                        _per_trace, "name",
                        json_object_new_string(
                                total->results.per_trace[i].name));
                json_object_object_add(
                        _per_trace, "issynthetic",
                        json_object_new_int(
                                total->results.per_trace[i].issynthetic));

                /* This only valuable when issynthetic value is 1 */
                if (1 == total->results.per_trace[i].issynthetic) {
                        synthetic[i] = docker_synthetic_serializer(
                                &total->results.per_trace[i].synthetic);
                        json_object_object_add(_per_trace, "synthetic",
                                               synthetic[i]);
                }

                stats[i] = docker_stats_serializer(
                        &total->results.per_trace[i].stats);
                json_object_object_add(_per_trace, "stats", stats[i]);

                json_object_object_add(
                        _per_trace, "trace_reset_count",
                        json_object_new_int(
                                total->results.per_trace[i].trace_reset_count));

                per_trace[i] = _per_trace;
                json_object_array_add(per_traces, per_trace[i]);
        }
        return per_traces;
}

/**
 * @brief To make a `aggr_result` structure's `per_trace` member to `json_object`.
 *
 * @param[in] total `total_results` data structure which wants to convert `aggr_result` member to `json_object`.
 *
 * @return `total_results` structure's `aggr_result` member's `json_object`. This is same form of `stats`.
 *
 * @note You must deallocate this returned `json_object` or attach this to the other `json_object`.
 */
static struct json_object *
docker_total_aggr_serializer(const struct total_results *total)
{
        return docker_stats_serializer(&total->results.aggr_result.stats);
}

/**
 * @brief to make a `total_results` structure's `per_trace` and `aggr_result` member to `json_object`.
 *
 * @param[in] total `total_results` data structure which wants to convert `per_trace` and `aggr_result`
 * member to `json_object`.
 * @param[in] jobject address of `docker_total_json_object` which already occupying the memory.
 *
 * @return `total_results` structure's `per_trace` and `aggr_result` member's `json_object`
 *
 * @note you must deallocate this returned `json_object` or attach this to the other `json_object`.
 */
static struct json_object *
docker_total_result_serializer(const struct total_results *total,
                               struct docker_total_json_object *jobject)
{
        struct json_object *results;
        results = json_object_new_object();
        json_object_object_add(results, "per_trace",
                               docker_total_per_trace_serializer(total,
                                                                 jobject));

        json_object_object_add(results, "aggr_result",
                               docker_total_aggr_serializer(total));
        return results;
}

/**
 * @brief to make a `total_results` structure's all member to `json_object`.
 *
 * @param[in] total `total_results` data structure which wants to convert all member to `json_object`.
 * @param[in] jobject address of `docker_total_json_object` which already occupying the memory.
 *
 * @return `total_results` structure's all member's `json_object`
 *
 * @note you must deallocate this returned `json_object` or attach this to the other `json_object`.
 */
static struct json_object *
docker_total_results_serializer(const struct total_results *total,
                                struct docker_total_json_object *jobject)
{
        struct json_object *total_results;
        total_results = json_object_new_object();
        json_object_object_add(total_results, "config",
                               docker_total_config_serializer(total, jobject));
        json_object_object_add(total_results, "results",
                               docker_total_result_serializer(total, jobject));
        return total_results;
}

/**
 * @brief Converts `realtime_log` to JSON string.
 *
 * @param[in] info `docker_info` structure's pointer.
 * @param[in] log `realtime_log` structure's pointer.
 * @param[out] buffer Data contains the execution-time results which are converted to JSON.
 *
 * @warning `buffer` must be allocated memory over and equal `INTERVAL_RESULT_STRING_SIZE`
 */
void docker_realtime_serializer(const struct docker_info *info,
                                const struct realtime_log *log, char *buffer)
{
        struct json_object *object;

        assert(NULL != info);
        assert(NULL != log);
        assert(NULL != buffer);

        object = json_object_new_object();
        json_object_object_add(object, "meta", docker_info_serializer(info));
        json_object_object_add(object, "data",
                               docker_realtime_log_serializer(log));

        snprintf(buffer, INTERVAL_RESULT_STRING_SIZE, "%s",
                 json_object_to_json_string(object));

        json_object_put(object);
}

/**
 * @brief Converts `total_results` to JSON string.
 *
 * @param[in] info `docker_info` structure's pointer.
 * @param[in] total `total_results` structure's pointer.
 * @param[out] buffer Data contains the end-time results which are converted to JSON.
 *
 * @warning `buffer` must be allocated memory over and equal `TOTAL_RESULT_STRING_SIZE`
 */
void docker_total_serializer(const struct docker_info *info,
                             const struct total_results *total, char *buffer)
{
        struct json_object *object;
        struct json_object *info_object, *total_object;
        struct docker_total_json_object *jobject;

        assert(NULL != info);
        assert(NULL != total);

        jobject = (struct docker_total_json_object *)malloc(
                sizeof(struct docker_total_json_object));
        assert(NULL != jobject);

        object = json_object_new_object();

        info_object = docker_info_serializer(info);
        total_object = docker_total_results_serializer(total, jobject);

        json_object_object_add(object, "meta", info_object);
        pr_info(INFO, "Adds meta success %p\n", (void *)info_object);

        json_object_object_add(object, "data", total_object);
        pr_info(INFO, "Adds data success %p\n", (void *)total_object);

        snprintf(buffer, TOTAL_RESULT_STRING_SIZE, "%s",
                 json_object_to_json_string(object));
        json_object_put(object);
        free(jobject);
}
