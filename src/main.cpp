/*                        TODO LIST
* 1. Calibration Function
* 4. Implement Configuration
* 5. Heat up every step mode
* 8. Finish up doPage and Main Page
* 9. Translate.
* 10. The-End (Web-Side)
*
*
*
*
*
*/
#include <WiFi.h>
#include <WiFiClient.h>
#include <ESPmDNS.h>
#include <C:\Users\Vity1\Documents\Arduino\libraries\DallasTemperature\DallasTemperature.h>
#include <C:\Users\Vity1\Documents\Arduino\libraries\Time-master\TimeLib.h>
#include <HTTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <SPIFFS.h>
#include "FS.h"
#include <C:\Users\Vity1\Documents\PlatformIO\Projects\Estufa_WebPage\.pio\libdeps\esp32dev\ESPAsyncWebServer-esphome\src\ESPAsyncWebServer.h>
#include <DNSServer.h>
#include "SD_MMC.h"
#include "esp_task_wdt.h"

#define DEBUGWEB
// #define DEBUGLOG
// #define DEBUGGENERAL
// #define DEBUGTEMP
// #define DEBUGRAMP

// Globals

//protects the SD_MMC fs of acessing more than available free files descriptors
static SemaphoreHandle_t sd_mutex;
static SemaphoreHandle_t get_ap_mutex;

#define HISTORY_SIZE 1000
#define MAX_ADC_CORRECTION_EXPONENT 15
#define MAX_STEPS 22
#define default_relay_pin 13
#define default_one_wire_bus 16
#define MAX_CACHE_PAGES 10

#define GET_VARIABLE_NAME(Variable) (#Variable)

float DS18Temp[HISTORY_SIZE];
int DS18Time[HISTORY_SIZE];
uint16_t DS18_CurrentIndex = 0;
uint LastTime = 0;
int cycle = 0;
uint startup_time = 0;
int random_temp = 300; //delete this after using

byte currentProg = 0;

//Update Variables
bool is_updating = false;
float update_progress = 0;

//Flags
bool new_ramp = false;
bool new_step = false;
bool new_prog = false;
bool new_relay_state = false;
bool boot_reset = false;

bool is_time_configured = false;

bool is_SPIFFSS_Mount = false;
bool is_SD_Mount = false;
void File_Writer(String, String, bool, bool);
void clearDir(String);
String getApAvailables();

int oldMillis = 0;
int oldLoopMillis = 0;

//references for funcitons
String normalizedDayMonth();
String getCurrentProcess();
void StandardTesting();

struct DebugFlags
{
    bool web = false;
    bool ramp = false;
    bool temp = false;
    bool log = false;
    bool general = false;
};

AsyncWebServer server(80);

struct Settings
{

    // Will mantain temperature within this tolerance.
    float tolerance = 0.5;
    // Will mantain temperature within this tolerance.
    float calibration = 0;
    //Resolution of DS18b20 Sensor
    byte TEMPERATURE_RESOLUTION = 9;

    //Web Site Credentials
    String www_username = "admin";
    String www_password = "admin";

    //Wifi Credentials
    String WIFI_SSID = "MCarvalho";
    String WIFI_PASSWORD = "salvador";
    String WIFI_AP_SSID = "ESP32";
    String WIFI_AP_PASSWORD = "ESP32ESP32";

    bool use_static_ip = false;
    String static_ip = "";

    /*
    * Available Services: Gmail = 0
    *                    Google Drive = 1
    */
    byte UploadServiceType = 0;
    bool enable_upload = false;
    String UploadLogin = "";
    String UploadPassword = "";

    /*missing*/ bool auto_upload = false;
    /*missing*/ uint auto_upload_freq = 0;

    //Sample rate for logging
    uint16_t sample_freq = 1;

    //Config of Pins
    uint8_t RELAY_PIN = 13;
    uint8_t ONEWIRE_BUS = 16;

    /*
        0 = DS1820
        1 = DS1820 High Temp
        2 = NTC thermopar
    */
    byte temp_sensor_type = 0;

    uint NTC_BASE_RESISTANCE = 100000;
    uint16_t NTC_BETA = 3450;
    int8_t NTC_BASE_TEMP = 25;
    uint8_t NTC_READ_PIN = 12;
    uint8_t NTC_ENABLE_PIN = -1;

    /*missing*/ bool ADC_REQ_CORRECT = false;
    /*missing*/ String ADC_CORRECTION = "1x0+2e-9x1+-3e-9x2+0.35e-9x5+3.5e-9x4+-0.513514546848464e-9x6+;";

    //Enables Cacheing;
    bool enableCache = true;
    bool continousLog = true;

    //Backup Options
    bool enable_backup = false;
    uint backup_freq = 0;
    uint lastbackup = 0;

    String host_name = "Estufa";

    /*Creates a string with the variables to be parsed elsewhere.
    *@param complete: wheater or not you want to send passwords aswell.
    */
    String toString(bool complete = false)
    {
        String message = "";
        message += "tolerance=";
        message += tolerance;
        message += ",calibration=";
        message += calibration;
        message += ",TEMPERATURE_RESOLUTION=";
        message += TEMPERATURE_RESOLUTION;
        message += ",sample_freq=";
        message += sample_freq;
        message += ",WIFI_SSID=";
        message += WIFI_SSID;
        if (complete)
        {
            message += ",WIFI_PASSWORD=";
            message += WIFI_PASSWORD;
        }
        message += ",www_username=";
        message += www_username;
        if (complete)
        {
            message += ",www_password=";
            message += www_password;
        }
        message += ",WIFI_AP_SSID=";
        message += WIFI_AP_SSID;
        if (complete)
        {
            message += ",WIFI_AP_PASSWORD=";
            message += WIFI_AP_PASSWORD;
        }
        message += ",use_static_ip=";
        message += use_static_ip;
        message += ",static_ip=";
        message += static_ip;
        message += ",enable_upload=";
        message += enable_upload;
        message += ",UploadServiceType=";
        message += UploadServiceType;
        message += ",UploadLogin=";
        message += UploadLogin;
        if (complete)
        {
            message += ",UploadPassword=";
            message += UploadPassword;
        }
        message += ",enableCache=";
        message += enableCache;
        message += ",continousLog=";
        message += continousLog;
        message += ",enable_backup=";
        message += enable_backup;
        message += ",backup_freq=";
        message += backup_freq;
        message += ",lastbackup=";
        message += lastbackup;
        message += ",RELAY_PIN=";
        message += RELAY_PIN;
        message += ",ONEWIRE_BUS=";
        message += ONEWIRE_BUS;
        message += ",host_name=";
        message += host_name;
        message += ",temp_sensor_type=";
        message += temp_sensor_type;
        message += ",NTC_BASE_RESISTANCE=";
        message += NTC_BASE_RESISTANCE;
        message += ",NTC_BASE_TEMP=";
        message += NTC_BASE_TEMP;
        message += ",NTC_READ_PIN=";
        message += NTC_READ_PIN;
        message += ",NTC_ENABLE_PIN=";
        message += NTC_ENABLE_PIN;
        message += ",NTC_BETA=";
        message += NTC_BETA;
        message += ",;";

        return message;
    }

    //Resets the config to  its hardcoded values
    void reset()
    {
        tolerance = 0.5;
        TEMPERATURE_RESOLUTION = 9;
        www_username = "admin";
        www_password = "admin";
        sample_freq = 1;
        WIFI_SSID = "MCarvalho";
        WIFI_PASSWORD = "salvador";
    }

    //parse new values from a string.
    void fromString(String newConfig, bool partialUpdate = false)
    {
        if (!partialUpdate)
            reset();

        String value = "";
        String variable = "";
        char nextChar;
        bool isValue = false;
        for (size_t i = 0; i < newConfig.length(); i++)
        {
            nextChar = newConfig[i];
            if (!isValue)
            {
                if (nextChar != '=')
                    variable += nextChar;
                else
                    isValue = true;
            }
            else
            {
                if (nextChar != ',')
                    value += nextChar;
                else
                {
                    if (variable == "tolerance")
                        tolerance = atof(value.c_str());
                    else if (variable == "calibration")
                        calibration = atof(value.c_str());
                    else if (variable == "WIFI_SSID")
                        WIFI_SSID = value;
                    else if (variable == "WIFI_PASSWORD")
                        WIFI_PASSWORD = value;
                    else if (variable == "WIFI_AP_SSID")
                        WIFI_AP_SSID = value;
                    else if (variable == "WIFI_AP_PASSWORD")
                        WIFI_AP_PASSWORD = value;
                    else if (variable == "www_username")
                        www_username = value;
                    else if (variable == "www_password")
                        www_password = value;
                    else if (variable == "TEMPERATURE_RESOLUTION")
                        TEMPERATURE_RESOLUTION = atoi(value.c_str());
                    else if (variable == "sample_freq")
                        sample_freq = atoi(value.c_str());
                    else if (variable == "use_static_ip ")
                        use_static_ip = value == "1" ? true : false;
                    else if (variable == "static_ip")
                        static_ip = value;
                    else if (variable == "enableCache")
                        enableCache = value == "1" ? true : false;
                    else if (variable == "continousLog")
                        continousLog = value == "1" ? true : false;
                    else if (variable == "enable_backup")
                        enable_backup = value == "1" ? true : false;
                    else if (variable == "backup_freq")
                        backup_freq = atoi(value.c_str());
                    else if (variable == "lastbackup")
                        lastbackup = atoi(value.c_str());
                    else if (variable == "host_name")
                        host_name = value;
                    else if (variable == "RELAY_PIN")
                        RELAY_PIN = atoi(value.c_str());
                    else if (variable == "ONEWIRE_BUS")
                        ONEWIRE_BUS = atoi(value.c_str());
                    else if (variable == "UploadServiceType")
                        UploadServiceType = atoi(value.c_str());
                    else if (variable == "enable_upload")
                        enable_upload = value == "1" ? true : false;
                    else if (variable == "UploadLogin")
                        UploadLogin = value;
                    else if (variable == "UploadPassword")
                        UploadPassword = value;
                    else if (variable == "temp_sensor_type")
                        temp_sensor_type = atoi(value.c_str());
                    else if (variable == "NTC_BASE_RESISTANCE")
                        NTC_BASE_RESISTANCE = atoi(value.c_str());
                    else if (variable == "NTC_BASE_TEMP")
                        NTC_BASE_TEMP = atoi(value.c_str());
                    else if (variable == "NTC_READ_PIN")
                        NTC_READ_PIN = atoi(value.c_str());
                    else if (variable == "NTC_ENABLE_PIN")
                        NTC_ENABLE_PIN = atoi(value.c_str());
                    else if (variable == "NTC_BETA")
                        NTC_BETA = atoi(value.c_str());

                    value = "";
                    variable = "";
                    isValue = false;
                }
            }
        }
    }

    //Saves the current config to '/config.cfg' in the SD card
    void save()
    {
        String _setting = toString(true);
        String saveString = "";
        saveString += checksum(_setting);
        saveString += ':';
        saveString += _setting;
        // Serial.println(_setting);
        // Serial.println(checksum(_setting));
        // Serial.println(saveString);
        File_Writer("/config.cfg", saveString, false, false);
        return;
    }

