RequestData('/loadconfig');
RequestData('/reqwifisearch');
var ReqInfo = setInterval(() => { RequestData('/systeminfo') }, 1500);
var ESP_Connected = true;
var failedRequests = 0;
var wifiAPs = [, ,]
const sysinfo = new InfoCreator();
const config = new Settings();

var test = "config:tolerance=0.50,calibration=0.00,TEMPERATURE_RESOLUTION=9,sample_freq=1,WIFI_SSID=MCarvalho,www_username=admin,WIFI_AP_SSID=ESP32,use_static_ip=0,static_ip=,enable_upload=0,UploadServiceType=0,UploadLogin=,enableCache=1,continousLog=1,enable_backup=0,backup_freq=0,lastbackup=0,RELAY_PIN=13,ONEWIRE_BUS=16,host_name=Estufa,WIFI_AP_PASSWORD=ESP32ESP32,;";

function InfoCreator() {
    this.ram = 0;
    this.ramMax = 0;
    this.SD = 0;
    this.SDMax = 0;
    this.internalTemp = 0;
    this.SPIFFS = 0;
    this.SPIFFSMax = 0;
    this.load = function (params) {
        var value = "";
        var variable = "";
        var nextChar;
        var isValue = false;

        for (var i = 0; i < params.length; i++) {
            nextChar = params[i];
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
                    if (variable == "SD")
                        this.SD = parseInt(value);
                    else if (variable == "SDMax")
                        this.SDMax = parseInt(value);
                    else if (variable == "ram")
                        this.ram = parseInt(value);
                    else if (variable == "ramMax")
                        this.ramMax = parseInt(value);
                    else if (variable == "SPIFFS")
                        this.SPIFFS = parseInt(value);
                    else if (variable == "SPIFFSMax")
                        this.SPIFFSMax = parseInt(value);
                    else if (variable == "internalTemp")
                        this.internalTemp = parseFloat(value);

                    value = "";
                    variable = "";
                    isValue = false;
                }
            }
        }
        this.update();
    };
    this.update = function () {
        setnew('internalTemp', this.internalTemp);

        if (ram >= 85)
            setnew('ram', this.ram, this.ram, 'darkred', 'darkred');
        else
            setnew('ram', this.ram, this.ram, 'darkblue', 'darkblue');

        if (ram >= 85)
            setnew('sdcard', this.SD, this.SD, 'darkred', 'darkred');
        else
            setnew('sdcard', this.SD, this.SD, 'darkgreen', 'darkgreen');

        if (ram >= 85)
            setnew('SPIFFS', this.SPIFFS, this.SPIFFS, 'darkred', 'darkred');
        else
            setnew('SPIFFS', this.SPIFFS, this.SPIFFS, 'darkgreen', 'darkgreen');


    };
}

