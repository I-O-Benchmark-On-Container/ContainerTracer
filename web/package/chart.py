import json
import copy


##
# @brief Class for preprocessing Trace-Replay interval results into front-end chart data form.
# Show average bandwidth, current bandwidth, latency, cgroup_id.
class chart:
    ##
    # @brief Set attribute for front-end chart with json..
    #
    # @param _json json string from trace replay interval result.
    def __init__(self):
        FIN = 3

    def set_config(_json):
        interval_result = json.loads(_json)

        if interval_result["data"]["type"] == FIN:
            self.chart_result = {}
        else:
            self.chart_result = {"data": {}}
            self.set_avg_bw(interval_result["data"]["avg_bw"])
            self.set_cur_bw(interval_result["data"]["cur_bw"])
            self.set_latency(interval_result["data"]["lat"])
            self.set_cgroup_id(interval_result["meta"]["cgroup_id"])

    ##
    # @brief Setter with avg_bw
    #
    # @param avg_bw Average bandwidth
    def set_avg_bw(self, avg_bw):
        self.chart_result["data"]["avg_bw"] = avg_bw

    ##
    # @brief Setter with cur_bw
    #
    # @param cur_bw Current bandwidth
    def set_cur_bw(self, cur_bw):
        self.chart_result["data"]["cur_bw"] = cur_bw

    ##
    # @brief Setter with lat
    #
    # @param lat Latency
    def set_latency(self, lat):
        self.chart_result["data"]["lat"] = lat

    ##
    # @brief Setter with cgroup_id
    #
    # @param cgroup_id Cgroup_id
    def set_cgroup_id(self, cgroup_id):
        self.chart_result["cgroup_id"] = cgroup_id

    ##
    # @brief Get datas in front-end chart data form.
    #
    # @return datas in front-end chart data form with dictionary.
    def get_chart_result(self):
        return self.chart_result


##
# @brief User function preprocessing trace replay interval result into front-end chart data.
#
# @param raw_json_data Json string with trace replay interval result.
#
# @return Trace replay interval result in front-end chart data from.
def get_chart_result(raw_json_data):
    buf = chart()
    buf.set_config(raw_json_data)
    chart_result = copy.copy(buf.get_chart_result())
    del buf
    return chart_result
