let socket = io.connect("http://" + document.domain + ":" + location.port);

$(document).ready(function(){
    const $optionDisplay = $('#optionDisplay');
    const $chartDisplay = $('#chartDisplay');
    const $btnSelectWork = $('#btnSelectWork');
    const $btnStartWork = $('#btnStartWork');
    const $options = $('#options');

    /* driver, croup 선택 */
    $btnSelectWork.on('click', function(event){
        /* Cgroup의 수를 바로 변경할 경우에...*/
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

    /* 세부 옵션 선택 */
    $btnStartWork.on('click', function(event){
        event.preventDefault();
        let setEach = $("form[name=setEach").serializeObject();
        let setAll = $("form[name=setAll]").serializeObject();
        socket.emit("set_options", setEach, setAll);
        $btnSelectWork.prop("disabled", true);
        $btnStartWork.prop("disabled", true);
    });

    /* left side button response */
    socket.on('set_driver_ret', function(){
        $optionDisplay.removeClass('hide');
    });

    socket.on('chart_start', function(nrCgroup){
        $chartDisplay.removeClass('hide');
        $("#chartState").val("1");
        toastr.info("작업이 완료되는 동안 새로고침, 페이지 이동을 하지마세요.", "성능 평가 진행 중 입니다.",
                    {timeOut: 0, extendedTimeOut: 0, tapToDismiss: false});
        showChart(nrCgroup);
        $optionDisplay.addClass('hide');
    });

    socket.on('chart_data_result', function(result){
        addDataInChart(result);
    });

    socket.on('chart_end', function(){
        toastr.clear();
        socket.emit("reset");
        toastr.success("결과가 저장 되었습니다.", "작업이 완료되었습니다.");
        $("#chartState").val("0");
        $btnSelectWork.prop("disabled", false);
        $btnStartWork.prop("disabled", false);
        $options.children().remove();
    });
});

