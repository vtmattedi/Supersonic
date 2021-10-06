var count = 0;
var CircCount = 0;


var failedRequests = 0;
var ESP_Connected = true;

var getDataTimer = setInterval(RequestData, 5000);
var getConfig = setTimeout(() => {RequestData("/loadconfig")}, 1000);
var updateTimer = setInterval(setTime, 1000);
var dns_name = window.location.hostname;

google.charts.load('current', { 'packages': ['corechart'] });
google.charts.setOnLoadCallback(drawChart);
var am_i_offline = false;

const HandledCircles = [];


const GraphOptions = new GraphOptionsCreator();
const CurrentRamp = new CurrentRampCreator();
const State = new StateCreator();
const Config = new ConfigCreator();

function CurrentRampCreator() {
    this.temp = [];
    this.times = [];
    this.stepsTimeStamps = [];

    this.totalsteps = 0;
    this.start = 0;
    this.heatingStart = 0;
    this.end = 0;
    this.computedEnd = 0;

    this.configured = false;
    this.finished = false;
    this.running = false;
    this.heating = false;
    this.programmedStart = false;
    this.heatUp = false;

    this.name = "";
    this.reset = function () {
        this.temp = [];
        this.times = [];
        this.stepsTimeStamps = [];

        this.totalsteps = 0;
        this.start = 0;
        this.heatingStart = 0;
        this.end = 0;
        this.computedEnd = 0;

        this.configured = false;
        this.finished = false;
        this.running = false;
        this.heating = false;
        this.heatUp = false;
        this.programmedStart = false;
        this.name = "";
    }

}

function StateCreator() {
    this.Temperature = 0;
    this.CurrentStep = 0;
    this.Progress = 0;
    this.isRunning = false;
    this.Paused = false;
    this.Finished = false;
    this.relay_status = false;
    this.WorkerState = 'Unknown';
    this.update = function () { updateRampState() };

}
function GraphOptionsCreator() {
    this.DrawProgramedRamp = true;
    this.RampOption = 'function';
    this.GraphSize = -1;
    this.DataTable = undefined;
    this.ShowFuture = true;
    this.HoldGraph = false;
    this.GraphName = "";
    this.resetDataTable = function () {
        var data = new google.visualization.DataTable();
        data.addColumn('date', 'Time');
        data.addColumn('number', 'Temperature');
        data.addColumn({ 'type': 'string', 'role': 'style' });
        this.DataTable = data;
    }
}
function CirclePropertyHandler(nameOfCircle) {
    this.id = nameOfCircle;
    this.NeedToMap = false;
    this.PercentMap = {
        'min': 0,
        'max': 100,
        'new_min': 0,
        'new_max': 100
    };
    this.RainbowBar = false;
    this.RainbowNumber = false;
    this.Rainbow = { 'min': '#000000', 'max': '#FFFFFF' };
    this.SetNew = function (value) {
        _val = parseInt(value);
        var _color = undefined;
        var _numberColor = undefined;
        var _valueText = undefined;
        if (_val < 0 || _val > 100) {
            console.log('cant parse new value.', this.id, value);
        }
        if (this.NeedToMap) {
            if (Number.isNaN(this.PercentMap.min) || Number.isNaN(this.PercentMap.max) || Number.isNaN(this.PercentMap.new_min) || Number.isNaN(this.PercentMap.new_max)) {
                this.NeedToMap = false;
                console.log('NaN detected in Mapping ' + this.id + ' disabling map functionality');
                return;
            }
            _val = mapN(_val, this.PercentMap.min, this.PercentMap.max, this.PercentMap.new_min, this.PercentMap.new_max);
            _valueText = value;
        }
        if (this.RainbowBar) {
            //TODO get raibow color
            _color = "#FFFFF0";
        }
        if (this.RainbowNumber) {
            //TODO get raibow color
            _numberColor = "#FFFFF0";
        }

        setnew(this.id, _val, _valueText, _color, _numberColor);
    };

}

