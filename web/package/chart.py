import json
import copy


##
# @brief trace-replay interval result를 Frontend의 차트 형식에 맞게 전처리하기 위한 클래스.
# average bandwidth, current bandwidth, latency, cgroup_id를 보여줌.
class chart:
    FIN = 3

    ##
    # @brief 전달받은 json 형식의 interval result를 Frontend 차트 형식에 맞게 지정.
    #
    # @param _json trace-replay interval result로 json string 형식.
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
    # @brief set_config()에서 formatting한 차트 형식의 데이터를 리턴.
    #
    # @return Frontend 차트 형식에 맞는 데이터. 딕셔너리 형태.
    def get_chart_result(self):
        return self.chart_result


##
# @brief trace-replay interval result를 Frontend 차트 형식으로 reformatting하는 유저 함수.
#
# @param raw_json_data string json 형식의 trace-replay interval result.
#
# @return Frontend 차트에 맞게 reformatting된 trace-replay interval result.
def get_chart_result(raw_json_data):
    buf = chart()
    buf.set_config(raw_json_data)
    chart_result = copy.copy(buf.get_chart_result())
    del buf
    return chart_result
