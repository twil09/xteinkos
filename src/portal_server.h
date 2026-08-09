// ============================================================================
//  portal_server.h — captive WiFi portal for first-run configuration
// ----------------------------------------------------------------------------
//  Brings up the "X3-OSINT" soft-AP, answers the OS captive-portal probes so
//  the config page auto-opens, and serves a Duet-styled form. On submit the
//  entered settings are captured for the caller to persist + reboot into.
//
//  The credentials collected here are the DEVICE OWNER'S OWN home WiFi, used to
//  provision this device — the standard IoT onboarding pattern.
// ============================================================================
#pragma once

#include <Arduino.h>
#include <DNSServer.h>
#include <WebServer.h>

#include "config.h"
#include "config_manager.h"

class PortalServer {
 public:
  // Start the soft-AP + DNS + web server.
  void begin(const char* ssid = AP_SSID, const char* password = AP_PASSWORD);

  // Pump DNS + HTTP; call frequently from loop().
  void handle();

  bool getConfigReceived() const { return configReceived_; }

  // Populate a DeviceConfig from the submitted form (valid once received).
  void getConfig(DeviceConfig& out) const { out = pending_; }

  // Spec-compatible string getter.
  void getConfig(String& ssid, String& pwd, String& mode, String& url) const;

  IPAddress apIP() const { return apIP_; }

 private:
  DNSServer    dns_;
  WebServer    server_{AP_HTTP_PORT};
  IPAddress    apIP_{192, 168, 4, 1};
  DeviceConfig pending_;
  bool         configReceived_ = false;
  bool         started_ = false;

  void routes_();
  void handleRoot_();
  void handleConnect_();
  void redirectToRoot_();
};