    bool LoadValidated(String newSetting)
    {
        bool checksumPassed = false;
        bool done = false;
        byte beginIndex = 0;
        String newchecksum = "";

        for (size_t i = 0; i < newSetting.length() && !done; i++)
        {
            if (newSetting[i] != ':')
                newchecksum += newSetting[i];
            else
            {
                done = true;
                beginIndex = i + 1;
            }
        }
        String SettingsString = newSetting.substring(beginIndex);

        if (checksum(SettingsString) == atoi(newchecksum.c_str()) && done) // if done is false, atoi(newchecksum.c_str()) = 0, therefore u can get
        {
            checksumPassed = true;
            fromString(SettingsString);
        }

        // Serial.println(checksum(SettingsString));
        // Serial.println(atoi(newchecksum.c_str()));
        // Serial.println(SettingsString);

        return checksumPassed;
    }

private:
    byte checksum(String evalString, int base = 100)
    {
        uint totalSum = 0;
        for (size_t i = 0; i < evalString.length(); i++)
        {
            totalSum += (int)evalString[0];
        }

        return totalSum % base;
    }
};

struct ADCCorrector
{
private:
    /*Coefficients array. Expoents are the indexes
    * avg calc time @ max expoent 6 : double 13us, float 4us
    */
    double Coeffs[MAX_ADC_CORRECTION_EXPONENT];

public:
    //Simpler pow functions for integers
    uint myPow(uint value, byte expoent)
    {
        uint finalValue = 1;

        // if (expoent == 0)
        // return 1;

        for (size_t i = 0; i < expoent; i++)
        {
            finalValue = finalValue * value;
        }

        return finalValue;
    }

    /*Get Coefficients from a String 
    * @param Polynome: Should be in the format "'coeff'x'expoent'+"
    * do note that the + sign at the end of every x factor is nescessary.
    * The coefficients found will be store at the index of their expoent.
    * you can change @param MAX_ADC_CORRECTION_EXPONENT as it will determine
    * the boundries of the object.
    */
    void fromString(String Polynome)
    {

        char nextChar = ' ';
        bool xFound = false;
        String coeff = "";
        String expoent = "";

        for (size_t i = 0; i < Polynome.length(); i++)
        {
            nextChar = Polynome[i];

            if (xFound)
            {
                if (nextChar == '+')
                {
                    if (coeff == "")
                        coeff == "0";

                    if (expoent == "")
                        expoent == "0";

                    byte index = atoi(expoent.c_str());

                    if (index >= 0 && index < MAX_ADC_CORRECTION_EXPONENT)
                        Coeffs[index] = atof(coeff.c_str());

                    xFound = false;
                    coeff = "";
                    expoent = "";
                }
                else
                    expoent += nextChar;
            }
            else
            {
                if (nextChar == 'x')
                    xFound = true;
                else
                    coeff += nextChar;
            }
        }
    }

    //Sets all values in the array to 0;
    void setup()
    {
        for (size_t i = 0; i < MAX_ADC_CORRECTION_EXPONENT; i++)
        {
            Coeffs[i] = 0;
        }
    }

    //Returns the ajusted value (passes the @param value by the polynome stored)
    uint16_t calculate(uint16_t value)
    {
        float trueValue = 0;
        for (size_t i = 0; i < 10; i++)
        {
            trueValue += Coeffs[i] * myPow(value, i);
        }
        return round(trueValue);
    }

    //Prints the current polynome to Serial
    //a different HardwareSerial can be Specified
    void print(HardwareSerial serial = Serial)
    {

        serial.println("Current Polynome: ");
        serial.print("	");
        bool first = true;
        for (size_t i = 0; i < MAX_ADC_CORRECTION_EXPONENT; i++)
        {
            if (Coeffs[i] != 0)
            {
                if (!first)
                    serial.print(" + ");

                first = false;
                serial.print(String(Coeffs[i], 10));
                serial.print(" * x^");
                serial.print(i);
            }
        }
        serial.println();
    }

    //Produces a String with the current polynome that can be used to recreate this object.
    String toString()
    {
        String msg = "";
        for (size_t i = 0; i < MAX_ADC_CORRECTION_EXPONENT; i++)
        {
            if (Coeffs[i] != 0)
            {
                msg += Coeffs[i];
                msg += "x";
                msg += i;
                msg += "+";
            }
        }

        return msg;
    }
};

ADCCorrector adcCorrector;
Settings Config;

struct HeatRamp
{
private:
    String csvFile = "/Logs/Noname.csv";
    String logFile = "/Logs/Noname.log";

public:
    String projectName;
    byte currentstep = 0;
    byte totalsteps = 0;
    float temperatureSteps[MAX_STEPS + 1];
    float minuteSteps[MAX_STEPS + 1];
    unsigned int StepsTimes[MAX_STEPS + 1];
    unsigned int HeatingSteps[MAX_STEPS + 1];

    unsigned int startTime = 0;
    unsigned int startHeating = 0;
    unsigned int endTime = 0;

    bool programmedStart = false;
    bool heatUp = false;
    bool heatUpEveryStep = false;
    bool heating = false;
    bool running = false;
    bool finished = false;
    bool configured = false;
    //Prints the Heatramp Object
    void print()
    {
        String ramp = "";
        ramp += "Name: ";
        ramp += projectName;
        ramp += "\nTotal steps: ";
        ramp += totalsteps;
        ramp += "\nTemperature, Time, Step Time\n";
        for (size_t i = 0; i < totalsteps; i++)
        {
            ramp += temperatureSteps[i];
            ramp += ", ";
            ramp += minuteSteps[i];
            ramp += ", ";
            ramp += StepsTimes[i];
            ramp += ",\n";
        }
        ramp += "Log file: ";
        ramp += logFile;
        ramp += "\nStart: ";
        ramp += startTime;
        ramp += "\nProgram Start: ";
        ramp += programmedStart;
        ramp += "\nHeatUp: ";
        ramp += heatUp;

        Serial.println(ramp);
    }
    //Sets the Log Filename
    void startLogs()
    {
        generateCsv();
        // generateLog();
    }
    void generateCsv()
    {
        if (!is_SD_Mount)
            return;
        String baseName = "/Logs/" + projectName;
        bool nameAvailable = false;
        u_int nextName = 1;
        if (!SD_MMC.exists(baseName + ".csv"))
        {
            csvFile = baseName + ".csv";
            //createLogFile(logFile);
            nameAvailable = true;
        }

        while (!nameAvailable)
        {
            String newName = baseName + "(" + nextName + ").csv";
            nextName++;
            if (!SD_MMC.exists(newName))
            {
                csvFile = newName;
                nameAvailable = true;
            }
        }

        Serial.print("Creating '");
        Serial.println(csvFile);
        File_Writer(csvFile, "'Time Stamp','Relative Time','Temperature','Set Temperature','Relay Status','Current Step','Comment'\n", true, false);
    }
    //Starts the Log file
    void generateLog()
    {
        if (!is_SD_Mount)
            return;
        String baseName = csvFile.substring(0, csvFile.lastIndexOf('.'));
        bool nameAvailable = false;
        u_int nextName = 1;
        baseName += "[";
        baseName += normalizedDayMonth();
        baseName += "]";
        if (!SD_MMC.exists(baseName + ".log"))
        {
            logFile = baseName + ".log";
            nameAvailable = true;
        }

        while (!nameAvailable)
        {
            String newName = baseName + "(" + nextName + ").csv";
            nextName++;
            if (!SD_MMC.exists(newName))
            {
                logFile = newName;
                nameAvailable = true;
            }
        }

        Serial.print("Creating '");
        Serial.print(logFile);
        Serial.println("'. Time, Temp, SetTemp, Relay, CurrentStep");
        File_Writer(logFile, "Time,Temperature,Set Temperature, Relay Status, Current Step\n", true, false);
    }
    //Calculate the StepTimes array
    void calculateStepTimes()
    {
        Serial.println("Calculating StepTimes: ");
        for (size_t i = 0; i < totalsteps; i++)
        {
            StepsTimes[i] = startTime;
            for (size_t j = 0; j < i + 1; j++)
            {
                StepsTimes[i] += (int)(minuteSteps[j] * 60);
            }
            Serial.print("Step: ");
            Serial.print(i);
            Serial.print(" -> ");
            Serial.println(StepsTimes[i]);
        }
    }
    //logs the temperature and all the variables to a csv file associated to your projectname.
    void csv(float temperature, bool RelayStatus, String comment = "")
    {
        String Message = "";
        Message += now();
        Message += ",";
        if (heating)
        {
            if (now() > startHeating)
            {
                Message += startHeating - now();
            }
            else
            {
                Message += now() - startHeating;
            }
        }
        else
        {
            if (now() > startTime)
            {
                Message += startTime - now();
            }
            else
            {
                Message += now() - startTime;
            }
        }
        Message += ",";
        Message += temperature;
        Message += ",";
        Message += temperatureSteps[currentstep];
        Message += ",";
        Message += RelayStatus;
        Message += ",";
        Message += currentstep + 1;
        Message += ",";
        Message += comment;
        Message += "\n";
        File_Writer(csvFile, Message, true, false);
    }
    void log(String message)
    {
        return;
        // String _message = "[";
        // _message += now() - startTime;
        // _message += "] ";
        // File_Writer(logFile, _message, true, false);
    }
    //Resets the HeatRamp Object --not otimized
    void reset()
    {
        Serial.println("Reseting HeatRamp...");
        endTime = 0;
        currentstep = 0;
        projectName = "No Name";
        logFile = "/Logs/Noname.log";
        csvFile = "/Logs/Noname.csv";
        programmedStart = false;
        startTime = 0;
        startHeating = 0;
        totalsteps = 0;
        heating = false;
        heatUp = false;
        running = false;
        finished = false;
        configured = false;
        for (size_t i = 0; i < MAX_STEPS + 1; i++)
        {
            minuteSteps[i] = 0;
            temperatureSteps[i] = 0;
            StepsTimes[i] = 0;
        }

        return;
    }
    //Flags

    String toString()
    {
        String ramp = "";
        ramp += "pname=";
        ramp += projectName;
        ramp += ",steps=";
        ramp += totalsteps;
        ramp += ",Temp=";
        for (size_t i = 0; i < totalsteps; i++)
        {
            ramp += temperatureSteps[i];
            ramp += " ";
        }
        ramp += ",Mins=";
        for (size_t i = 0; i < totalsteps; i++)
        {
            ramp += minuteSteps[i];
            ramp += " ";
        }
        ramp += ",HeatingSteps=";
        for (size_t i = 0; i < totalsteps; i++)
        {
            ramp += HeatingSteps[i];
            ramp += " ";
        }
        ramp += ",start=";
        ramp += startTime;
        ramp += ",heatingStart=";
        ramp += startHeating;
        ramp += ",end=";
        ramp += endTime;
        ramp += ",programmedStart=";
        ramp += programmedStart;
        ramp += ",configured=";
        ramp += configured;
        ramp += ",running=";
        ramp += running;
        ramp += ",finished=";
        ramp += finished;
        ramp += ",heating=";
        ramp += heating;
        ramp += ",heatUp=";
        ramp += heatUp;
        ramp += ",heatUpEveryStep=";
        ramp += heatUpEveryStep;

        ramp += ",;";

        return ramp;
    }

