var labels = new Array(60).fill('');
var $ctxArray = document.getElementsByClassName("chart");
const CHART = 0, CONFIG = 1;
let dataList = new Array(new Array(), new Array());
let charts = new Array(new Array(), new Array());

function showChart(nr_cgroup) {
    // 만약 차트를 추가한다면 ctxIdx를 추가하면 됨
    for(let ctxIdx = 0; ctxIdx < $ctxArray.length; ctxIdx++){
        for(let cgroupIdx = 0; cgroupIdx < nr_cgroup; cgroupIdx++){
            let randomRgba = `${Math.floor(Math.random() * 255)}, ${Math.floor(Math.random() * 255)}, ${Math.floor(Math.random() * 255)}`
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

        }
        charts[ctxIdx][CHART] = new Chart($targetCtx, charts[ctxIdx][CONFIG]);
    }
}

const LATENCY = 0, THROUGHPUT = 1;
function addDataInChart(response_data){
    for (let chartIdx = 0; chartIdx < 2; chartIdx++) {
        var datasets = charts[chartIdx][CONFIG].data.datasets;
        for (let cgroupIdx = 0; cgroupIdx < datasets.length; cgroupIdx++) {
            let eachData = datasets[cgroupIdx].data;
            eachData.shift();
            eachData.push(response_data[cgroupIdx][chartIdx])
        }
    }

    charts[LATENCY][CHART].update();
    charts[THROUGHPUT][CHART].update();
}