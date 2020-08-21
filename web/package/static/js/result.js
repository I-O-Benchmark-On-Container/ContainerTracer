var labels = new Array(60).fill('');
var $ctxArray = document.getElementsByClassName("chart");
const CHART = 0, CONFIG = 1;
let dataList = [[], [], []];
let charts = [[], [], []];

function showChart(nrCgroup) {
    // 만약 차트를 추가한다면 ctxIdx를 추가하면 됨
    let rgb = new Array(nrCgroup);
    for(let ctxIdx = 0; ctxIdx < $ctxArray.length; ctxIdx++){
        for(let cgroupIdx = 0; cgroupIdx < nrCgroup; cgroupIdx++){
            let randomRgba = `${Math.floor(Math.random() * 255)}, ${Math.floor(Math.random() * 255)}, ${Math.floor(Math.random() * 255)}`;

            if (!ctxIdx) {
                rgb[cgroupIdx] = randomRgba;
            } else {
                randomRgba = rgb[cgroupIdx];
            }

            dataList[ctxIdx][cgroupIdx] = {
                label: `Cgroup-${cgroupIdx + 1}`,
                data: Array(60).fill(0),
                    backgroundColor: [
                        `rgba(${randomRgba}, 0.2`,
                    ],
                    borderColor: [
                        `rgba(${randomRgba}, 1`,
                    ],
                borderWidth: 1
            };
        }

        let $targetCtx = $ctxArray[ctxIdx];

        charts[ctxIdx][CONFIG] = {
            type: 'line',
            data: {
                labels: labels,
                datasets: dataList[ctxIdx]
            },
            options: {
                scales: {
                    yAxes: [{
                        ticks: {
                            beginAtZero:true
                        }
                    }]
                }
            }

        };
        charts[ctxIdx][CHART] = new Chart($targetCtx, charts[ctxIdx][CONFIG]);
    }
}

const LATENCY = 0, CUR_BW = 1, AVG_BW = 2;
function addDataInChart(responseData){
    for (let chartIdx = 0; chartIdx < 3; chartIdx++) {
        var datasets = charts[chartIdx][CONFIG].data.datasets;
        for (let cgroupIdx = 0; cgroupIdx < datasets.length; cgroupIdx++) {
            let id = responseData[cgroupIdx].cgroup_id;
            let data = responseData[cgroupIdx].data.lat;

            if (chartIdx == CUR_BW) {
                data = responseData[cgroupIdx].data.cur_bw;
            } else { //AVG BW
                data = responseData[cgroupIdx].data.avg_bw;
            }

            let eachData = datasets[id - 1].data;
            eachData.shift();
            eachData.push(data);
        }
    }

    charts[LATENCY][CHART].update();
    charts[CUR_BW][CHART].update();
    charts[AVG_BW][CHART].update();
}