    void fromString(String ramp)
    {
        String value = "";
        String variable = "";
        char nextChar;
        bool isValue = false;
        reset();

        for (size_t i = 0; i < ramp.length(); i++)
        {
            nextChar = ramp[i];
            if (!isValue)
            {
                if (nextChar != '=')
                    variable += nextChar;
                else
                    isValue = true;
            }
            else
            {
                if (nextChar != ',')
                    value += nextChar;
                else
                {
                    if (variable == "pname")
                        projectName = value;
                    else if (variable == "steps")
                        totalsteps = atoi(value.c_str());
                    else if (variable == "start")
                        startTime = atoi(value.c_str());
                    else if (variable == "heatingStart")
                        startHeating = atoi(value.c_str());
                    else if (variable == "end")
                        endTime = atoi(value.c_str());
                    else if (variable == "configured")
                        configured = value == "1" ? 1 : 0;
                    else if (variable == "running")
                        running = value == "1" ? 1 : 0;
                    else if (variable == "heating")
                        heating = value == "1" ? 1 : 0;
                    else if (variable == "finished")
                        finished = value == "1" ? 1 : 0;
                    else if (variable == "heatUp")
                        heatUp = value == "1" ? 1 : 0;
                    else if (variable == "heatUpEveryStep")
                        heatUpEveryStep = value == "1" ? 1 : 0;
                    else if (variable == "programmedStart")
                        programmedStart = value == "1" ? 1 : 0;

                    else if (variable == "Temp")
                    {
                        String _temp = "";
                        int _index = 0;
                        for (size_t i = 0; i < value.length(); i++)
                        {
                            if (value[i] == ' ')
                            {
                                if (_index < MAX_STEPS)
                                    temperatureSteps[_index] = atof(_temp.c_str());
                                _index++;
                            }
                            else
                                _temp += value[i];
                        }
                    }
                    else if (variable == "Mins")
                    {
                        String _temp = "";
                        int _index = 0;
                        for (size_t i = 0; i < value.length(); i++)
                        {
                            if (value[i] == ' ')
                            {
                                if (_index < MAX_STEPS)
                                    minuteSteps[_index] = atof(_temp.c_str());
                                _index++;
                            }
                            else
                                _temp += value[i];
                        }
                    }

                    value = "";
                    variable = "";
                    isValue = false;
                }
            }
        }
    }
};

struct HtmlCache
{
    String CachedPages[MAX_CACHE_PAGES];
    String Keys[MAX_CACHE_PAGES];

    void setup()
    {
        for (size_t i = 0; i < MAX_CACHE_PAGES; i++)
        {
            CachedPages[i] = "";
            Keys[i] = "";
        }
    }

    int Contains(String key)
    {
        for (size_t i = 0; i < MAX_CACHE_PAGES; i++)
        {
            if (key == Keys[i])
                return i;
        }
        return -1;
    }

    bool LoadPage(String key, String filename)
    {
        int index = getFreeSpace();
        if (index < 0 || index > MAX_CACHE_PAGES)
            return false;
        if (!is_SD_Mount)
            return false;
        if (!SD_MMC.exists(filename))
            return false;
        Keys[index] = key;
        File _file = SD_MMC.open(filename);
        CachedPages[index] = _file.readString();
        _file.close();
        return true;
    }

    bool releasePage(String key)
    {
        int index = Contains(key);
        if (index < 0)
            return false;

        CachedPages[index] = "";
        Keys[index] = "";

        return true;
    }

    String getPagebyIndex(int index)
    {
        if (index > -1 && index < MAX_CACHE_PAGES)
            return CachedPages[index];
        else
            return "";
    }

    String getPagebyKey(String key)
    {
        return getPagebyIndex(Contains(key));
    }

private:
    int getFreeSpace()
    {
        for (size_t i = 0; i < MAX_CACHE_PAGES; i++)
        {
            if (Keys[i] == "")
                return i;
        }
        return -1;
    }
};

struct Info
{
    byte ram = 0;
    byte SD = 0;
    byte SPIFFS_Used = 0;
    float internalTemp = 0;

    void update()
    {

        float _temp = (float)(ESP.getHeapSize() - ESP.getFreeHeap()) / ESP.getHeapSize() * 100;
        ram = round(_temp);
        if (is_SD_Mount)
        {

            _temp = 0;
            _temp = (float)SD_MMC.usedBytes() / SD_MMC.totalBytes() * 100;

            SD = round(_temp);
        }

        if (is_SPIFFSS_Mount)
        {

            _temp = 0;
            _temp = (float)SPIFFS.usedBytes() / SPIFFS.totalBytes() * 100;

            SPIFFS_Used = round(_temp);
        }

        //Implement DS18 internal Temp
        internalTemp = random(internalTemp - 5, internalTemp + 5); //delete after example
    }

    String toString(bool SendMax = false)
    {
        update();
        String Message = "";
        Message += "SD=";
        Message += SD;
        Message += ",SPIFFS=";
        Message += SPIFFS_Used;
        Message += ",ram=";
        Message += ram;
        if (SendMax)
        {
            Message += ",SDMax=";
            Message += double(SD_MMC.totalBytes());
            Message += ",ramMax=";
            Message += ESP.getHeapSize();
            Message += ",SPIFFSMax=";
            Message += SPIFFS.totalBytes();
        }
        Message += ",internalTemp=";
        Message += internalTemp;
        Message += ",;";
        return Message;
    }

    void print(bool DisplayMax = false)
    {
        update();
        Serial.println("System Info:");
        Serial.print("Ram usage: ");
        Serial.print(ram);
        Serial.print("%");
        if (DisplayMax)
        {
            Serial.print(" of ");
            Serial.print((double)ESP.getHeapSize() / 1024);
            Serial.print("Kb");
        }
        Serial.println();
        Serial.print("SD Usage: ");
        Serial.print(SD);
        Serial.print("%");
        if (DisplayMax)
        {
            Serial.print(" of ");
            Serial.print((double)SD_MMC.totalBytes() / 1024 / 1024);
            Serial.print("Mb.");
        }
        Serial.println();
        Serial.print("Temperature around the ESP: ");
        Serial.print(internalTemp);
        Serial.println(" C.");

        return;
    }
};

Info SysInfo;
HtmlCache Cache;

HeatRamp currentRamp;
HeatRamp lastRamp;

DebugFlags debug;

// Setup a oneWire instance to communicate with any OneWire devices;
OneWire *oneWire;
DallasTemperature *DS18;

String StateString()
{
    String msg = "";
    if (currentRamp.configured)
    {
        msg += "heat_ramp:";
        msg += currentRamp.toString();
    }
    msg += ";new_prog:";
    msg += currentProg;
    msg += ";current_state: ";
    msg += currentRamp.running;
    msg += ";new_step:";
    msg += currentRamp.currentstep + 1;
    msg += ";relay_status:";
    msg += digitalRead(Config.RELAY_PIN);
    msg += ";current_process:";
    if (currentRamp.running)
        msg += "Working";
    else if (currentRamp.heating)
        msg += "Heating";
    else if (currentRamp.configured && !currentRamp.finished)
        msg += "Waiting to Start";
    else
        msg += " Idle";
    msg += ';';

    return msg;
}

String getMIME(String filename)
{
    if (filename.endsWith(".htm"))
        return "text/html";
    else if (filename.endsWith(".html"))
        return "text/html";
    else if (filename.endsWith(".css"))
        return "text/css";
    else if (filename.endsWith(".js"))
        return "text/javascript";
    else if (filename.endsWith(".png"))
        return "image/png";
    else if (filename.endsWith(".gif"))
        return "image/gif";
    else if (filename.endsWith(".jpg"))
        return "image/jpeg";
    else if (filename.endsWith(".ico"))
        return "image/x-icon";
    else if (filename.endsWith(".xml"))
        return "text/xml";
    else if (filename.endsWith(".pdf"))
        return "application/x-pdf";
    else if (filename.endsWith(".zip"))
        return "application/x-zip";
    else if (filename.endsWith(".gz"))
        return "application/x-gzip";
    else if (filename.endsWith(".csv"))
        return "text/csv";
    else if (filename.endsWith(".log"))
        return "text/log";
    return "text/plain";
}

String normalizedDayMonth()
{

    if (!is_time_configured)
        return "";
    String msg = "";
    if (day() < 10)
        msg += '0';
    msg += day();
    msg += '-';
    if (month() < 10)
        msg += '0';
    msg += month();
    return msg;
}

String GetSdFileSafe(String filename)
{
    if (!is_SD_Mount)
        return "";
    String file;
    while (!xSemaphoreTake(sd_mutex, 50))
    {
        esp_task_wdt_reset();
    }
    File _file = SD_MMC.open(filename);

    file = _file.readString();
    _file.close();

    xSemaphoreGive(sd_mutex);
    return file;
}

String GetPage()
{
    return
        //##$$test.html
R"===(<head>
    <script>
  function FileUpload(file) {
  const reader = new FileReader();
  const xhr = new XMLHttpRequest();
  this.xhr = xhr;

  const self = this;
  this.xhr.upload.addEventListener("progress", function(e) {
        if (e.lengthComputable) {
          const percentage = Math.round((e.loaded * 100) / e.total);
          console.log(percentage);
        }
      }, false);

  xhr.upload.addEventListener("load", function(e){

      }, false);
  xhr.open("POST", "/dodo");
  xhr.overrideMimeType('text/plain; charset=x-user-defined-binary');
  reader.onload = function(evt) {
    xhr.send(evt.target.result);
  };
  reader.readAsBinaryString(file);
}
    </script>
    </head>


<form action="/dodo" method="POST" enctype="multipart/form-data">
    <input name="fupdate" id="fone" type="file" />
    <button type="submit" > go </button>
</form>)==="//##$$
        ;
}

