const QUEUE_SIZE = 60;
let appCharts = null;

function appDriverChartDisplay() {
    appCharts = new Map();
    $('.app-driver-chart').each((_, chartCanvas) => {
        let lineColor = `${Math.floor(Math.random() * 255)}, ${Math.floor(Math.random() * 255)}, ${Math.floor(Math.random() * 255)}`;
        let initData = {
            label: chartCanvas.id,
            data: Array(QUEUE_SIZE).fill(0),
            backgroundColor: [`rgba(${lineColor}, 0.2)`],
            borderColor: [`rgba(${lineColor}, 1)`],
            borderWidth: 1,
        };
        let config = {
            type: 'line',
            data: {
                labels: new Array(QUEUE_SIZE).fill(''),
                datasets: [initData]
            }, // end of data
            options: {
                scales: {
                    yAxes: [{
                        ticks: {
                            beginAtZero: true
                        }
                    }]
                },
            }, // end of options
        }; // end of chart
        let object = new Chart(chartCanvas, config);
        appCharts[chartCanvas.id] = { "config": config, "object": object };
    });

    return 0;
}

function addDataToAppDriverChart(id, value) {
    let chart = appCharts[id];
    if (chart === null) {
        console.log("Cannot find the chart: " + id);
        throw new UserException('InvalidChartIndex');
    }
    let config = chart['config'];
    let datasets = config.data.datasets;

    try {
        datasets[0].data.shift();
        datasets[0].data.push(value);
    } catch (e) {
        assert(e);
    }
    let object = chart['object'];
    object.update();
}