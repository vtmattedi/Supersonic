# Supersonic [Old - do not use]

Supersonic is a ESP32 software for temperature control of external components with html interface


## Supported Temperature Sensors:

-Digital Sensors:  
    DS18b20 family

-NTC Sensors:  
    any


## Available APIs and Usage

### API Routine:
The api routine starts by checking if the ESP is updating, then it checks for pages in the cache, then it tries to serve an api, then it tries to serve a file with the webpage's
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
'/newconfig'  ''' 


### Requests:
'/requpdate' returns a snapshop of current Heat Ramp object Stringfied



## Local credentials

Before building, copy `include/WifiCred.example.h` to `include/WifiCred.h` and
fill in all five settings locally. `WifiCred.h` is ignored by Git. Do not put
credentials in source files, build flags, backups, or commits. The firmware
requires nonempty Wi-Fi settings and a web username, a web password of at least
12 characters, and an access-point password of 12 to 63 characters. Configuration
resets use these local values rather than shared defaults.

Previously committed Wi-Fi and access-point passwords are exposed. Change any
that are still in use, including reused passwords. Update stored configuration
on deployed devices as well; rebuilding does not replace persisted settings.
Removing files or adding them to `.gitignore` does not erase Git history.
Firmware binaries also contain configured credentials; keep those private.

Before committing, run Gitleaks 8.30.1 or newer from the repository root:

```sh
gitleaks dir . --redact=100 --max-archive-depth=3
```

The checked-in rules include low-entropy firmware passwords. Scan a clean
checkout before copying local credentials; a local populated `WifiCred.h` is
expected to trigger a directory scan. History scans will continue to find the
old passwords until the published history has been cleaned.
