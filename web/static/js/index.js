let socket = io.connect("http://" + document.domain + ":" + location.port + "/config");

$(document).ready(function(){
    const $optionDisplay = $('#optionDisplay');
    const $chartDisplay = $('#chartDisplay')
    const $btnSelectWork = $('#btnSelectWork');
    const $btnStartWork = $('#btnStartWork');

    /* driver, croup 선택 */
    $btnSelectWork.on('click', function(event){
        let nr_cgroup = $('#cgroup').val();
        let driver = $('#driver').val();

        let data = {
            driver: driver,
            nr_cgroup: nr_cgroup
        };

        socket.emit("set_driver", data);
        cgroups = nr_cgroup;
        $btnSelectWork.prop("disabled", true);
    });

    /* 세부 옵션 선택 */
    $btnStartWork.on('click', function(event){
        event.preventDefault();
        let data = $("form[name=searchForm]").serializeObject();
        socket.emit("set_options", data);
        $btnStartWork.prop("disabled", true);
    });

    /* left side button response */
    socket.on('set_driver_ret', function(msg){
        $optionDisplay.removeClass('hide');
    });


    socket.on('chart_start', function(nr_cgroup){
        $chartDisplay.removeClass('hide');
        showChart(nr_cgroup);
    });

    socket.on('chart_data_result', function(result){
        addDataInChart(result);
    });

    socket.on('chart_end', function(msg){
        $btnSelectWork.prop("disabled", false);
        $btnStartWork.prop("disabled", false);
    });
});
