let socket = io.connect("http://" + document.domain + ":" + location.port);
let prevDataList = null;
let nrCgroup = 0;

function setCookie(cookie_name, value, days) {
    var exdate = new Date();
    exdate.setDate(exdate.getDate() + days);

    var cookie_value = escape(value) + ((days == null) ? '' : ';    expires=' + exdate.toUTCString());
    document.cookie = cookie_name + '=' + cookie_value;
}

function getCookie(cookie_name) {
    var x, y;
    var val = document.cookie.split(';');

    for (var i = 0; i < val.length; i++) {
        x = val[i].substr(0, val[i].indexOf('='));
        y = val[i].substr(val[i].indexOf('=') + 1);
        x = x.replace(/^\s+|\s+$/g, '');
        if (x == cookie_name) {
              return unescape(y);
        }
    }
}

$(document).ready(function(){
    const $optionDisplay = $('#optionDisplay');
    const $chartDisplay = $('#chartDisplay');
    const $btnSelectWork = $('#btnSelectWork');
    const $btnStartWork = $('#btnStartWork');
    const $options = $('#options');

    if (prevDataList === null) {
	    prevDataList = new Array();
	    idx = 1;
	    while (getCookie("cgroup-"+idx) !== undefined) {
		    let weight = getCookie("cgroup-"+idx);
		    let path = getCookie("cgroup-"+idx+"path");
		    prevDataList.push({"weight":weight, "path":path});
		    idx++;
	    }
    }

    /* advanced opttions button event listener */
    $btnSelectWork.on('click', function(event){
        $chartDisplay.addClass('hide');
        $options.children().remove();
        let driver = $('#driver').val();
        let data = {
            driver: driver,
        };

        nrCgroup = $('#cgroup').val();
        
        $('#nr_tasks').val(nrCgroup);
	for (let idx = prevDataList.length; idx < Number(nrCgroup); idx++) {
		prevDataList.push({"weight":1000, "path":"./sample/sample1.dat"});
	}
        addOptions(nrCgroup, prevDataList);
        socket.emit("set_driver", data);
    });

    /* start button event listener */
    $btnStartWork.on('click', function(event){
        event.preventDefault();
        let setEach = $("form[name=setEach").serializeObject();
        let setAll = $("form[name=setAll]").serializeObject();

	for (let idx = 1; idx <= Number(nrCgroup); idx++) {
		prevDataList[idx-1]["weight"] = setEach["cgroup-"+idx]
		prevDataList[idx-1]["path"] = setEach["cgroup-"+idx+"-path"]
		setCookie("cgroup-"+idx, setEach["cgroup-"+idx])
		setCookie("cgroup-"+idx+"-path", setEach["cgroup-"+idx+"-path"])
	}
        socket.emit("set_options", setEach, setAll);
        $btnSelectWork.prop("disabled", true);
        $btnStartWork.prop("disabled", true);
    });

    socket.on('set_driver_ret', function(){
        $optionDisplay.removeClass('hide');
    });

    socket.on('Invalid', function(message){
        toastr.error("Invalid " + message , "error!!");
        $btnStartWork.prop("disabled", false);
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
        socket.emit("reset");
        toastr.success("The results of the performance evaluation have been saved.", "Complete!");
        $("#chartState").val("0");
        $btnSelectWork.prop("disabled", false);
        $btnStartWork.prop("disabled", false);
        $options.children().remove();
    });
});