String GetPage2()
{
    return
        //##$$Settings.js
R"===(RequestData('/loadconfig');
RequestData('/reqwifisearch',10000);
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
    //Resolution of DS18b20 Sensor 
    this.TEMPERATURE_RESOLUTION = 9;

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


    //Enables Cacheing; 
    this.enableCache = true;
    this.continousLog = true;

    //Backup Options 
    this.enable_backup = false;
    this.backup_freq = 0;
    this.lastbackup = 0;

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

    this.send = function (complete) {

            var message = "";
            message += "tolerance=";
            message += this.tolerance;
            message += ",calibration=";
            message += this.calibration;
            message += ",TEMPERATURE_RESOLUTION=";
            message += this.TEMPERATURE_RESOLUTION;
            message += ",sample_freq=";
            message += this.sample_freq;
            message += ",WIFI_SSID=";
            message += this.WIFI_SSID;
            if (complete)
            {
                message += ",WIFI_PASSWORD=";
                message += this.WIFI_PASSWORD;
            }
            message += ",www_username=";
            message += this.www_username;
            if (complete)
            {
                message += ",www_password=";
                message += this.www_password;
            }
            message += ",WIFI_AP_SSID=";
            message += this.WIFI_AP_SSID;
            if (complete)
            {
                message += ",WIFI_AP_PASSWORD=";
                message += this.WIFI_AP_PASSWORD;
            }
            message += ",use_static_ip=";
            message += this.use_static_ip;
            message += ",static_ip=";
            message += this.static_ip;
            message += ",enable_upload=";
            message += this.enable_upload;
            message += ",UploadServiceType=";
            message += this.UploadServiceType;
            message += ",UploadLogin=";
            message += this.UploadLogin;
            if (complete)
            {
                message += ",UploadPassword=";
                message += this.UploadPassword;
            }
            message += ",enableCache=";
            message += this.enableCache;
            message += ",continousLog=";
            message += this.continousLog;
            message += ",enable_backup=";
            message += this.enable_backup;
            message += ",backup_freq=";
            message += this.backup_freq;
            message += ",lastbackup=";
            message += this.lastbackup;
            message += ",RELAY_PIN=";
            message += this.RELAY_PIN;
            message += ",ONEWIRE_BUS=";
            message += this.ONEWIRE_BUS;
            message += ",host_name=";
            message += this.host_name;
            message += ",temp_sensor_type=";
            message += this.temp_sensor_type;
            message += ",NTC_BASE_RESISTANCE=";
            message += this.NTC_BASE_RESISTANCE;
            message += ",NTC_BASE_TEMP=";
            message += this.NTC_BASE_TEMP;
            message += ",NTC_READ_PIN=";
            message += this.NTC_READ_PIN;
            message += ",NTC_ENABLE_PIN=";
            message += this.NTC_ENABLE_PIN;
            message += ",NTC_BETA=";
            message += this.NTC_BETA;
            message += ",;";
    
            return message;
        };
}

