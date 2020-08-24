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

function addOptions(nrCgroup) {
    for (let idx = 1; idx < Number(nrCgroup) + 1; idx++) {
        let ulList = $('#options');
        ulList.append("<li>" +
                            "<div class=form-group>" +
                            "<label>Cgroup " + idx + "</label>" +
                                ": " +
                                '<div class="row">' +
                                "<label>Weight</label>" +
                                '<input type="text" class="form-control form_theme search_theme" name="cgroup-' + idx + '" id="cgroup' + idx + '" placeholder="input weight" autocomplete="off" value="1000">' +
                                "</div>" +
                                '<div class="row">' +
                                "<label>trace_data_path</label>" +
                                '<input type="text" class="form-control form_theme search_theme" name="cgroup-' + idx + '-path" id="cgroup' + idx + '-path" placeholder="ex: " autocomplete="off" value="./sample/sample1.dat">' +
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