function Settings() {
    this.tolerance = 0;
    // Will mantain temperature within this tolerance.
    this.calibration = 0;
    //Resolution of DS18b20 Sensor this.TEMPERATURE_RESOLUTION = 9;

    //Web Site Credentials
    this.www_username = "";
    this.www_password = "";

    //Wifi Credentials
    this.WIFI_SSID = "";
    this.WIFI_PASSWORD = "";
    this.WIFI_AP_SSID = "";
    this.WIFI_AP_PASSWORD = "";
    this.use_static_ip = false;
    this.static_ip = "";

    /*
    * Available Services: Gmail = 0
    *                    Google Drive = 1
    */
    this.enable_upload = false;
    this.UploadServiceType = 0;
    this.UploadLogin = "";
    this.UploadPassword = "";

    //Sample rate for logging
    this.sample_freq = 0;

    //Config of Pins
    this.RELAY_PIN = 0;
    this.ONEWIRE_BUS = 0;


    /*
        0 = DS1820
        1 = DS1820 High Temp
        2 = NTC thermopar
    */
    this.temp_sensor_type = 0;

    this.NTC_BASE_RESISTANCE = 0;
    this.NTC_BASE_TEMP = 0;
    this.NTC_READ_PIN = 0;
    this.NTC_ENABLE_PIN = 0;
    this.NTC_BETA = 0;
    this.ADC_REQ_CORRECT = false;
    this.ADC_CORRECTION = "";


    //Enables Cacheing; this.enableCache = true; this.continousLog = true;

    //Backup Options this.enable_backup = false; this.backup_freq = 0; this.lastbackup = 0;

    this.host_name = "";

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
                    else if (variable == "calibration")
                        this.calibration = parseFloat(value);
                    else if (variable == "WIFI_SSID")
                        this.WIFI_SSID = value;
                    else if (variable == "WIFI_PASSWORD")
                        this.WIFI_PASSWORD = value;
                    else if (variable == "WIFI_AP_SSID")
                        this.WIFI_AP_SSID = value;
                    else if (variable == "WIFI_AP_PASSWORD")
                        this.WIFI_AP_PASSWORD = value;
                    else if (variable == "www_username")
                        this.www_username = value;
                    else if (variable == "www_password")
                        this.www_password = value;
                    else if (variable == "TEMPERATURE_RESOLUTION")
                        this.TEMPERATURE_RESOLUTION = parseInt(value);
                    else if (variable == "sample_freq")
                        this.sample_freq = parseInt(value);
                    else if (variable == "use_static_ip ")
                        this.use_static_ip = value == "1" ? true : false;
                    else if (variable == "static_ip")
                        this.static_ip = value;
                    else if (variable == "enableCache")
                        this.enableCache = value == "1" ? true : false;
                    else if (variable == "continousLog")
                        this.continousLog = value == "1" ? true : false;
                    else if (variable == "enable_backup")
                        this.enable_backup = value == "1" ? true : false;
                    else if (variable == "backup_freq")
                        this.backup_freq = parseInt(value);
                    else if (variable == "lastbackup")
                        this.lastbackup = parseInt(value);
                    else if (variable == "host_name")
                        this.host_name = value;
                    else if (variable == "RELAY_PIN")
                        this.RELAY_PIN = parseInt(value);
                    else if (variable == "ONEWIRE_BUS")
                        this.ONEWIRE_BUS = parseInt(value);
                    else if (variable == "UploadServiceType")
                        this.UploadServiceType = parseInt(value);
                    else if (variable == "enable_upload")
                        this.enable_upload = value == "1" ? true : false;
                    else if (variable == "UploadLogin")
                        this.UploadLogin = value;
                    else if (variable == "UploadPassword")
                        this.UploadPassword = value;
                    else if (variable == "temp_sensor_type")
                        this.temp_sensor_type = parseInt(value);
                    else if (variable == "NTC_BASE_RESISTANCE")
                        this.NTC_BASE_RESISTANCE = parseInt(value);
                    else if (variable == "NTC_BASE_TEMP")
                        this.NTC_BASE_TEMP = parseInt(value);
                    else if (variable == "NTC_READ_PIN")
                        this.NTC_READ_PIN = parseInt(value);
                    else if (variable == "NTC_ENABLE_PIN")
                        this.NTC_ENABLE_PIN = parseInt(value);
                    else if (variable == "NTC_BETA")
                        this.NTC_BETA = parseInt(value);

                    value = "";
                    variable = "";
                    isValue = false;
                }
            }
        }
    };
}

function RequestData(whichdata) {
    if (typeof whichdata !== 'string') {
        httpGetAsync("requpdate", ParseNewData);
    }
    else
        httpGetAsync(whichdata, ParseNewData);
}

function ParseNewData(incomeString) {
    try {

        for (var i = 0; i < incomeString.split(';').length; i++) {
            var _args = incomeString.split(';')[i].split(':');
            if (_args[0] == 'config') {
                config.load(_args[1]);
            }

            if (_args[0] == 'sysinfo') {
                sysinfo.load(_args[1]);

            }
            if (_args[0] == 'reqwifisearch') {
                var aps = _args[1].split(',');
                wifiAPs = [, ,];
                for (var i = 0; i < aps.length; aps++) {
                    if (aps[i].split('=')[0] != "" && aps[i].split('=')[i][1] != "" && aps[i].split('=')[0] != null && aps[i].split('=')[1] != null) {
                        wifiAPs[i][0] = aps[i].split('=')[0];
                        wifiAPs[i][1] = aps[i].split('=')[1];

                    }
                }

            }
            if (_args[0] == 'cal_temp') {
                if (!isNaN((parseFloat(_args[1]))))
                    setnew('raw_temp', _args[1]);
                if (!isNaN((parseFloat(_args[2]))))
                    setnew('cal_temp', _args[2]);
            }
            if (_args[0] == 'error') {
                alert(_args[1]);
            }

        }


    }


    catch (err) {
        console.log(err);
    }
}