function RequestData(whichdata, timeout) {
    if (typeof whichdata !== 'string') {
        httpGetAsync("requpdate", ParseNewData, timeout);
    }
    else
        httpGetAsync(whichdata, ParseNewData, timeout);
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
            if (_args[0] == 'wifiAP') {
                var aps = _args[1].split(',');
                wifiAPs = [, ,];
                for (var i = 0; i < aps.length -1; aps++) {
                    if (aps[i] !== '') {
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

function httpGetAsync(theUrl, callback, timeout) {
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
    if (typeof (timeout) === 'number')
        xmlHttp.timeout = timeout
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
    generateCircle('cal_raw_sensor', 'Raw:', 0, 'raw_temp', '#d4ebd3', 'red', 'red', '&deg;C', 'map 20 80 0 100');
    generateCircle('cal_sensor', 'Calib:', 0, 'cal_temp', '#d4ebd3', 'darkred', 'darkred', '&deg;C', 'map 20 80 0 100');
    setLabelSize('cal_temp', '45px');
    setLabelSize('raw_temp', '45px');
    setnew('raw_temp', '0');
    setnewangle('raw_temp', 120);
    setnew('cal_temp', '0');
    setnewangle('cal_temp', 120);
    setnewangle('ram', 120);
    setnewangle('internalTemp', 120);
    setnewangle('sdcard', 120);
    setnewangle('SPIFFS', 120);

    ParseNewData(test);
    draw('all');

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

    console.log(sender.id, sender.value)

    if (type == 'common') {
        config[variable] = sender.value;
        draw(area);
        return;
    }
    if (type == 'suppressdraw') {
        config[variable] = sender.value;
        return;
    }



    if (type === 'slider') {
        config[variable] = sender.checked;
        draw(area);
        return;
    }
}

function draw(params) {

    if (params == 'all') {
        draw('wifi');
        draw('general');
        draw('files');
        draw('backup');
        draw('sensors');
        draw('upload');
    }
    if (params == 'wifi') {
        $('wifi_ssid').value = config.WIFI_SSID;//custom ssid
        $('WIFI_PASSWORD').value = config.WIFI_PASSWORD;
        $('WIFI_AP_SSID').value = config.WIFI_AP_SSID;
        $('WIFI_AP_PASSWORD').value = config.WIFI_AP_PASSWORD;
        $('ip_address').value = config.static_ip;
        $('use_static_ip').checked = config.use_static_ip;

        enable('ip_address', config.use_static_ip);
        togglediv('wifi_ssid', $('wifi_custom_ssid').checked);
        //enable()

    }

    if (params == 'general') {
        $('host_name').value = config.host_name;
        $('sample_freq').value = config.sample_freq;
        $('enableCache').checked = config.enableCache;
        $('continousLog').checked = config.continousLog;
        $('RELAY_PIN').value = config.RELAY_PIN;

    }
    if (params == 'backup') {
        $('enable_backup').checked = config.enable_backup;
        $('backup_freq').value = config.backup_freq;
        if (config.lastbackup > 0)
            $('lastbackup').innerText = new Date(config.lastbackup * 1000).toDateString();
        else
            $('lastbackup').innerText = 'Never';
    }
    if (params == 'upload') {
        $('enable_upload').checked = config.enable_upload;
        $('UploadServiceType').value = config.UploadServiceType;
        $('UploadLogin').value = config.UploadLogin;
        $('UploadPassword').value = config.UploadPassword;
        togglediv('upload_service', config.UploadServiceType);
        //add auto upload
    }
    if (params == 'sensors') {
        $('NTC_READ_PIN').value = config.NTC_READ_PIN;
        $('NTC_ENABLE_PIN').value = config.NTC_ENABLE_PIN;
        $('NTC_BASE_RESISTANCE').value = config.NTC_BASE_RESISTANCE;
        $('NTC_BASE_TEMP').value = config.NTC_BASE_TEMP;
        $('NTC_BETA').value = config.NTC_BETA;
        $('ONEWIRE_BUS').value = config.ONEWIRE_BUS;
        $('ADC_REQ_CORRECT').checked = config.ADC_REQ_CORRECT;
        $('ADC_CORRECTION').value = config.ADC_CORRECTION;
        $('ONEWIRE_BUS').value = config.ONEWIRE_BUS;
        $('TEMPERATURE_RESOLUTION').value = config.TEMPERATURE_RESOLUTION;
        $('calibration').value = config.calibration;
        $('tolerance').value = config.tolerance;
        $('temp_sensor_type').value = config.temp_sensor_type;
        togglediv('temp_sensor', config.temp_sensor_type)
    }
}

function togglediv(which, value) {
    if (which === 'wifi_ssid') {
        if (value) {
            $('wifi_ssid_list_div').style.display = 'none';
            $('wifi_custom_ssid_div').style.display = 'flex'
        }
        else {
            $('wifi_ssid_list_div').style.display = 'flex';
            $('wifi_custom_ssid_div').style.display = 'none'
        }
    }
    if (which === 'change_password') {
        if (value) {
            $('nonewpassdiv').style.display = 'none';
            $('newpassdiv').style.display = 'block';
            $('confirmpassword').value = "";
            $('newpassword').value = "";
            $('oldpassword').value = "";
        }
        else {
            $('nonewpassdiv').style.display = 'block';
            $('newpassdiv').style.display = 'none';
        }
    }
    if (which === 'temp_sensor') {
        if (value == 1 || value == 2) {
            $('ds18-div').style.display = 'none';
            $('ntc-div').style.display = 'block';

        }
        else {
            $('ds18-div').style.display = 'block';
            $('ntc-div').style.display = 'none';
        }
    }
    if (which === 'cal_div')
    {
        if (value) {
            $('nocal_div').style.display = 'none';
            $('cal_div').style.display = 'block';
        }
        else {
            $('nocal_div').style.display = 'block';
            $('cal_div').style.display = 'none';
        }
    }
    if (which === 'upload_service')
    {
        if (value == 0) //Gmail
        {
            $('nocal_div').style.display = 'none';
            $('cal_div').style.display = 'block';
        }
        else if (value == 1)//Google Drive
        {
            $('nocal_div').style.display = 'block';
            $('cal_div').style.display = 'none';
        }
        else if (value == 2)//IFFT
        {
            $('nocal_div').style.display = 'block';
            $('cal_div').style.display = 'none';
        }
    }
}
function wifi_connect() {

}

function $(name) {
    return document.getElementById(name);
}

function send() {
    //not safe probably.
    if ($('confirmpassword').value !== $('newpassword').value) {
        alert('Passwords don\'t match.');
        return;
    }
    var req = '/newpass?n=' + $('newpassword').value + '&o=' + $('oldpassword').value;
    RequestData(req);
}

function saveconfig()
{
    var url = '/newconfig?config=';
    url += config.send();
    RequestData(url);

})==="//##$$
        ;
}

void GeneralLog(String message)
{
    String _msg = "[";
    if (is_time_configured)
        _msg += now();
    else
    {
        _msg += "clock not synced:";
        _msg += millis();
    }
    _msg += "]>>>>>";
    _msg += message;
    _msg += "\n";
    File_Writer("/Logs/General.log", _msg, true, false);
}

void File_Writer(String Filename, String Content, bool append = false, bool useSPIFFS = false)
{
    if (!is_SD_Mount && !useSPIFFS)
    {
        Serial.println("SD not Mount");
        return;
    }
    if (!is_SPIFFSS_Mount && useSPIFFS)
    {
        Serial.println("SPIFFS not Mount");
        return;
    }

    if (Filename[0] != '/')
        Filename = '/' + Filename;

    File _file;
    if (!useSPIFFS)
    {
        if (append)
            _file = SD_MMC.open(Filename, "a");
        else
        {
            if (SD_MMC.exists(Filename))
                SD_MMC.remove(Filename);
            _file = SD_MMC.open(Filename, "w");
        }
    }
    else
    {
        if (append)
            _file = SPIFFS.open(Filename, "a");
        else
        {
            if (SPIFFS.exists(Filename))
                SPIFFS.remove(Filename);
            _file = SPIFFS.open(Filename, "w");
        }
    }
    _file.print(Content);

    _file.flush();
    _file.close();
}

void resetFlags() // Reset the flags of new data to be sent to HTTP front end
{
    new_ramp = false;
    new_step = false;
    new_prog = false;
    new_relay_state = false;
}

#pragma region Ramp Start - End

void StartRamp()
{
    String msg = "Starting new Ramp: ";
    msg += currentRamp.projectName;
    msg += "\nNow: ";
    msg += now();
    msg += "\nTimes: [";
    for (size_t i = 0; i < currentRamp.totalsteps; i++)
    {
        msg += currentRamp.minuteSteps[i];
        msg += ',';
    }
    msg += "]\nTemps: [";
    for (size_t i = 0; i < currentRamp.totalsteps; i++)
    {
        msg += currentRamp.temperatureSteps[i];
        msg += ',';
    }
    msg += "]\nSteps: ";
    msg += currentRamp.totalsteps;
    msg += "\n";
    currentRamp.log(msg);
    msg = "Starting Ramp: ";
    msg += currentRamp.projectName;
    GeneralLog(msg);
    currentRamp.running = true;
    currentRamp.finished = false;
    currentRamp.currentstep = 0;
    currentRamp.startTime = now() + 1;
    currentRamp.calculateStepTimes();
    currentRamp.csv(DS18Temp[DS18_CurrentIndex], digitalRead(Config.RELAY_PIN), "heat ramp started");
    new_ramp = true;
    Serial.println("======= Starting Heat Ramp =======");
    Serial.print("Name: ");
    Serial.println(currentRamp.projectName);
    oldMillis = millis();
}

void startHeating()
{
    Serial.println("Start Heating up to: ");
    Serial.print(currentRamp.temperatureSteps[0]);
    currentRamp.heating = true;
    currentRamp.startHeating = now();
    currentRamp.csv(DS18Temp[DS18_CurrentIndex], digitalRead(Config.RELAY_PIN), "Heating Started");
}

void resetRamp()
{
    lastRamp.fromString(currentRamp.toString());
    currentRamp.reset();
    currentProg = 0;
}

void EndRamp()
{
    currentRamp.running = false;
    currentRamp.heating = false;
    currentRamp.finished = true;
    currentRamp.endTime = now();
    GeneralLog("Ending Ramp: " + currentRamp.projectName);
    Serial.println("======= Ending Heat Ramp =======");
}

#pragma endregion

String listLogFiles()
{
    if (!is_SD_Mount)
        return "error:SD card not found;";

    String msg = "files:";

    File root = SD_MMC.open("/Logs");
    File file = root.openNextFile();

    while (file)
    {
        String s = file.name();
        msg += s;
        msg += '=';
        msg += (float)file.size() / 1024;
        msg += ',';
        // msg += file.getLastWrite();
        // msg += ',';
        esp_task_wdt_reset();
        file = root.openNextFile();
    }

    msg += ';';
    return msg;
}

void handleRequestHist(AsyncWebServerRequest *request)
{

    if (!is_time_configured)
    {
        request->send(200, "txt/plain", "set_time;");
        return;
    }
    bool done = false;
    String msg = "temp_history:";
    for (int i = 0; !done; i++)
    {
        if (DS18Temp[i] == 0 && DS18Time[i] == 0)
            done = true;
        else if (i + 1 >= HISTORY_SIZE)
            done = true;
        else
        {
            msg += DS18Time[i];
            msg += " ";
            msg += DS18Temp[i];
            msg += ",";
        }
    }
    msg += ';';
    msg += "new_temp:";
    int x = DS18_CurrentIndex - 1;
    if (x < 0)
        x = HISTORY_SIZE - 1;
    msg += DS18Time[x];
    msg += " ";
    msg += DS18Temp[x];
    msg += ";";

    msg += StateString();

    AsyncWebServerResponse *res = request->beginResponse(200, "text/plain", msg);
    request->send(res);
    return;
}

void handleRequestUpdate(AsyncWebServerRequest *request)
{
    String message = "new_temp:";
    int x = DS18_CurrentIndex - 1;
    if (x < 0)
        x = HISTORY_SIZE - 1;
    message += DS18Time[x];
    message += " ";
    message += DS18Temp[x];
    message += ";";

    if (!is_time_configured)
    {
        message += "set_time;";
    }
    else
    {
        message += StateString();
    }

    request->send(200, "text/plain", message);
}

void handleNewRamp(AsyncWebServerRequest *request)
{
    if (!currentRamp.running)
    {
        if (currentRamp.configured)
        {
            lastRamp.fromString(currentRamp.toString());
            currentRamp.reset();
        }
        currentRamp.totalsteps = atoi(request->arg((size_t)0).c_str());
        int j = 0;
        for (size_t i = 1; i < request->args(); i++)
        {
            if (i + 1 < request->args())
            {
                if (request->arg(i) != "" && request->arg(i + 1) != "")
                {
                    currentRamp.temperatureSteps[j] = atof(request->arg(i).c_str());
                    currentRamp.minuteSteps[j] = atof(request->arg(i + 1).c_str());
                    j++;
                }
                else
                    currentRamp.totalsteps--;
            }
            i++;
        }
        currentRamp.currentstep = 0;
        String start = request->arg("_start");
        if (request->arg("heatup") == "on")
            currentRamp.heatUp = true;
        currentRamp.projectName = request->arg("pname");
        currentRamp.startLogs();
        currentRamp.print();
        currentRamp.configured = true;
        if (start == "on")
        {
            if (currentRamp.heatUp)
                startHeating();
            else
                StartRamp();
        }
    }
    request->send(200, "text/html", "<html><script>window.location.assign(\"/\")</script></html>");
}

void SendLargeFiles(AsyncWebServerRequest *request, String _filename)
{
    if (debug.web)
    {
        Serial.print("Sending Large File: ");
        Serial.println(_filename);
    }

    while (xSemaphoreTake(sd_mutex, (TickType_t)50) != 1)
    {
        esp_task_wdt_reset();
    }

    AsyncWebServerResponse *response = request->beginResponse(SD_MMC, _filename, getMIME(_filename));
    request->send(response);
    xSemaphoreGive(sd_mutex);
    /*  */
    return;
}
void getTime()
{
    Serial.println("Syncing Time Online");
    HTTPClient http;
    http.begin("http://worldtimeapi.org/api/timezone/America/Bahia.txt"); //HTTP
    int httpCode = http.GET();
    // httpCode will be negative on error
    if (httpCode > 0)
    {
        // HTTP header has been send and Server response header has been handled
        // file found at server
        if (httpCode == HTTP_CODE_OK)
        {
            Serial.printf("[HTTP] OK... code: %d\n", httpCode);
            String payload = http.getString();
            char str[payload.length() + 1];
            strcpy(str, payload.c_str());
            char *pch;
            pch = strtok(str, ":\n");
            int i = 0;
            //  int raw_offset = 0;
            while (pch != NULL)
            {

                i++;
                if (i == 23)
                {
                    //    raw_offset = atoi(pch);
                }
                if (i == 27)
                {
                    setTime(atoi(pch));
                }
                //printf("%d: %s\n", i, pch);
                pch = strtok(NULL, ":\n");
            }
            is_time_configured = true;
            startup_time = now();
            String msg = "Time Synced ";
            msg += millis();
            msg += "ms from boot.";
            GeneralLog(msg);
            StandardTesting();
        }
        else
        {
            Serial.printf("[HTTP] Error code: %d\n", httpCode);
        }
    }
    else
    {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
}

float readTemperature(bool raw = false)
{
    if (Config.temp_sensor_type >= 3)
        return 0;
    float _temp;

    if (Config.temp_sensor_type == 0 || Config.temp_sensor_type == 0) //DS18b20
    {
        DS18->requestTemperatures();
        _temp = DS18->getTempCByIndex(0);
    }
    else if (Config.temp_sensor_type == 2) //NTC thermopar
    {
        float average = 0;

        //These 2 will be #defines
        int NTC_SAMPLES = 5000;
        int NTC_SERIES_RESISTOR = 1000;

        if (Config.NTC_ENABLE_PIN >= 0)
            digitalWrite(Config.NTC_ENABLE_PIN, 1);

        for (uint16_t i = 0; i < NTC_SAMPLES; i++)
        {
            average += analogRead(Config.NTC_READ_PIN);
            delayMicroseconds(30);
        }
        if (Config.NTC_ENABLE_PIN >= 0)
            digitalWrite(Config.NTC_ENABLE_PIN, 0);

        average = average / NTC_SAMPLES;

        // convert the value to resistance
        average = 4095 / average - 1;
        average = NTC_SERIES_RESISTOR / average;

        float steinhart;
        steinhart = (float)average / Config.NTC_BASE_RESISTANCE;   // (R/Ro)
        steinhart = (float)log(steinhart);                         // ln(R/Ro)
        steinhart = (float)steinhart / Config.NTC_BETA;            // 1/Beta * ln(R/Ro)
        steinhart += (float)1.0 / (Config.NTC_BASE_TEMP + 273.15); // + (1/To)
        steinhart = (float)1.0 / steinhart;                        // Invert
        steinhart -= (float)273.15;                                // convert absolute temp to C

        _temp = steinhart;
    }

    bool sim = true;

    if (sim)
    {
        _temp = (float)random_temp / 10;
    }
    if (!raw)
    {
        _temp = (float)_temp + Config.calibration;
    }

    return (float)_temp;
}

//most important
void updateTemperatures()
{
    float _temp = readTemperature();

    //ReadError

    if (now() >= LastTime + Config.sample_freq)
    {

        if (currentRamp.running || currentRamp.heating)
        {
            if (digitalRead(Config.RELAY_PIN))
                random_temp = random(random_temp, random_temp + 50);
            else
                random_temp = random(random_temp - 50, random_temp);
        }
        else
        {
            random_temp = random(random_temp - 50, random_temp + 50);
        }

        /*String file = "";
        file += "(";
        file += LineCounter;
        file += ") - ";
        file += now();
        file += " - ";
        file += _temp;
        file += "\n";
        File_Writer("/Logs/test.log", file, true);
        LineCounter++;*/

        resetFlags();
        if (is_time_configured)
        {

            if (DS18_CurrentIndex >= HISTORY_SIZE)
            {
                cycle++;
                DS18_CurrentIndex = 0;
            }
            DS18Temp[DS18_CurrentIndex] = _temp;
            DS18Time[DS18_CurrentIndex] = now();
            if (debug.general)
            {
                Serial.print("time:");
                Serial.print(is_time_configured);
                Serial.print(".cycle:");
                Serial.print(cycle);
                Serial.print(".index:");
                Serial.print(DS18_CurrentIndex);
                Serial.print(".Temp:");
                Serial.print(DS18Temp[DS18_CurrentIndex]);
                Serial.print(".Time:");
                Serial.print(DS18Time[DS18_CurrentIndex]);
                Serial.print(".Time from boot:");
                Serial.println(now() - startup_time);
            }
            DS18_CurrentIndex++;
        }
        LastTime = now();
        if (debug.temp)
        {
            Serial.print("current temp: ");
            Serial.print(_temp);
            Serial.println();
        }

        if (currentRamp.running || currentRamp.heating)
        {
            currentRamp.csv(_temp, digitalRead(Config.RELAY_PIN));
        }
    }

    //TODO GREENHOUSE CONTROL
    if (currentRamp.running)
    {
        if (now() >= currentRamp.StepsTimes[currentRamp.currentstep])
        {
            int timeup = millis() - oldMillis;
            String msg = "[";
            msg += timeup;
            msg += " ms.] ";

            currentRamp.currentstep++;
            msg += ">>> step: ";
            if (currentRamp.currentstep >= currentRamp.totalsteps)
                msg += "-";
            else
                msg += currentRamp.currentstep;
            msg += ". Time: ";
            msg += now();
            msg += ". next step: ";
            if (currentRamp.currentstep + 1 >= currentRamp.totalsteps)
                msg += "-";
            else
                msg += currentRamp.StepsTimes[currentRamp.currentstep];
            msg += "\n";
            currentRamp.log(msg);
            Serial.print(msg);
            oldMillis = millis();
            if (currentRamp.currentstep >= currentRamp.totalsteps)
            {
                currentRamp.currentstep--;
                String msg2 = "Ramp Finished. now: ";
                msg2 += now();
                msg2 += ". Step: ";
                msg2 += currentRamp.currentstep;
                msg2 += ".";
                currentRamp.log(msg);
                new_prog = true;
                currentProg = 100;
                currentRamp.csv(_temp, digitalRead(Config.RELAY_PIN), "heat ramp finished");
                EndRamp();
            }
            else
            {
                String comment = "new step:";
                comment += currentRamp.currentstep + 1;
                currentRamp.csv(_temp, digitalRead(Config.RELAY_PIN), comment);
                new_step = true;
            }
        }
        bool oldRelayState = digitalRead(Config.RELAY_PIN);

        //Serial.print(">>>>>>");Serial.println(" _temp < currentRamp.temperatureSteps[currentRamp.currentstep] && oldRelayState");
        //High Temp
        /*
        Serial.print("old: ");
        Serial.print(oldRelayState);
        Serial.print("-t: ");
        Serial.print(_temp);
        Serial.print("-st: ");
        Serial.print( currentRamp.temperatureSteps[currentRamp.currentstep]);
         Serial.print("-ev: ");
        Serial.println(_temp < currentRamp.temperatureSteps[currentRamp.currentstep]);
*/
        if (_temp > currentRamp.temperatureSteps[currentRamp.currentstep] + Config.tolerance && oldRelayState)
        {
            digitalWrite(Config.RELAY_PIN, 0);
            new_relay_state = true;
        }
        //Low Temp
        else if (_temp < currentRamp.temperatureSteps[currentRamp.currentstep] - Config.tolerance && !oldRelayState)
        {
            digitalWrite(Config.RELAY_PIN, 1);
            new_relay_state = true;
        }

        int raw_current = now() - currentRamp.startTime;
        int raw_end = currentRamp.StepsTimes[currentRamp.totalsteps - 1] - currentRamp.startTime;
        byte new_percent = map(raw_current, 0, raw_end, 0, 100);
        //byte new_percent = map(now() - currentRamp.startTime,0,computedEnd - currentRamp.startTime,0,100);

        if (new_percent != currentProg)
        {
            // Serial.print("percent: ");
            // Serial.print(new_percent);
            if (debug.ramp)
            {
                Serial.print("percent: ");
                Serial.print(new_percent);
                Serial.print("%. curr: ");
                Serial.print(raw_current);
                Serial.print(" secs. end: ");
                Serial.print(raw_end);
                Serial.println();
            }
            new_prog = true;
            currentProg = new_percent;
        }
    }

    else if (currentRamp.heating)
    {
        bool oldRelayState = digitalRead(Config.RELAY_PIN);
        if (_temp > currentRamp.temperatureSteps[0] + Config.tolerance && oldRelayState)
        {
            digitalWrite(Config.RELAY_PIN, 0);
            new_relay_state = true;
        }
        if (_temp < currentRamp.temperatureSteps[0] - Config.tolerance && !oldRelayState)
        {
            digitalWrite(Config.RELAY_PIN, 1);
            new_relay_state = true;
        }

        if (_temp >= currentRamp.temperatureSteps[0] - Config.tolerance && _temp <= currentRamp.temperatureSteps[0] + Config.tolerance)
        {
            currentRamp.log("Heating done. Starting ramp.");
            StartRamp();
            currentRamp.csv(_temp, digitalRead(Config.RELAY_PIN), "heating finished");
            currentRamp.heating = false;
        }
    }
    else if (now() > currentRamp.endTime + (HISTORY_SIZE * Config.sample_freq) && currentRamp.finished)
        resetRamp();
}

void handleStart(AsyncWebServerRequest *request)
{

    if (!currentRamp.running)
        StartRamp();

    String msg = "ramp_start:";
    msg += currentRamp.startTime;
    msg += ';';
    msg += "current_state:";
    msg += "true;";
    request->send(200, "text/plain", msg);
}

void handleStop(AsyncWebServerRequest *request)
{
    if (currentRamp.running)
        EndRamp();
    String msg = "";
    request->send(200, "text/plain", msg);
}

void handleReqState(AsyncWebServerRequest *request)
{
    String msg = StateString();
    // Serial.println(msg);
    request->send(200, "text/plain", msg);
}

void handleSetTime(AsyncWebServerRequest *request)
{

    //prevents override --dont trust clients if u can trust time api
    // if(is_time_configured)
    // return;

    if (request->hasArg("time"))
    {
        Serial.print("time recieved from client: ");
        Serial.println(atoi(request->arg((size_t)0).c_str()));
        setTime(atoi(request->arg((size_t)0).c_str()));
        is_time_configured = true;
        startup_time = now() - millis() / 1000;
        String msg = "Time Synced ";
        msg += millis();
        msg += "ms from boot.";
        GeneralLog(msg);
        StandardTesting();
    }

    request->send(200, "text/plain", "set_time-sucess");
    return;
}

void handleWebServer(AsyncWebServerRequest *request)
{

    // if (!request->authenticate(Config.www_username.c_str(), Config.www_password.c_str()))
    // {
    //     request->requestAuthentication();
    //     return;
    // }

    ///Check if file exists
    int a = millis();
    String filename = "/Web";
    String apiname = request->url();

    if (apiname.endsWith("/"))
        apiname += "MainPage.html";
    filename += apiname;

    if (debug.web)
    {
        Serial.print("filename: ");
        Serial.print(filename);
        Serial.print("  -----  apiname: ");
        Serial.println(apiname);
    }

    if (apiname == "/reqespupdate")
    {
        if (!is_updating)
        {
            request->send(200, "text/plain", "error:OTA Failed;");
        }
        else
        {
            String msg = "update_progress:";
            msg += update_progress;
            msg += ';';
            request->send(200, "text/plain", msg);
        }

        if (debug.web)
        {
            Serial.print("Service Time: ");
            Serial.print(millis() - a);
            Serial.println("ms.");
        }
        return;
    }
    if (is_updating)
    {
        if (apiname != "/Updating.html" && apiname != "/Common.css")
            request->send(200, "text/plain", "go:/Updating.html;");
        else
        {
            if (is_SD_Mount)
                request->send(200, "text/html", GetSdFileSafe(filename));
        }

        if (debug.web)
        {
            Serial.print("Service Time: ");
            Serial.print(millis() - a);
            Serial.println("ms.");
        }
        return;
    }

    if (Config.enableCache)
    {
        int cache = Cache.Contains(apiname);
        if (debug.web)
        {
            Serial.print("cache location: ");
            Serial.println(cache);
        }
        if (cache >= 0)
        {
            AsyncWebServerResponse *res = request->beginResponse(200, getMIME(apiname), Cache.getPagebyIndex(cache));
            request->send(res);
            if (debug.web)
            {
                Serial.print("Service Time: ");
                Serial.print(millis() - a);
                Serial.println("ms.");
            }
            return;
        }
    }

    if (apiname == "/requpdate")
    {
        handleRequestUpdate(request);
        if (debug.web)
        {
            Serial.print("Service Time: ");
            Serial.print(millis() - a);
            Serial.println("ms.");
        }
        return;
    }
    else if (apiname == "/reqstate")
    {
        request->send(200, "text/plain", StateString());
        if (debug.web)
        {
            Serial.print("Service Time: ");
            Serial.print(millis() - a);
            Serial.println("ms.");
        }
        return;
    }
    else if (apiname == "/reqhist")
    {
        handleRequestHist(request);
        return;
    }
    else if (apiname == "/do")
    {
        handleNewRamp(request);
        request->send(200, "text/html", "<html><script>window.location.assign(\"/\")</script></html>");
    }
    else if (apiname == "/dodo")
    {
        String message = "File Not Found\n\n";
        message += "URI: ";
        message += request->url();
        message += "\nMethod: ";
        message += request->method();
        message += "\nArguments: ";
        message += request->args();
        message += "\n";
        int params = request->params();

        for (uint8_t i = 0; i < params; i++)
        {
            AsyncWebParameter *p = request->getParam(i);
            if (p->isFile())
            { //p->isPost() is also true
                message += "FILE[";
                message += p->name();
                message += "]: ";
                message += p->value();
                message += ", size: ";
                message += p->size();
                message += '\n';
            }
            else if (p->isPost())
            {
                message += "POST[";
                message += p->name();
                message += "]: ";
                message += p->value();
                message += ", size: ";
                message += p->size();
                message += '\n';
            }
            else
            {
                message += "GET[";
                message += p->name();
                message += "]: ";
                message += p->value();
                message += ", size: ";
                message += p->size();
                message += '\n';

            }
            message += " " + request->argName(i) + ": " + request->arg(i) + "\n";
        }
        request->send(200, "text/html", message);
    }
    else if (apiname == "/settime")
    {
        handleSetTime(request);
        if (debug.web)
        {
            Serial.print("Service Time: ");
            Serial.print(millis() - a);
            Serial.println("ms.");
        }
        return;
    }
    else if (apiname == "/deleteAllFiles")
    {
        clearDir("/Logs");
        AsyncWebServerResponse *res = request->beginResponse(200, "text/plain", listLogFiles());
        request->send(res);
        if (debug.web)
        {
            Serial.print("Service Time: ");
            Serial.print(millis() - a);
            Serial.println("ms.");
        }
        return;
    }
    else if (apiname == "/csv")
    {
        String filename = "/Logs/";
        filename += request->arg("fileName");
        if (SD_MMC.exists(filename))
        {
            SendLargeFiles(request, filename);
        }
        else
            request->send(200, "text/plain", "error:filenotfound;");
        if (debug.web)
        {
            Serial.print("Service Time: ");
            Serial.print(millis() - a);
            Serial.println("ms.");
        }
        return;
    }
    else if (apiname == "/fileslist")
    {
        String msg = listLogFiles();
        AsyncWebServerResponse *res = request->beginResponse(200, "text/plain", msg);
        request->send(res);

        if (debug.web)
        {
            Serial.print("Service Time: ");
            Serial.print(millis() - a);
            Serial.println("ms.");
        }
        return;
    }
    else if (apiname == "/deleteFile")
    {
        String filename = "/Logs/";
        filename += request->arg("fileName");
        String Message = "";
        if (SD_MMC.exists(filename))
        {
            if (filename != "/Logs/General.log")
            {
                SD_MMC.remove(filename);
                String logMsg = "File deleted: ";
                logMsg += filename;
                logMsg += ".";
                GeneralLog(logMsg);
                Message += "delete_sucessful:";
                Message += filename;
                Message += ";";
            }
            else
            {
                Message += "error:Acess Denied.\nYou don't have permission to delete the General Log;";
            }
        }
        else
            Message += "error:File was not Found;";
        Message += listLogFiles();
        AsyncWebServerResponse *res = request->beginResponse(200, "text/plain", Message);
        request->send(res);
        if (debug.web)
        {
            Serial.print("Service Time: ");
            Serial.print(millis() - a);
            Serial.println("ms.");
        }
        return;
    }
    else if (apiname == "/loadconfig")
    {
        String config_msg = "config:";
        config_msg += Config.toString();
        request->send(200, "text/plain", config_msg);
        if (debug.web)
        {
            Serial.print("Service Time: ");
            Serial.print(millis() - a);
            Serial.println("ms.");
        }
        return;
    }
    else if (apiname == "/saveconfig")
    {
    }
    else if (apiname == "/systeminfo")
    {
        String msg = "sysinfo:";
        msg += SysInfo.toString();
        request->send(200, "text/plain", msg);
        if (debug.web)
        {
            Serial.print("Service Time: ");
            Serial.print(millis() - a);
            Serial.println("ms.");
        }
        return;
    }
    else if (apiname == "/reqwifisearch")
    {
        WiFi.scanNetworks(true);
        while (WiFi.scanComplete() < 0)
        {
            esp_task_wdt_reset();
        }

        String msg = "wifiAP:";
        msg += getApAvailables();
        msg += ";";
        AsyncWebServerResponse *res = request->beginResponse(200, "text/plain", msg);
        request->send(res);

        if (debug.web)
        {
            Serial.print("Service Time: ");
            Serial.print(millis() - a);
            Serial.println("ms.");
        }
        return;
    }
    else if (apiname == "/newcaltest")
    {
        //new_cal:raw:cal;
        String msg = "new_cal:";
        float temp = readTemperature();
        msg += temp;
        msg += ':';
        if (request->hasArg("cal"))
        msg += temp + atof(request->arg("cal").c_str());
        AsyncWebServerResponse *res = request->beginResponse(200, "text/plain", msg);
        request->send(res);

        if (debug.web)
        {
            Serial.print("Service Time: ");
            Serial.print(millis() - a);
            Serial.println("ms.");
        }
        return;
    }
    else if (apiname == "/newconfig")
    {
        //new_cal:raw:cal;
        if (request->hasArg("config"))
        Config.fromString(request->arg("config"));
        AsyncWebServerResponse *res = request->beginResponse(200, "text/plain", "done");
        request->send(res);

        if (debug.web)
        {
            Serial.print("Service Time: ");
            Serial.print(millis() - a);
            Serial.println("ms.");
        }
        return;
    }

    if (is_SD_Mount)
    {
        if (debug.web)
        {
            Serial.println("looking in SD Card");
        }

        if (SD_MMC.exists(filename))
        {
            SendLargeFiles(request, filename);
            if (debug.web)
            {
                Serial.print("Service Time: ");
                Serial.print(millis() - a);
                Serial.println("ms.");
            }
            return;
        }
        else if (SD_MMC.exists(filename + ".html"))
        {
            SendLargeFiles(request, filename + ".html");
            if (debug.web)
            {
                Serial.print("Service Time: ");
                Serial.print(millis() - a);
                Serial.println("ms.");
            }
            return;
        }
        else if (SD_MMC.exists(filename + ".css"))
        {
            SendLargeFiles(request, filename + ".css");
            if (debug.web)
            {
                Serial.print("Service Time: ");
                Serial.print(millis() - a);
                Serial.println("ms.");
            }
            return;
        }
        else if (SD_MMC.exists(filename + ".js"))
        {
            SendLargeFiles(request, filename + ".js");
            if (debug.web)
            {
                Serial.print("Service Time: ");
                Serial.print(millis() - a);
                Serial.println("ms.");
            }
            return;
        }
        else if (SD_MMC.exists(apiname))
        {
            SendLargeFiles(request, apiname);
            if (debug.web)
            {
                Serial.print("Service Time: ");
                Serial.print(millis() - a);
                Serial.println("ms.");
            }
            return;
        }
    }

    else if (is_SPIFFSS_Mount)
    {
        if (debug.web)
        {
            Serial.println("looking in SPIFFS");
        }

        if (SPIFFS.exists(filename))
        {
            File _file = SPIFFS.open(filename);
            request->send(200, getMIME(filename), _file.readString());
            _file.close();
            return;
        }
        else if (SPIFFS.exists(filename + ".html"))
        {
            File _file = SPIFFS.open(filename + ".html");
            request->send(200, getMIME(filename + ".html"), _file.readString());
            _file.close();
            Serial.print(millis() - a);
            Serial.println("ms.");
            return;
        }
        else if (SPIFFS.exists(filename + ".css"))
        {
            File _file = SPIFFS.open(filename + ".css");
            request->send(200, getMIME(filename + ".css"), _file.readString());
            _file.close();
            return;
        }
        else if (SPIFFS.exists(filename + ".js"))
        {
            File _file = SPIFFS.open(filename + ".js");
            request->send(200, getMIME(filename + ".js"), _file.readString());
            _file.close();
            return;
        }
    }

    String message = "File Not Found\n\n";
    message += "URI: ";
    message += request->url();
    message += "\nMethod: ";
    message += request->method();
    message += "\nArguments: ";
    message += request->args();
    message += "\n";

    for (uint8_t i = 0; i < request->args(); i++)
    {
        message += " " + request->argName(i) + ": " + request->arg(i) + "\n";
    }
    request->send(404, "text/plain", message);
    if (debug.web)
    {
        Serial.println(message);
        Serial.print("Service Time: ");
        Serial.print(millis() - a);
        Serial.println("ms.");
    }
    return;
}

class EstufaRequestHandler : public AsyncWebHandler
{
public:
    EstufaRequestHandler() {}
    virtual ~EstufaRequestHandler() {}

    bool canHandle(AsyncWebServerRequest *request)
    {
        //request->addInterestingHeader("ANY");
        return true;
    }

    void handleRequest(AsyncWebServerRequest *request)
    {
        handleWebServer(request);
    }
};

String getApAvailables()
{
    String returnmsg = "";
    byte numSsid = WiFi.scanComplete();
    if (numSsid == 0xFE)
    {
        numSsid = WiFi.scanNetworks();
    }
    for (byte i = 0; i < numSsid; i++)
    {
        returnmsg += WiFi.SSID(i);
        returnmsg += '=';
        returnmsg += WiFi.RSSI(i);
        returnmsg += ',';
    }
    return returnmsg;
}

#pragma region OTA
void startOTA()
{
    String type;
    is_updating = true;
    //caso a atualização esteja sendo gravada na memória flash externa, então informa "flash"
    SPIFFS.end();
    if (ArduinoOTA.getCommand() == 0)
        type = "flash";
    else                     //caso a atualização seja feita pela memória interna (file system), então informa "filesystem"
        type = "filesystem"; // U_SPIFFS
    //exibe mensagem junto ao tipo de gravação
    Serial.println("Start updating " + type);
}
//exibe mensagem
void endOTA()
{
    Serial.println("\nEnd");
}
//exibe progresso em porcentagem
void progressOTA(unsigned int progress, unsigned int total)
{
    update_progress = (float)progress / total * 100;
    Serial.printf("Progress: %u%%\r\n", (progress / (total / 100)));
}

void errorOTA(ota_error_t error)
{
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
        Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR)
        Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR)
        Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR)
        Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR)
        Serial.println("End Failed");

    is_updating = false;
}
#pragma endregion

