

google.charts.load('current', { 'packages': ['corechart'] });
google.charts.setOnLoadCallback(drawChart);

const GraphOptions = new GraphOptionsCreator();
const CurrentRamp = new CurrentRampCreator();

function GraphOptionsCreator() {
    this.DrawProgramedRamp = true;
    this.RampOption = 'function';
    this.GraphSize = -1;
    this.DataTable = undefined;
    this.ShowFuture = true;
    this.HoldGraph = false;
    this.GraphName = "Live Data";
    this.hColors = ['red', 'darkred', 'darkgreen'];
    this.resetDataTable = function () {
        var data = new google.visualization.DataTable();
        data.addColumn('date', 'Time');
        data.addColumn('number', 'Temperature');
        data.addColumn({ 'type': 'string', 'role': 'style' });
        this.DataTable = data;
    }
}

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

        var finalData;
    //Create a separated datatable then join 
    if (CurrentRamp.times.length != 0 && CurrentRamp.temp.length != 0) {
        var data = new google.visualization.DataTable();
        data.addColumn('date', 'Time');
        data.addColumn('number', 'Set Temperature');

        genRampGraph(data, 100);
        finalData = google.visualization.data.join(GraphOptions.DataTable, data, 'full', [[0, 0]], [1, 2], [1]);
    }
    else {
        finalData = GraphOptions.DataTable;
    }


    var date_formatter = new google.visualization.DateFormat({
        pattern: "HH:mm:ss"
    });
    date_formatter.format(finalData, 0);

    var dataview = new google.visualization.DataView(finalData);

    if (!GraphOptions.DrawProgramedRamp) {
        dataview.setColumns([0, 1, 2]);
    }

    if (GraphOptions.GraphSize > 0)
        dataview.setRows(dataview.getFilteredRows([{ column: 0, minValue: new Date(Date.now() - 60 * 1000 * GraphOptions.GraphSize) }]));
    if (!GraphOptions.ShowFuture)
        dataview.setRows(dataview.getFilteredRows([{ column: 0, maxValue: new Date(Date.now()) }]));

    if (CurrentRamp.running)
        GraphOptions.RampOption = "Line";

    var options = {
        //tooltip: {isHtml: true, trigger: 'selection'},
        title: GraphOptions.GraphName,
        backgroundColor: '#F0FFF0',
        interpolateNulls: true,
        titleTextStyle: { textAlign: 'center', justify: 'center', fontName: 'Arial', fontSize: '30', bold: 'true' },
        legend: 'none',
        dataOpacity: 1,
        pointSize: 1,
        curveType: GraphOptions.RampOption, animation: { startup: true, easing: 'in' },
        hAxis: { title: 'Time', format: 'HH:mm' },
        colors: GraphOptions.hColors,
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

