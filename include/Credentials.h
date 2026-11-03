#pragma once

// This file is tracked; WifiCred.h contains your private, local configuration.
#if !__has_include("WifiCred.h")
#error "Copy include/WifiCred.example.h to include/WifiCred.h and set your credentials."
#else
#include "WifiCred.h"

#if !defined(WIFISSID) || !defined(WIFIPASSWD) || !defined(WEB_USERNAME) || !defined(WEB_PASSWORD) || !defined(DEVICE_AP_PASSWORD)
#error "Update WifiCred.h using WifiCred.example.h; all five settings are required."
#else
static_assert(sizeof(WIFISSID) > 1, "Set WIFISSID in WifiCred.h.");
static_assert(sizeof(WIFIPASSWD) > 1, "Set WIFIPASSWD in WifiCred.h.");
static_assert(sizeof(WEB_USERNAME) > 1, "Set WEB_USERNAME in WifiCred.h.");
static_assert(sizeof(WEB_PASSWORD) >= 13, "Use at least 12 characters for WEB_PASSWORD.");
static_assert(sizeof(DEVICE_AP_PASSWORD) >= 13 && sizeof(DEVICE_AP_PASSWORD) <= 64,
              "Use 12 to 63 characters for DEVICE_AP_PASSWORD.");
#endif
#endif
