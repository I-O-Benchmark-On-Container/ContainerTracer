$.fn.serializeObject = function() {
    var obj = null;
    try {
        if (this[0].tagName && this[0].tagName.toUpperCase() == "FORM") {
            var arr = this.serializeArray();
            if (arr) {
                obj = {};
                $.each(arr, function() {
                    obj[this.name] = this.value;
                });
            }
        }
    } catch (e) {
        alert(e.message);
    } finally {
    }
    return obj;
};

function addOptions(nrCgroup, prevDataList) {
    let ulList = $('#options');
    ulList.append("<div class=row>" + "<label>All weights are in the range <B>(10, 1000)</B></label>" + "</div>");
    for (let idx = 1; idx < Number(nrCgroup) + 1; idx++) {
        ulList.append("<li>" +
                            "<div class=form-group>" +
                            "<label>Cgroup " + idx + "</label>" +
                                ": " +
                                '<div class="row">' +
                                "<label>Weight</label>" +
                                '<input type="text" class="form-control form_theme search_theme" name="cgroup-' + idx + '" id="cgroup' + idx + '" placeholder="input weight" autocomplete="off" value="'+prevDataList[idx-1]["weight"]+'">' +
                                "</div>" +
                                '<div class="row">' +
                                "<label>trace_data_path</label>" +
                                '<input type="text" class="form-control form_theme search_theme" name="cgroup-' + idx + '-path" id="cgroup' + idx + '-path" placeholder="ex: " autocomplete="off" value="'+prevDataList[idx-1]["path"]+'">' +
                                "</div>" +
                            "</div>" +
                        "</li>");
    }
}

function noRefresh()
{
    if ($("#chartState").val() === "1") {
        if ((event.keyCode === 78 || event.keyCode === 82) && (event.ctrlKey === true))
        {
            event.keyCode = 0;
            return false;
        }

        if(event.keyCode === 116)
        {
            event.keyCode = 0;
            return false;
        }
    }
}

document.onkeydown = noRefresh ;


/* toastr */
toastr.options = {
    "closeButton": false,
    "debug": false,
    "newestOnTop": false,
    "progressBar": false,
    "positionClass": "toast-bottom-right",
    "preventDuplicates": false,
    "onclick": null,
    "showDuration": "300",
    "hideDuration": "1000",
    "timeOut": "5000",
    "extendedTimeOut": "1000",
    "showEasing": "swing",
    "hideEasing": "linear",
    "showMethod": "fadeIn",
    "hideMethod": "fadeOut"
  };
