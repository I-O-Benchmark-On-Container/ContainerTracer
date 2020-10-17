function doRun() {
    let app_path = $('#app-path').val()
    let docker_name = $('#docker-name').val()
    if (app_path == '' || docker_name == '') {
        assert(`Invalid value detected.... ${app_path} ${docker_name}`)
    }
    socket.emit('app_driver_run', { 'app-path': app_path, 'docker-name': docker_name });
}

function doRecord() {
    socket.emit('app_driver_record');
}

function doReplay() {
    socket.emit('app_driver_replay');
}

function activateBtnListener(socket) {
    const btnFunc = { '#app-run': doRun, '#app-record': doRecord, '#app-replay': doReplay };

    for (const key in btnFunc) {
        const btn = $(key);
        btn.on('click', () => {
            btnFunc[key]();
        });
    }
}

function activateRecordListener(socket) {
    socket.on('app_driver_record_start', () => {
        toastr.clear();
        toastr.info('Record sequence is started...');
        $('#app-replay').prop('disabled', true);
    });

    socket.on('app_driver_record_stop', () => {
        toastr.clear();
        toastr.success('Record sequence is stopped...');
        $('#app-replay').prop('disabled', false);
    });
}

function activateProgramRunListener(socket) {
    socket.on('app_driver_program_run', () => {
        toastr.clear();
        toastr.success('Program running started...');
    });

}

function activateReplayListener(socket) {
    socket.on('app_driver_replay_start', () => {
        toastr.clear();
        toastr.info('Replay sequence is started...');
        $('#app-record').prop('disabled', true);
    });

    socket.on('app_driver_replay_stop', () => {
        toastr.clear();
        toastr.success('Replay sequence is stopped...');
        $('#app-record').prop('disabled', false);
    });
}

function activatePerformanceListener(socket) {
    socket.on('app_driver_perf_put', (message) => {
        for (let key in message) {
            addDataToAppDriverChart(key, message[key]);
        }
    });
}

function activateAppDriverListener(socket) {
    activateBtnListener(socket);
    activateRecordListener(socket);
    activateReplayListener(socket);
    activatePerformanceListener(socket);
}