function ConfigCreator() {
    this.tolerance;
    //Resolution of DS18b20 Sensor
    this.TEMPERATURE_RESOLUTION;

    //Web Site Credentials
    this.www_username;
    this.www_password;

    //Wifi Credentials
    this.WIFI_SSID;
    this.WIFI_PASSWORD;

    //Sample rate for logging
    this.sample_rate_in_seconds;

    this.load = function (newConfig) {

        var value = "";
        var variable = "";
        var nextChar;
        var isValue = false;
        for (var i = 0; i < newConfig.length; i++) {
            nextChar = newConfig[i];
            if (!isValue) {
                if (nextChar != '=')
                    variable += nextChar;
                else
                    isValue = true;
            }
            else {
                if (nextChar != ',')
                    value += nextChar;
                else {
                    if (variable == "tolerance")
                        this.tolerance = parseFloat(value);
                    else if (variable == "WIFI_SSID")
                        this.WIFI_SSID = value;
                    else if (variable == "WIFI_PASSWORD")
                        this.WIFI_PASSWORD = value;
                    else if (variable == "www_username")
                        this.www_username = value;
                    else if (variable == "www_password")
                        this.www_password = value;
                    else if (variable == "TEMPERATURE_RESOLUTION")
                        this.TEMPERATURE_RESOLUTION = parseInt(value);
                    else if (variable == "sample_rate_in_seconds")
                        this.sample_rate_in_seconds = parseInt(value);

                    value = "";
                    variable = "";
                    isValue = false;
                }
            }
        }
    };
}

function test() {
    // var x = "new_temp" + (Date.now() / 1000) + " 23.81;heat_ramp:-1,10.00 20.00,20.00 20.00,40.00 10.00,20.00 10.00,15.00 15.00,;new_step:1; address:;ramp_start:1622612683;"

    // ParseNewData(x);

    saveData();
}

function setcircle(CircId, value) {

    for (var i = 0; i < HandledCircles.length; i++) {
        if (HandledCircles[i].id == CircId) {
            HandledCircles[i].SetNew(value);
            return;
        }
    }
}

function updateButtonState(buttonState) {
    var name = buttonState.id;
    document.getElementById(name + '_text').innerHTML = buttonState.state;
    document.getElementById(name).className = 'button button' + buttonState.state;
    document.getElementById(name + '_tooltip').hidden = true;
    if (typeof buttonState.enabled !== 'undefined') {

        if (buttonState.enabled) {
            document.getElementById(buttonState.id).removeAttribute("disabled");
        }
        else
            document.getElementById(buttonState.id).setAttribute("disabled", "disabled");
    }
    if (typeof buttonState.tooltip !== 'undefined') {

        if (buttonState.tooltip.enabled) {
            document.getElementById(name + '_tooltip').hidden = false;
        }
        else
            document.getElementById(name + '_tooltip').hidden = true;
        document.getElementById(name + '_tooltip').innerHTML = buttonState.tooltip.text;
    }

}

function newHeatRamp(args) {

    var _newtimes = [];
    var _newtemps = [];
    var _args = args.split(',');
    for (var i = 0; i < _args.length; i++) {
        var variable = _args[i].split('=')[0];
        var value = _args[i].split('=')[1];
        if (variable == "pname")
            CurrentRamp.name = value;
        else if (variable == "steps")
            CurrentRamp.totalsteps = parseInt(value);
        else if (variable == "start")
            CurrentRamp.start = parseInt(value);
        else if (variable == "heatingStart")
            CurrentRamp.heatingStart = parseInt(value);
        else if (variable == "end")
            CurrentRamp.end = parseInt(value) == 1 ? true : false;
        else if (variable == "configured")
            CurrentRamp.configured = parseInt(value) == 1 ? true : false;
        else if (variable == "running")
            CurrentRamp.running = parseInt(value) == 1 ? true : false;
        else if (variable == "heating")
            CurrentRamp.heating = parseInt(value) == 1 ? true : false;
        else if (variable == "finished")
            CurrentRamp.finished = parseInt(value) == 1 ? true : false;
        else if (variable == "heatUp")
            CurrentRamp.heatUp = parseInt(value) == 1 ? true : false;
        else if (variable == "programmedStart")
            CurrentRamp.programmedStart = parseInt(value) == 1 ? true : false;
        else if (variable == "Temp") {
            var newFloat = value.split(' ');
            for (var j = 0; j < newFloat.length; j++) {

                if (!isNaN(parseFloat(newFloat[j]))) {
                    _newtemps[j] = parseFloat(newFloat[j]);
                }
            }
            CurrentRamp.temp = _newtemps;
        }
        else if (variable == "Mins") {
            var newFloat = value.split(' ');
            for (var j = 0; j < newFloat.length; j++) {

                if (!isNaN(parseFloat(newFloat[j]))) {
                    _newtimes[j] = parseFloat(newFloat[j]);
                }
            }
            CurrentRamp.times = _newtimes;
        }
    }

    if (CurrentRamp.finished)
        EndRampRoutine(true);
    else if (CurrentRamp.running || CurrentRamp.heating)
        StartRampRoutine(true);
    else if (CurrentRamp.configured)
        NewRampRoutine(true);


    drawChart();
}