void listAllFiles(String dir = "/", bool SD_card = false, byte treesize = 0)
{
    ///SPIFFS mode
    if (!SD_card)
    {
        // List all available files (if any) in the SPI Flash File System
        if (!is_SPIFFSS_Mount)
        {
            Serial.print("SPIFFS not Mount");
            return;
        }
        Serial.print("Used Bytes: ");
        Serial.print(SPIFFS.usedBytes());
        Serial.print("-----Total Bytes: ");
        Serial.print(SPIFFS.totalBytes());
        Serial.print("-----Used: ");
        Serial.print(map(SPIFFS.usedBytes(), 0, SPIFFS.totalBytes(), 0, 100));
        Serial.println("%");
        Serial.print("Listing files in: ");
        Serial.println(dir);
        File root = SPIFFS.open(dir);
        File file = root.openNextFile();
        while (file)
        {
            for (size_t i = 0; i < treesize; i++)
            {
                Serial.print("--");
            }
            Serial.print("FILE: ");
            Serial.print(file.path());
            Serial.print(" size: ");
            Serial.print((float)file.size() / 1024);
            Serial.println("Kb");
            if (file.isDirectory())
                listAllFiles(file.path(), false, treesize + 1);
            file = root.openNextFile();
        }
        root.close();
        file.close();
    }
    //SD card mode
    else if (SD_card)
    {
        if (!is_SD_Mount)
        {
            Serial.print("SD card not Mount");
            return;
        }
        if (treesize == 0)
        {
            Serial.print("Used Bytes: ");
            Serial.print(SD_MMC.usedBytes());
            Serial.print("-----Total Bytes: ");
            Serial.print(SD_MMC.totalBytes());
            Serial.print("-----Used: ");
            Serial.print(map(SD_MMC.usedBytes(), 0, SD_MMC.totalBytes(), 0, 100));
            Serial.println("%");
            Serial.print("Listing files in: ");
            Serial.println(dir);
        }
        File root = SD_MMC.open(dir);
        File file = root.openNextFile();
        while (file)
        {
            for (size_t i = 0; i < treesize; i++)
            {
                Serial.print("--");
            }

            Serial.print("FILE: ");
            Serial.print(file.path());
            Serial.print(" size: ");
            Serial.print((float)file.size() / 1024);
            Serial.println("Kb");
            if (file.isDirectory())
                listAllFiles(file.path(), true, treesize + 1);
            file = root.openNextFile();
        }
        root.close();
        file.close();
        // if (root.path() == "/Logs")
        // SD_MMC.remove(file.path());
    }
}

