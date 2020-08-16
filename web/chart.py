import json
import copy


class chart:
    """ Preprocessing trace replay interval result into front-end chart.
        Show average bandwidth,current bandwidth, latency, cgroup_id.
    """

    def __init__(self, _json):
        """ Set attribute for front-end chart with json.
            Args:
                _json: json string from trace replay interval result.
        """
        FIN = 3
        interval_result = json.loads(_json)

        if interval_result["data"]["type"] == FIN:
            self.chart_result = {}
        else:
            self.chart_result = {"data": {}}
            self.set_avg_bw(interval_result["data"]["avg_bw"])
            self.set_cur_bw(interval_result["data"]["cur_bw"])
            self.set_latency(interval_result["data"]["lat"])
            self.set_cgroup_id(interval_result["meta"]["cgroup_id"])

    def set_avg_bw(self, avg_bw):
        """ Setter with avg_bw.
        """
        self.chart_result["data"]["avg_bw"] = avg_bw

    def set_cur_bw(self, cur_bw):
        """ Setter with cur_bw.
        """
        self.chart_result["data"]["cur_bw"] = cur_bw

    def set_latency(self, lat):
        """ Setter with lat.
        """
        self.chart_result["data"]["lat"] = lat

    def set_cgroup_id(self, cgroup_id):
        """ Setter with cgroup_id
        """
        self.chart_result["cgroup_id"] = cgroup_id

    def get_chart_result(self):
        """ Getter with chart data in dictionary.
        """
        return self.chart_result


def get_chart_result(raw_json_data):
    """ Preprocessing trace replay interval result into front-end chart data.
        Args:
            raw_json_data: json string with trace replay interval result.
    """
    buf = chart(raw_json_data)
    chart_result = copy.copy(buf.get_chart_result())
    del buf
    return chart_result