function SetStopButton(state, enabled) {
    var newBtnState =
    {
        id: undefined,
        state: undefined,
        tooltip: undefined,
        enabled: true
    }

    if (typeof enabled !== 'undefined') {
        newBtnState.enabled = enabled;
        if (!enabled)
            newBtnState.tooltip = { enabled: true, text: 'Load a heat ramp to be able to start it' }
    }


    newBtnState.id = 'btnStop'
    newBtnState.state = state;
    updateButtonState(newBtnState);


}

function SetNewRampButton(enabled) {
    var newBtnState =
    {
        id: undefined,
        state: undefined,
        tooltip: undefined,
        enabled: true
    }

    if (typeof enabled !== 'undefined') {
        newBtnState.enabled = enabled;
        if (!enabled)
            newBtnState.tooltip = { enabled: true, text: 'Stop or finish current procedure before loading a new ramp' }
    }


    newBtnState.id = 'btnNewRamp'
    newBtnState.state = 'New Ramp';
    updateButtonState(newBtnState);


}


function updateRampState() {

    document.getElementById("stateLabel").innerHTML = State.WorkerState;
    document.getElementById("stateLabel").className = 'stateLabel ' + State.WorkerState.trim();
    updateRemTime()
    document.getElementById('currentStepsLabel').innerText = State.CurrentStep + '/' + CurrentRamp.totalsteps;
    if (CurrentRamp.name !== "" && typeof (CurrentRamp.name) !== undefined && !CurrentRamp.finished)
        GraphOptions.GraphName = ": " + CurrentRamp.name;
    else
        GraphOptions.GraphName = "";
}

function updateRemTime() {

    if (!CurrentRamp.configured)
        return;

    var _remTime = "";

    var maxSize = 0;
    for (let i = 0; i < CurrentRamp.times.length; i++) {
        maxSize += CurrentRamp.times[i] * 60;
    }

    if (CurrentRamp.finished)
        maxSize = 0;
    if (CurrentRamp.running)
        maxSize = maxSize - (Date.now() / 1000 - CurrentRamp.start);

    var minutes = Math.floor(maxSize / 60);
    var seconds = Math.floor(maxSize % 60);

    if (minutes < 10)
        _remTime += "0";

    if (minutes >= 0)
        _remTime += minutes;
    else
        _remTime += "0"

    if ((Date.now() / 1000) % 2 < 1)
        _remTime += ":";
    else
        _remTime += " ";

    if (seconds < 10)
        _remTime += "0";
    if (seconds >= 0)
        _remTime += seconds;
    else
        _remTime += "0"

    document.getElementById('remainingTimeLabel').innerHTML = _remTime;

}

window.onload = function () {

    generateCircle('circle_1', 'Temperature', 0, 'Temp', '#d4ebd3', '#ff3f38', '#964845', '&degC', 'map 10 90 1 99');
    generateCircle('circle_2', 'Progress', 0, 'Prog', '#f79e9e', '#60bd6b', '#ad8053', '%', 'rainbow #FF0000 #00FF00');
    GraphOptions.resetDataTable();
    ResetRoutine();

    document.getElementById('DrawHeatRampSlider').checked = GraphOptions.DrawProgramedRamp;
    document.getElementById('TimeSpanSelect').value = GraphOptions.GraphSize;
    document.getElementById('ShowFutureSlider').checked = GraphOptions.ShowFuture;
    setTime();
    RequestData('reqhist');
    am_i_offline = isOnline();
};
function setTime() {

    //var x = "new_temp:" + Date.now()/1000 + " 20;";
    //ParseNewData(x);

    var date = new Date(Date.now());
    document.getElementById('timeLabel').innerHTML = formatDate(date);

    if (CurrentRamp.end > 0) {
        if (Date.now() / 1000 - CurrentRamp.end > 2 * 60 * 60) {
            ResetRoutine();
        }

    }

    updateRemTime();
    updateProg();



}

