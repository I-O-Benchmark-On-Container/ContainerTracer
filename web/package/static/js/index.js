let socket = io.connect("http://" + document.domain + ":" + location.port);

$(document).ready(function(){
    const $optionDisplay = $('#optionDisplay');
    const $chartDisplay = $('#chartDisplay');
    const $btnSelectWork = $('#btnSelectWork');
    const $btnStartWork = $('#btnStartWork');
    const $options = $('#options');

    /* advanced opttions button event listener */
    $btnSelectWork.on('click', function(event){
        $chartDisplay.addClass('hide');
        $options.children().remove();
        let nrCgroup = $('#cgroup').val();
        let driver = $('#driver').val();

        let data = {
            driver: driver,
        };

        addOptions(nrCgroup);
        socket.emit("set_driver", data);
    });

    /* start button event listener */
    $btnStartWork.on('click', function(event){
        event.preventDefault();
        let setEach = $("form[name=setEach").serializeObject();
        let setAll = $("form[name=setAll]").serializeObject();
        socket.emit("set_options", setEach, setAll);
        $btnSelectWork.prop("disabled", true);
        $btnStartWork.prop("disabled", true);
    });

    socket.on('set_driver_ret', function(){
        $optionDisplay.removeClass('hide');
    });


    socket.on('chart_start', function(nrCgroup){
        $chartDisplay.removeClass('hide');
        $("#chartState").val("1");
        toastr.info("Do not refresh or move pages while the work is being completed.", "Performance evaluation is currently in progress.",
                    {timeOut: 0, extendedTimeOut: 0, tapToDismiss: false});
        showChart(nrCgroup);
        $optionDisplay.addClass('hide');
    });

    socket.on('chart_data_result', function(result){
        addDataInChart(result);
    });

    socket.on('chart_end', function(){
        toastr.clear();
        toastr.success("The results of the performance evaluation have been saved.", "Complete!");
        $("#chartState").val("0");
        $btnSelectWork.prop("disabled", false);
        $btnStartWork.prop("disabled", false);
        $options.children().remove();
    });
});

