// Net — Wi-Fi station lifecycle + a small file-transfer web server.
//
//  * begin(): on boot, restore saved creds and connect (auto-reconnecting), so
//    the link comes back after every wake instead of silently staying down.
//  * poll(): call every loop; services the file server and (re)starts NTP/mDNS
//    when the link comes up.
//  * the file server lets a phone on the same Wi-Fi upload/download/delete books
//    and images at http://vix.local/ (or the shown IP).
#pragma once
#include <Arduino.h>

namespace Net {

void   begin();                 // restore creds, connect, enable auto-reconnect
void   connectSaved();          // (re)connect using stored credentials
bool   connected();
String ssid();
String ip();
String url();                   // IP URL, e.g. "http://192.168.1.5/" (QR-friendly)
String hostUrl();              // "http://vix.local/" (human-friendly name)

void   poll();                  // service server + NTP/mDNS housekeeping

void   startFileServer();       // begin serving the transfer page
void   stopFileServer();
bool   fileServerRunning();

}  // namespace Net