function formatDate(date) {
    if (typeof date === 'object') {
        try {

            var str = '';
            if (date.getHours() < 10)
                str += '0';
            str += date.getHours();
            if (date.getSeconds() % 2 == 0)
                str += ':';
            else
                str += ' ';
            if (date.getMinutes() < 10)
                str += '0'
            str += date.getMinutes();

            return str;
        }

        catch (error) {
            console.log(error);
            return null;
        }
    }

}

//Request Data from backend and add to chart
function RequestData(whichdata) {
    if (typeof whichdata !== 'string') {
        httpGetAsync("requpdate", ParseNewData);
    }
    else
        httpGetAsync(whichdata, ParseNewData);
}

function ParseNewData(incomeString) {
    try {
        var new_routine = 'none';
        var requestStateUpdate = false;
        for (var i = 0; i < incomeString.split(';').length; i++) {
            var _args = incomeString.split(';')[i].split(':');
            if (_args[0] == 'heat_ramp') {
                newHeatRamp(_args[1]);
            }
            if (_args[0] == 'new_temp') {

                var new_temp = _args[1].split(' ')[1];
                State.Temperature = parseInt(new_temp);
                setcircle('Temp', new_temp.substr(0, 4));
                addData(GraphOptions.DataTable, _args[1]);
            }
            if (_args[0] == 'raw') {
                handleRaw(_args[1]);
            }
            if (_args[0] == 'new_prog') {
                var new_prog = parseInt(_args[1]);
                if (!isNaN(new_prog)) {
                    State.Progress = new_prog;
                    updateProg();
                }
            }
            if (_args[0] == 'new_step') {
                var new_step = parseInt(_args[1]);
                State.CurrentStep = new_step;
                requestStateUpdate = true;
                if (new_step - 1 >= 0)
                    CurrentRamp.stepsTimeStamps[new_step - 1] = Date.now() / 1000;
            }
            if (_args[0] == 'relay_status') {
                State.relay_status = (_args[1] == true);
                requestStateUpdate = true;
            }
            if (_args[0] == 'temp_history') {
                addData(GraphOptions.DataTable, _args[1]);
            }
            if (_args[0] == 'current_state') {
                State.isRunning = (_args[1] == true);
                if (State.isRunning && !CurrentRamp.configured)
                    RequestData("/heatramp");
                requestStateUpdate = true;
            }
            if (_args[0] == 'ramp_start') {
                CurrentRamp.start = parseInt(_args[1]);
                if (CurrentRamp.start > 0) {
                    if (!State.isRunning)
                        StartRampRoutine();

                }
                console.log('start', Date.now());
            }
            if (_args[0] == 'ramp_end') {
                var end = parseInt(_args[1]);
                if (end > 0) {
                    CurrentRamp.end = end;
                    if (State.isRunning())
                        EndRampRoutine();

                }
                console.log('end', Date.now());
            }
            if (_args[0] == 'ramp_reset') {
                new_routine = 'reset';

            }
            if (_args[0] == 'set_time') {
                sendTime();
                GraphOptions.resetDataTable();

            }
            if (_args[0] == 'set_time-sucess') {
                alert('Time Synced!');
            }
            if (_args[0] == 'error') {
                alert(_args[1]);
            }
            if (_args[0] == 'current_process') {
                State.WorkerState = _args[1];
                State.update();
            }
            if (_args[0] == 'pname') {
                CurrentRamp.name = _args[1];
                State.update();
            }
            if (_args[0] == 'config') {
                Config.load(_args[1]);
            }
            if (_args[0] == 'go') {
               go(_args[1]);
            }

        }

        if (new_routine !== 'none') {
            if (new_routine === 'start')
                StartRampRoutine();
            if (new_routine === 'end')
                EndRampRoutine();
            if (new_routine === 'reset')
                ResetRoutine();
        }

        if (requestStateUpdate) {
            State.update();
        }

    }


    catch (err) {
        console.log(err);
    }
}

function handleRaw(args) {
    var commands = args.split(',');
    for (var command in commands) {
    }
}

function httpGetAsync(theUrl, callback) {
    var xmlHttp = new XMLHttpRequest();
    xmlHttp.onreadystatechange = function () {
        if (xmlHttp.readyState == 4){
            if (xmlHttp.status == 200) {
                callback(xmlHttp.responseText);
                if (!ESP_Connected) {
                    ESP_Connected = true;
                    changeESPConnected(ESP_Connected);
                }
                failedRequests = 0;
            }
            else {
                if (ESP_Connected) {
                    failedRequests++;
                if (failedRequests >= 3) {
                    ESP_Connected = false;
                    changeESPConnected(ESP_Connected);
                }
            }
            }
    }

}
xmlHttp.open("GET", theUrl, true); // true for asynchronous
xmlHttp.timeout = 1000;
try {
    xmlHttp.send();

} catch (error) {
    console.log(error);
}

}
//Change the current HTML Page
function go(where) {
    if (where[0] != "/")
    window.location.assign("/" + where);
    else
    window.location.assign(where);


}