void loadConfig()
{
    byte mode = 0x0;

    if (is_SPIFFSS_Mount)
    {
        if (SPIFFS.exists("/config.cfg"))
            mode = 0x02;
        else
            mode = 0x01;
    }
    if (is_SD_Mount)
    {
        if (SD_MMC.exists("/config.cfg"))
            mode += 0x20;
        else
            mode += 0x10;
    }

#ifdef DEBUGMODE

    Serial.println("----- Config loader -----");
    Serial.println("mode = 0xab || a = SPIFFS. b = SD. || 0 = not mount 1 = mount 2 = found.");
    Serial.print("#########    mode = 0x");
    if (mode < 0x10)
        Serial.print(0);
    Serial.print(mode, HEX);
    Serial.println("    #########");
    Serial.println("----- Config loader END-----");

#endif

    if (mode == 0x0)
    {
        Serial.println("SPIFFS and SD filesystems not Found.");
        return;
    }

    else if (mode == 0x21 || mode == 0x20)
    {
        Serial.print("SPIFFS config not found, loading from SD.");
        String _setting = SD_MMC.open("/config.cfg").readString();
        if (mode == 0x21)
        {
            Serial.print("....Making a copy of SD card's one into SPIFFS....");
            File_Writer("/config.cfg", _setting, false, true);
        }

        Config.LoadValidated(_setting) ? Serial.println(". Sucess!") : Serial.println(". Failed Checksum.");
    }

    else if (mode == 0x22)
    {
        Serial.print("Found Config.cfg files in both SD and SPIFFS. Loading from SD");
        Config.LoadValidated(SD_MMC.open("/config.cfg").readString()) ? Serial.println(". Sucess!") : Serial.println(". Failed Checksum.");
    }
    else if (mode == 0x12 || mode == 0x02)
    {
        Serial.print("Found config.cfg in SPIFFS. Loading...");
        Config.LoadValidated(SPIFFS.open("/config.cfg").readString()) ? Serial.println(". Sucess!") : Serial.println(". Failed Checksum.");
    }
    else if (mode == 0x11 || mode == 0x10 || mode == 0x01)
    {
        Serial.println("No /config.cfg found.");
    }
}