function httpGetAsync(theUrl, callback) {
    var xmlHttp = new XMLHttpRequest();
    xmlHttp.onreadystatechange = function () {
        if (xmlHttp.readyState == 4) {
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

window.onload = function () {
    generateCircle('circle_1', 'RAM:', 0, 'ram', '#d4ebd3', 'darkblue', 'darkblue', '%');
    generateCircle('circle_2', 'TEMP:', 0, 'internalTemp', '#d4ebd3', 'darkred', 'darkred', '&deg;C', 'map 20 80 0 100');
    generateCircle('circle_3', 'SD:', 0, 'sdcard', '#d4ebd3', 'darkgreen', 'darkgreen', '%');
    generateCircle('circle_4', 'SPIFFS:', 0, 'SPIFFS', '#d4ebd3', 'darkgreen', 'darkgreen', '%');
    generateCircle('cal_raw_sensor', 'Raw:', 25, 'raw_temp', '#d4ebd3', 'red', 'red', '&deg;C', 'map 20 80 0 100');
    generateCircle('cal_sensor', 'Calib:', 25, 'cal_temp', '#d4ebd3', 'darkred', 'darkred', '&deg;C', 'map 20 80 0 100');
    setLabelSize('cal_temp', '45px');
    setLabelSize('raw_temp', '45px');
    setnew('raw_temp', '25.25');
    setnewangle('raw_temp', 120);
    setnew('cal_temp', '25.25');
    setnewangle('cal_temp', 120);
    setnewangle('ram', 120);
    setnewangle('internalTemp', 120);
    setnewangle('sdcard', 120);
    setnewangle('SPIFFS', 120);

    ParseNewData(test);
    draw('wifi');

}

function changeESPConnected(isESPConnected) {
    if (typeof (isESPConnected) === 'undefined')
        return;

    if (isESPConnected)//do Online stuff
    {


        ESP_Connected = true;
    }
    else//do Offline stuff 
    {
        ESP_Connected = false;

    }

}

function animatereconect(size) {

    if (!ESP_Connected)
        setTimeout(() => { animatereconect(size) }, 500);
}

function enable(id, value) {
    if (value)
        $(id).removeAttribute("disabled");
    else
        $(id).setAttribute("disabled", "disabled");

}

function showCalibrationDiv(show) {
    if (show) //Show Calibration Div
    {
        document.getElementById("cal_div").style.display = "block";
    }
    else //Hide Calibration Div
    {
        document.getElementById("cal_div").style.display = "none";

    }
}

function handleSettingsChanges(sender, area, type) {
    if (typeof (type) === 'undefined')
        type = 'common';
    var variable = "";
    variable += sender.id;
    console.log(sender.id)

    if (type == 'common') {
        config[variable] = sender.value;
        draw(area);
        return;
    }
    if (type == 'suppressdraw')
    {
        config[variable] = sender.value;
        return;
    }
}

function draw(params) {
    if (params == 'wifi') {
        $('wifi_ssid').value = config.WIFI_SSID;
        $('WIFI_PASSWORD').value = config.WIFI_PASSWORD;
        $('ap_ssid').value = config.WIFI_AP_SSID;
        $('WIFI_AP_PASSWORD').value = config.WIFI_AP_PASSWORD;
        $('ip_address').value = config.static_ip;
        $('use_static_ip').checked = config.use_static_ip;

        enable('ip_address', config.use_static_ip);
        tooglewificustomssid($('wifi_custom_ssid').checked);
        //enable()

    }
}

function tooglewificustomssid(value) {
    if (value) {
        $('wifi_ssid_list_div').style.display = 'none';
        $('wifi_custom_ssid_div').style.display = 'flex'
    }
    else {
        $('wifi_ssid_list_div').style.display = 'flex';
        $('wifi_custom_ssid_div').style.display = 'none'
    }

}
function wifi_connect() {

}

function $(name) {
    return document.getElementById(name);
}