function manualStop() {

    confirm("Are you sure you want to stop the current running process?");
    var xmlHttp = new XMLHttpRequest();
    xmlHttp.onreadystatechange = function () {
        if (xmlHttp.readyState == 4 && xmlHttp.status == 200)
            rld(xmlHttp.responseText);
    }
    xmlHttp.open("POST", '/pause', true); // true for asynchronous
    xmlHttp.send();
}

function rld(dummy) {
    document.location.reload();
}

//Graph Functions

function genRampGraph(data, steps) {
    var _currRampTime = CurrentRamp.start * 1000;
    if (_currRampTime <= 1)
        _currRampTime = Date.now();

    if (typeof (steps) !== 'number')
        steps = 10;

    if (steps <= 0)
        steps = 10;

    // data.addRow([new Date(_currRampTime), CurrentRamp.temp[0]]);
    for (var i = 0; i < CurrentRamp.temp.length; i++) {
        for (var j = 1; j < steps; j++) {
            data.addRow([new Date(_currRampTime + ((j / steps) * 60 * 1000 * CurrentRamp.times[i])), CurrentRamp.temp[i]]);
        }
        var newtime = _currRampTime + 60 * 1000 * CurrentRamp.times[i];
        _currRampTime = newtime;
    }
}

function drawChart() {

    if (GraphOptions.HoldGraph)
        return;

    //Create a separated datatable then join 
    var data = new google.visualization.DataTable();
    data.addColumn('date', 'Time');
    data.addColumn('number', 'Set Temperature');

    genRampGraph(data, 100);


    //nowMarker = new google.visualization.DataTable();
    //nowMarker.addColumn('date', 'Time');
    //nowMarker.addColumn('number', 'Set Temperature');
    //nowMarker.addRow([new Date(Date.now()), State.Temperature]);
    //var semiData = google.visualization.data.join(GraphOptions.DataTable, data, 'full', [[0, 0]], [1], [1]);
    //var finalData = google.visualization.data.join(semiData, nowMarker, 'full', [[0, 0]], [1, 2], [1]);
    var finalData = google.visualization.data.join(GraphOptions.DataTable, data, 'full', [[0, 0]], [1, 2], [1]);

    var date_formatter = new google.visualization.DateFormat({
        pattern: "HH:mm:ss"
    });
    date_formatter.format(finalData, 0);

    var dataview = new google.visualization.DataView(finalData);

    if (!GraphOptions.DrawProgramedRamp) {
        dataview.setColumns([0, 1, 2]);
    }
    else
        dataview.setColumns([0, 1, 2, 3]);
    if (GraphOptions.GraphSize > 0)
        dataview.setRows(dataview.getFilteredRows([{ column: 0, minValue: new Date(Date.now() - 60 * 1000 * GraphOptions.GraphSize) }]));
    if (!GraphOptions.ShowFuture)
        dataview.setRows(dataview.getFilteredRows([{ column: 0, maxValue: new Date(Date.now()) }]));

    if (CurrentRamp.running)
        GraphOptions.RampOption = "Line";

    var options = {
        //tooltip: {isHtml: true, trigger: 'selection'},
        title: 'Live Data' + GraphOptions.GraphName,
        backgroundColor: '#F0FFF0',
        interpolateNulls: true,
        titleTextStyle: { textAlign: 'center', justify: 'center', fontName: 'Arial', fontSize: '30', bold: 'true' },
        legend: 'none',
        dataOpacity: 1,
        pointSize: 1,
        curveType: GraphOptions.RampOption, animation: { startup: true, easing: 'in' },
        hAxis: { title: 'Time', format: 'HH:mm' },
        colors: ['red', 'darkred', 'darkgreen'],
        vAxes: {
            0: {
                logScale: false, gridlines: { count: 5 }, title: 'Temp(\xB0C)', titleTextStyle: {
                    color: 'red',
                    fontName: 'Arial',
                    fontSize: '30'
                }
            },
            1: {
                logScale: false, maxValue: 100, minValue: 50, gridlines: { count: 10 }, title: 'Humidity(%)', fontName: 'Arial', titleTextStyle: {
                    color: 'blue',
                    fontName: 'Arial',
                    fontSize: '25'
                }
            }
        },
        series: {
            0: { targetAxisIndex: 0 },
            1: { targetAxisIndex: 0 },
            2: { targetAxisIndex: 0 },
        },
        explorer: {
            keepInBounds: true,
            maxZoomIn: 4.0,
            maxZoomOut: 2.0
        },
    };

    var chart = new google.visualization.LineChart(document.getElementById('ramp_chart'));
    chart.draw(dataview, options);

}

