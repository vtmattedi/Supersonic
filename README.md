# Supersonic

Supersonic is a ESP32 software for temperature control of external components with html interface


## Supported Temperature Sensors:

-Digital Sensors:  
    DS18b20 family

-NTC Sensors:  
    any


## Available APIs and Usage

### API Routine:
The api routine starts by checkin if the ESP is updating, then it checks for pages in the cache, then it tries to serve an api, then it tries to serve a file with the webpage's
name from the Web folder in the SD, then tries to serve a file plus '.html', '.css' and '.js' in that order from the SD card, then tries to serve to serve a file with the
webpage's name from the root of the SD card. After that, the process repeats on SPIFFS and finally it returns a 404 page.

### Known APIs:
'/requpdate'  
'/reqhist'  
'/reqtemp'  
'/do'  
'/settime'  
'/deleteAllFiles'  
'/csv'  
'/fileslist'  
'/deleteFile'  
'/loadconfig'  
'/saveconfig'  
'/systeminfo'  
'/newconfig'  

### Requests:
'/requpdate' returns a snapshop of current Heat Ramp object Stringfied