void listDir(fs::FS &fs, const char *dirname, byte filetree = 1)
{
    if (filetree == 1)
        Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if (!root)
    {
        Serial.println("Failed to open directory");
        return;
    }
    if (!root.isDirectory())
    {
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        Serial.print(' ');
        for (size_t i = 0; i < filetree; i++)
        {
            Serial.print("--");
        }

        if (file.isDirectory())
        {
            Serial.print("DIR : ");
            Serial.print(file.name());
            Serial.println();
            String subdirname = dirname;
            subdirname += file.name();
            listDir(fs, subdirname.c_str(), filetree++);
        }
        else
        {
            Serial.print("FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.print(file.size());
            Serial.println();
        }
        file = root.openNextFile();
    }

    Serial.println();
}

void clearDir(String dir)
{
    if (!is_SD_Mount)
    {
        return;
    }

    Serial.print("Deleting all files in: ");
    Serial.print(dir);
    File root = SD_MMC.open(dir);
    File file = root.openNextFile();
    while (file)
    {

        String path = file.path();
        if (path != "/Logs/General.log")
            ;
        {
            SD_MMC.remove(file.path());
            Serial.print("Deleting ");
            Serial.println(file.path());
        }
        file = root.openNextFile();
        Serial.print(".");
    }
    String log = "All Files Deleted in: \'";
    log += dir;
    log += '\'';
    GeneralLog(log);
    Serial.println("Done.");
}

void StandardTesting()
{

    float temps[5] = {40.5, 50.5, 30.5, 50.4, 60};
    float mins[5] = {1.5, 2.6, 1.4, 1.6, 1};
    String pname = "stdTest";

    currentRamp.totalsteps = 5;
    for (size_t i = 0; i < 5; i++)
    {
        currentRamp.minuteSteps[i] = mins[i];
        currentRamp.temperatureSteps[i] = temps[i];
    }
    currentRamp.projectName = pname;
    currentRamp.startLogs();
    currentRamp.configured = true;
    currentRamp.heatUp = true;
    StartRamp();
}

void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
  if(!index){
    Serial.print((String)"UploadStart: ");
Serial.println(filename);
  }
  for(size_t i=0; i<len; i++){
    Serial.write(data[i]);
  }
  if(final){
    Serial.print((String)"UploadEnd: ");
    Serial.print(filename );
    Serial.print(",");
    Serial.print(index);
    Serial.print(",");
    Serial.println(len); 
  }
}

void setup()
{

    Serial.begin(115200); //Start Serial

    //Start SD Card
    if (SD_MMC.begin("/sd", true, false))
        is_SD_Mount = true;

    int oldTime = millis();

    if (!is_SD_Mount)
        Serial.println("Error initializing SD");
    else
    {
        Serial.print("SD Card mounted. Size: ");
        Serial.print(SD_MMC.cardSize() / 1024 / 1024);
        Serial.print("Mb.  Used: ");
        Serial.print((float)(SD_MMC.usedBytes()) / SD_MMC.totalBytes() * 100);
        Serial.println("%");
    }
    if (!SPIFFS.begin()) //Start SPIFFS
    {
        Serial.println("Error initializing SPIFFS");
        is_SPIFFSS_Mount = false;
    }
    else
    {
        Serial.print("SPIFFS mounted. Size: ");
        Serial.print(SPIFFS.totalBytes() / 1024);
        Serial.print("kb.  Used: ");
        Serial.print((float)SPIFFS.usedBytes() / SPIFFS.totalBytes() * 100);
        Serial.println("%");
        is_SPIFFSS_Mount = true;
    }

    loadConfig();

    oneWire = new OneWire(Config.ONEWIRE_BUS);
    DS18 = new DallasTemperature(oneWire);

    adcCorrector.setup();
    adcCorrector.fromString(Config.ADC_CORRECTION);
    //Initializing Arrays and Data Structs.
    for (int i = 0; i < HISTORY_SIZE; i++)
    {
        DS18Temp[i] = DS18Time[i] = 0;
    }
    currentRamp.reset();
    lastRamp.reset();

    //Start SPIFFS
    pinMode(Config.RELAY_PIN, OUTPUT); //Set the Relay pin to output

    WiFi.mode(WIFI_STA); //WiFi Station Mode
    WiFi.begin(Config.WIFI_SSID.c_str(), Config.WIFI_PASSWORD.c_str());

    bool createAp = false;

    bool beauty = true;
    while (WiFi.status() != WL_CONNECTED && !createAp)
    {
        if (millis() % 100 == 0 && beauty)
        {
            Serial.print(".");
            beauty = false;
        }
        else if (millis() % 100 != 0)
        {
            beauty = true;
        }

        if (millis() - oldTime > 14999)
        {
            Serial.printf("\nNetwork '%s' not found.\n", Config.WIFI_SSID);
            createAp = true;
        }
    }

    if (createAp)
    {
        WiFi.mode(WIFI_AP_STA);
        WiFi.softAP(Config.WIFI_AP_SSID.c_str(), Config.WIFI_AP_PASSWORD.c_str());
        // dnsServer.start(53, "*", WiFi.softAPIP());
        Serial.println("");
        Serial.print("Creating WiFi Ap.SSID:  ");
        Serial.println(Config.WIFI_AP_SSID);
        Serial.print("IP address: ");
        Serial.println(WiFi.softAPIP());
    }
    else
    {

        Serial.println();
        Serial.print("Connected to ");
        Serial.println(Config.WIFI_SSID);
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    }

    // server.on("/", handleRoot);
    // server.onNotFound(handleWebServer);
    // server.on("/doPage", handleDoPage);
    // server.on("/do", handleNewRamp);

    // server.on("/Common.css", handleCSS);
    // server.on("/requpdate", handleRequestUpdate);
    // server.on("/reqhist", handleRequestHist);
    // server.on("/CommonScript.js", handleCommonJs);
    // server.on("/FileSaver.js", handleFileSaverjs);
    // server.on("/favicon.ico", handlefav);
    // server.on("/reqstate", handleReqState);
    // server.on("/start", handleStart);
    // server.on("/stop", handleStop);
    // server.on("/settime", handleSetTime);

    // server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER); //only when requested from AP
    //more handlers...server.begin();

    //Start HTTP Server
    server.addHandler(new EstufaRequestHandler()).setFilter(ON_STA_FILTER);

    if (MDNS.begin(Config.host_name.c_str())) //host_name.local/
    {
        Serial.println("MDNS responder started");
    }

    // ArduinoOTA.setPassword("admin");
    // Password can be set with it's md5 value as well
    // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
    // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");
    // ArduinoOTA.setHostname("ESTUFA");
    ArduinoOTA.onStart(startOTA);
    ArduinoOTA.onEnd(endOTA);
    ArduinoOTA.onProgress(progressOTA);
    ArduinoOTA.onError(errorOTA);
    ArduinoOTA.begin();
server.onFileUpload(handleUpload);

    GeneralLog("Booted");
    getTime();
    DS18->begin();
    sd_mutex = xSemaphoreCreateCounting(3, 3);
    if (sd_mutex == NULL)
    {
        Serial.println("sd mutex = ! NULL MUTEX !");
    }
    get_ap_mutex = xSemaphoreCreateBinary();

    if (get_ap_mutex == NULL)
    {
        Serial.println("get ap mutex = ! NULL MUTEX !");
    }
    File_Writer("/Web/test.html", GetPage());
    File_Writer("/Web/Settings.js", GetPage2());

    server.begin();
    Serial.println("HTTP server started");

    if (Config.enableCache)
    {
        Cache.setup();
        Cache.LoadPage("/FileSaver.js", "/Web/FileSaver.js");
        Cache.LoadPage("/doPage.html", "/Web/doPage.html");
        Cache.LoadPage("/MainPage.html", "/Web/MainPage.html");
        Cache.LoadPage("/Common.css", "/Web/Common.css");
        Cache.LoadPage("/Circles.css", "/Web/Circles.css");
        Cache.LoadPage("/Circles.js", "/Web/Circles.js");
        Cache.LoadPage("/Graphs.js", "/Web/Graphs.js");
        Cache.LoadPage("/medium-slider.css", "/Web/medium-slider.css");
    }

    Serial.print("Setup done. time: ");
    Serial.print(millis() - oldTime);
    Serial.println("ms");

    SysInfo.print(true);
    #ifdef DEBUGWEB
    debug.web = true;
    #endif
    #ifdef DEBUGGENERAL
    debug.general = true;
    #endif
    #ifdef DEBUGLOG
    debug.log = true;
    #endif
    #ifdef DEBUGTEMP
    debug.temp = true;
    #endif
    #ifdef DEBUGRAMP
    debug.ramp = true;
    #endif
}

void loop()
{

    // oldLoopMillis = millis();
    ArduinoOTA.handle();
    // request->handleClient();
    updateTemperatures();

    // Serial.print("run time: ");
    // Serial.print(millis() - time);
    // Serial.println("ms");
    if (Serial.available())
    {
        String s = "";
        while (Serial.available() > 0)
        {
            char c = Serial.read();
            if (c != 10 && c != 13)
            {
                s += c;

                //Serial.print((int)c);
                //Serial.print('.');
            }
        }
        if (s != "")
        {
            Serial.print(">");
            Serial.println(s);
        }
        if (s == "web")
        {
            listDir(SD_MMC, "/Web");
        }
        else if (s == "debug web")
        {
            debug.web = !debug.web;
            Serial.print("Debug Web is now: ");
            Serial.println(debug.web);
        }
        else if (s == "debug ramp")
        {
            debug.ramp = !debug.ramp;
            Serial.print("Debug ramp is now: ");
            Serial.println(debug.ramp);
        }
        else if (s == "debug temp")
        {
            debug.temp = !debug.temp;
            Serial.print("Debug temp is now: ");
            Serial.println(debug.temp);
        }
        else if (s == "debug log")
        {
            debug.log = !debug.log;
            Serial.print("Debug log is now: ");
            Serial.println(debug.log);
        }
        else if (s == "debug general")
        {
            debug.general = !debug.general;
            Serial.print("Debug general is now: ");
            Serial.println(debug.general);
        }
        else if (s == "heatramp")
        {
            Serial.println(currentRamp.toString());
        }
        else if (s == "mem")
        {
            int mem = ESP.getFreeHeap();
            Serial.print("Free Heap: ");
            Serial.print(mem / 1024);
            Serial.println("kb.");
        }
        else if (s == "info")
        {
            SysInfo.print(true);
        }
        else if (s == "cache")
        {
            for (size_t i = 0; i < MAX_CACHE_PAGES; i++)
            {
                Serial.print("(");
                Serial.print(i);
                Serial.print(") Key: ");
                Serial.print(Cache.Keys[i]);
            }
        }
        else if (s == "drop cache")
        {
            Cache.setup();
        }
         else if (s == "config")
        {
         Serial.println(Config.toString());
        }
        else if (s == "led")
        {
            digitalWrite(4, !digitalRead(4));
        }
        else if (s == "time")
        {
            Serial.println(startup_time);
        }
        else if (s == "net")
        {
            Serial.print(WiFi.scanComplete());
        }
        else if (SD_MMC.exists(s))
        {
            Serial.print("FILE: ");
            Serial.println(s);
            Serial.println("-------------");
            if (SD_MMC.open(s).isDirectory())
                listAllFiles(s, true);
            else
                Serial.println(SD_MMC.open(s).readString());
            Serial.println("-------------");
        }
    }

    //   Serial.print("loop Time:");
    //  Serial.print(millis() - oldLoopMillis);
    //  Serial.println("ms.");
}