function addData(_DataTable, _newData) {
    try {
        var data = _newData.split(',');
        if (typeof data === 'undefined')
            return;

        for (var i = 0; i < data.length; i++) {
            var data2 = data[i].split(' ');
            if (data2[0] == 0 && data2[1] == 0);
            else {
                if (_DataTable.getNumberOfRows() - 1 >= 0)
                    _DataTable.setCell(GraphOptions.DataTable.getNumberOfRows() - 1, 2, null);
                _DataTable.addRow([new Date(parseInt(data2[0]) * 1000), parseFloat(data2[1]), "point { size: 7; shape-type: circle; fill-color: #006400}"]);
                if (_DataTable.getNumberOfRows() > 5000) {
                    _DataTable.removeRow(0);
                }
            }
        }

        drawChart();

    } catch (e) {
        console.log(e);
    }
}



function isOnline() {
    return window.navigator.onLine;
}

///Circles
function generateCircle(div, _title, _initialPercent, _id, _bgcolor, _color, _numbercolor, _sign, args) {
    if (typeof div === 'object')
        div.innerHTML = createCircle(_title, _initialPercent, _id, _bgcolor, _color, _numbercolor, _sign);
    else {
        document.getElementById(div).innerHTML = createCircle(_title, _initialPercent, _id, _bgcolor, _color, _numbercolor, _sign);
    }

    var _property = new CirclePropertyHandler(_id);

    if (typeof args !== 'undefined') {
        _args = args.split(',');


        for (var i = 0; i < _args.length; i++) {
            _args2 = _args[i].split(' ');
            if (_args2[0] == 'map') {
                _property.NeedToMap = true;
                _property.PercentMap.min = parseInt(_args2[1]);
                _property.PercentMap.max = parseInt(_args2[2]);
                _property.PercentMap.new_min = parseInt(_args2[3]);
                _property.PercentMap.new_max = parseInt(_args2[4]);
            }
            else if (_args2[0] == 'rainbow') {
                _property.RainbowBar = true;
                _property.RainbowTitle = true;
                _property.Rainbow.min = _args2[1];
                _property.Rainbow.max = _args2[2];
            }
        }
    }

    HandledCircles.push(_property);
}
function createCircle(_title, _percent, _Id, _bgcolor, _color, _numbercolor, _sign) {
    if (typeof _title === 'undefined') {
        _title = null;
    }
    if (typeof _percent === 'undefined') {
        _percent = 0;
    }
    if (typeof _Id === 'undefined') {
        _Id = 'newId';

    }
    if (typeof _bgcolor === 'undefined') {
        _bgcolor = "#d4ebd3";
    }
    if (typeof _color === 'undefined') {
        _color = "darkred";
    }
    if (typeof _numbercolor === 'undefined') {
        _numbercolor = "#5f6362";
    }
    if (typeof _sign === 'undefined') {
        _sign = '%';
    }

    var percent = 440 * _percent / 100;
    percent = 440 - percent;
    var rtr = `
<div class="box">
<div class="Pbar">
<svg>
<circle style="stroke:` + _bgcolor + `;"cx ="70" cy="70" r="70"> </circle>
<circle class="c` + CircCount + `" id="` + _Id + `" cx="70" cy="70" r="70">
<style>
.box .Pbar .c`+ CircCount + ` {
 stroke:`+ _color + `;
 stroke-dasharray: 440;
 stroke-dashoffset: `+ percent + `;
 animation: mymove`+ CircCount + ` 2s ease-in;
 animation-iteration-count: 1;
}
@keyframes mymove`+ CircCount + `{
    from {stroke-dashoffset: 440;}
    to {stroke-dashoffset:` + percent + `;}
}</style>
</circle>
</svg>
<div class="prog" style="color:`+ _numbercolor + `;"> <h2 id="` + _Id + "N" + `"> ` + _percent + ` </h2><span>` + _sign + `</span></div></div><h2 class="legend">` + _title + `</h2>
</div>`;

    CircCount++;
    return rtr;
}
function setnew(_id, _newPercent, _newNumber, _color, _titleColor) {
    var _idN = _id + "N";
    if (typeof _color !== 'undefined') {
        document.getElementById(_id).style.color = _color;
    }
    if (typeof _titleColor !== 'undefined') {
        document.getElementById(_idN).style.color = _titleColor;
    }
    if (typeof _newNumber === 'undefined')
        document.getElementById(_idN).innerHTML = _newPercent;
    else
        document.getElementById(_idN).innerHTML = _newNumber;

    var _newoffset = 440 - (440 * _newPercent / 100);
    var _foward = true;
    if (_newoffset > parseInt(getComputedStyle(document.getElementById(_id)).strokeDashoffset, 10))
        _foward = false;

    //            console.log(parseInt(getComputedStyle(document.getElementById(_id)).strokeDashoffset, 10), _newoffset, _newPercent, _newNumber, _foward)

    if (!document.hidden)
        animateRoute(document.getElementById(_id), parseInt(getComputedStyle(document.getElementById(_id)).strokeDashoffset, 10), _newoffset, _foward);
}
function animateRoute(_id, _start, _end, _foward) {
    if (_foward) {
        _start -= 1;
        if (_start <= _end) {
            _start = _end;
            _id.style.strokeDashoffset = _start;
        }
        else {
            _id.style.strokeDashoffset = _start;
            setTimeout(function () { animateRoute(_id, _start, _end, true); }, 10);
        }
    }
    else {
        _start += 1;
        if (_start >= _end) {
            _start = _end;
            _id.style.strokeDashoffset = _start;
        }
        else {
            _id.style.strokeDashoffset = _start;
            setTimeout(function () { animateRoute(_id, _start, _end, false); }, 10);
        }
    }

}
function mapN(value, in_min, in_max, out_min, out_max) {
    if (value > out_max)
        return out_max;
    if (value < out_min)
        return out_min;
    return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

///Toggle Buttons
function changeDrawHeatRamp(slider) {
    GraphOptions.DrawProgramedRamp = slider.checked;
    drawChart();
}
function changeTimeSpan(object) {
    GraphOptions.GraphSize = parseInt(object.value);
    drawChart();
}
function changeShowFuture(object) {
    GraphOptions.ShowFuture = object.checked;
    drawChart();
}
function changeGraphMode(object) {
    GraphOptions.RampOption = object.value;
    drawChart();
}
function handleHoldButton() {
    GraphOptions.HoldGraph = !GraphOptions.HoldGraph;
    if (GraphOptions.HoldGraph) {
        document.getElementById('btnHold').className = "hold";
        // document.getElementById('btnHold').innerText="Release";
    }
    else {
        document.getElementById('btnHold').className = "";
        // document.getElementById('btnHold').innerText="Hold";

    }

    if (!GraphOptions.HoldGraph)
        drawChart();
}



///Ramp Routines
function NewRampRoutine(silent) {
    SetStopButton('Start');
    document.getElementById('WorkingState').style.display = 'flex';
    var _remTime = new Date(State._end * 1000);
    document.getElementById('remainingTimeLabel').innerText = _remTime.getTime() / 60000;

    document.getElementById('currentStepsLabel').innerText = '0/' + CurrentRamp.totalsteps;
    if (silent != true)
        alert('New ramp loaded!');


}

function StartRampRoutine(silent) {
    document.getElementById('WorkingState').style.display = 'flex';
    SetNewRampButton(false);
    SetStopButton("Stop");
    State.isRunning = true;
    updateRemTime();
    if (silent != true)
        alert("Ramp Started");

}

function PauseRampRoutine(params) {

}

function EndRampRoutine(silent) {
    SetStopButton('Start', true);
    SetNewRampButton(true);
    State.Finished = true;
    if (silent != true)
        alert('This Ramp has finished');

}

function ResetRoutine(params) {
    SetStopButton('Start', false);
    SetNewRampButton(true);
    CurrentRamp.reset();
    State.Progress = 0;
    document.getElementById('WorkingState').style.display = 'none';
}



function saveData() {

    if (confirm('Do you want to download this last ramp?')) {
        var _start = CurrentRamp.start;
        var text = "[Heat Ramp] [csv] [unixtimestamp in m]\n";
        text += "start = ";
        text += _start;
        text += ", Heat Ramp = {temp: ";
        text += CurrentRamp.temp;
        text += "} {time: ";
        text += CurrentRamp.times;
        text += "}\r\n";
        text += "Temperature, time, delta time\r\n";
        text += "======== Starting step: ";
        text += 1;
        text += ". Set Tempereture: ";
        text += CurrentRamp.temp[0];
        text += "\u00B0C for ";
        text += CurrentRamp.times[0];
        text += " minutes\r\n";
        var _step = 1;
        var stepFlag = true;
        if (CurrentRamp.stepsTimeStamps.length < 1)
            stepFlag = false;


        var endFlag = true;
        if (CurrentRamp.end <= 0)
            endFlag = false;

        for (var i = 0; i < GraphOptions.DataTable.getNumberOfRows(); i++) {
            var delta = GraphOptions.DataTable.getValue(i, 0).getTime() / 1000 - _start;

            if (delta >= 0) {
                if (GraphOptions.DataTable.getValue(i, 0).getTime() / 1000 >= CurrentRamp.stepsTimeStamps[_step] && stepFlag) {
                    text += "======== Starting step: ";
                    text += _step + 1;
                    text += ". Set Tempereture: ";
                    text += CurrentRamp.temp[_step];
                    text += "\u00B0C for ";
                    text += CurrentRamp.times[_step];
                    text += " minutes\r\n";
                    _step++;

                    if (CurrentRamp.stepsTimeStamps.length <= _step)
                        stepFlag = false;

                }
                if (GraphOptions.DataTable.getValue(i, 0).getTime() / 1000 >= CurrentRamp.end && endFlag) {
                    text += "========//========== Heat Ramp Ended ========//==========\r\n"
                    endFlag = false;
                }
                text += GraphOptions.DataTable.getValue(i, 1);
                text += ", ";
                text += GraphOptions.DataTable.getValue(i, 0).getTime() / 1000;
                text += ", ";
                text += delta;
                text += "\r\n";
            }
            var blob = new Blob([text], { type: "text/plain;charset=utf-8" });
            var _now = new Date(Date.now());

        }
        saveAs(blob, "HeatRampLog[" + _now.toLocaleDateString() + "].txt");

    }
}

function getnextStep(start, time) {
    return start + time * 60;
}

function stopbutton() {

    if (State.isRunning)
        RequestData('/stop');
    else
        RequestData('/start');
}


function sendTime() {
    var text = "settime?time=";
    text += Math.round(Date.now() / 1000);
    RequestData(text);
}

function makeTable(_selectedRow) {
    if (typeof _selectedRow === 'undefined')
        return;
    var newTable = "";
    newTable = "<tr><th>Step</th><th>Temperature (&deg;C)</th><th>Time (Mins)</th></tr>";
    for (var i = 0; i < CurrentRamp.temp.length; i++) {

        newTable += "<tr ";
        if (i === _selectedRow)
            newTable += "class = \"selectedRow\"";
        else if (i % 2 === 0)
            newTable += "class = \"even-Row\"";
        newTable += "><td>" + (i + 1) + "</td><td>" + CurrentRamp.temp[i] + "</td><td>" + CurrentRamp.times[i] + "</td></tr>"
    }
    document.getElementById("offline-table").innerHTML = newTable;
    document.getElementById("offline-div").style.display = 'flex';

}

function updateProg(prog) {
    setcircle('Prog', State.Progress);
}

function changeESPConnected(isESPConnected) {
    if (typeof (isESPConnected) === 'undefined')
        return;

    if (isESPConnected)//do Online stuff
    {

        document.getElementById("esp_connected").style.display = "block";
        document.getElementById("esp_not_connected").style.display = "none";
        drawChart();       
        ESP_Connected = true;
    }
    else//do Offline stuff 
    {
        document.getElementById("esp_connected").style.display = "none";
        document.getElementById("esp_not_connected").style.display = "block";
        ESP_Connected = false;
        console.log('esp-offline');
        animatereconect(0);

    }

}

function animatereconect(size) {

    var div = document.getElementById("reconnect-animation");
    var divhtml = "";
    for (let index = 0; index < size; index++) {
       
        divhtml+= ".";
    }    

    size ++;
    if (size >= 4)
    size = 0;
    
    div.innerHTML = divhtml;

    if (!ESP_Connected)
    setTimeout(() => {animatereconect(size)},500);
}