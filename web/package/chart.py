import json


##
# @brief Want to convert trace-replay interval result into Frontend chart form.
# Show average bandwidth, current bandwidth, latency, cgroup_id.
class Chart:
    FIN = 3

    ##
    # @brief Set interval result in json form into frontend chart form.
    #
    # @param _json trace-replay interval result in json string form.
    def set_config(self, _json):
        interval_result = json.loads(_json)

        if interval_result["data"]["type"] == self.FIN:
            self.chart_result = {}
        else:
            self.chart_result = {"data": {}}
            self.set_avg_bw(interval_result["data"]["avg_bw"])
            self.set_cur_bw(interval_result["data"]["cur_bw"])
            self.set_latency(interval_result["data"]["lat"])
            self.set_cgroup_id(interval_result["meta"]["cgroup_id"])

    ##
    # @brief average bandwidth setter.
    #
    # @param avg_bw average bandwidth.
    def set_avg_bw(self, avg_bw):
        self.chart_result["data"]["avg_bw"] = avg_bw

    ##
    # @brief current bandwidth setter.
    #
    # @param cur_bw current bandwidth
    def set_cur_bw(self, cur_bw):
        self.chart_result["data"]["cur_bw"] = cur_bw

    ##
    # @brief latency setter.
    #
    # @param lat latency.
    def set_latency(self, lat):
        self.chart_result["data"]["lat"] = lat

    ##
    # @brief cgroup id setter.
    #
    # @param cgroup_id cgroup id.
    def set_cgroup_id(self, cgroup_id):
        self.chart_result["cgroup_id"] = cgroup_id

    ##
    # @brief return formatted data at set_config()
    #
    # @return Trace-replay interval resut in frontend chart form. dictinory.
    def get_chart_result(self):
        return self.chart_result
