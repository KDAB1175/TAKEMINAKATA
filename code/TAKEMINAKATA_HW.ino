// include all the necessary libraries
#include <Arduino.h>
#include <Wire.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <SparkFun_SCD30_Arduino_Library.h>
#include <time.h>
#include <TimeLib.h>
#include <DFRobot_SGP40.h>
//#include <SparkFun_SCD4x_Arduino_Library.h>
#include <SensirionI2CScd4x.h>
#include <OneWire.h>
#include <DallasTemperature.h>


#define SCD30_ADDRESS 0x61  // I2C address of SCD30
const int oneWireBus = 26;

// set-up constants for Wi-Fi - some might be depreciated later on
WiFiServer server(80);
SCD30 airSensor;
DFRobot_SGP40 mySgp40;
SensirionI2CScd4x scd4x;
//const char *ssid = "Zahrada";
//const char *password = "NasDomaciRouterNaZahradu2012!";
const char *url = "https://takeminakata.snad.cz/insertmko29ijnbhu8.php";

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature sensors(&oneWire);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

const long timezoneOffset = 3600 * 1; // For example, GMT+2

void setup() {
  // set serial output to 115200 baud for Serial Monitor to read
  Serial.begin(115200);

  // Wire lib initialization
  Wire.begin();
}

void requestToWiFi();

void loop() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An error occurred while mounting SPIFFS");
    return;
  }

  requestToWiFi();

  wifiConnect(readData("/ssid.txt"), readData("/passwd.txt"));
  Serial.println(readData("/ssid.txt"));
  Serial.println(readData("/passwd.txt"));
  Serial.println(WiFi.status());
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("error-no-wifi");
    requestToWiFiExisting();
    delay(30000);
    wifiConnect(readData("/ssid.txt"), readData("/passwd.txt"));
  }
  Serial.println("here");

  // Main part for sensor readings and long pause for the next measurement
  makePostRequest();
  //readSCD30();
  delay(1200000);
  //delay(10000);
}

void requestToWiFi() {
  String data = readData("/ssid.txt");
  //Serial.println("Data from SPIFFS: " + data);
  if (data == "") {
    // Set up SoftAP (Access Point) without a password
    WiFi.softAP("TAKEMINAKATA-WiFi", "");
    // Start the server
    server.begin();

    while (readData("/ssid.txt") == "" || readData("/passwd.txt") == "") {
      // Check for connected devices every 5 seconds
      delay(5000);
      listConnectedDevices();

      // Check for incoming clients
      WiFiClient client = server.available();
      if (client) {
        handleClient(client);
      }
    }
  }
}

void requestToWiFiExisting() {
  // Set up SoftAP (Access Point) without a password
  Serial.println(WiFi.status());
  Serial.println("requestToWiFiExisting()");
  WiFi.softAP("TAKEMINAKATA-WiFi", "");
  // Start the server
  server.begin();
  int handled = 0;

  while (handled == 0) {
    // Check for connected devices every 5 seconds
    delay(5000);
    listConnectedDevices();

    // Check for incoming clients
    WiFiClient client = server.available();
    if (client) {
      handleClient(client);
      handled = 1;
    }
  }
}

// writes important credentials to device's long-term memory
int writeData(const char* path, String data) {
  File file = SPIFFS.open(path, "w");
  if (!file) {
    Serial.println("Failed to open file for writing");
    return 0;
  }

  if (file.print(data)) {
    Serial.println("File written successfully");
    file.close();
    return 1;
  } else {
    Serial.println("Write failed");
    file.close();
    return 0;
  }
}

// reads data from device's long-term memory
String readData(const char* path) {
  String data = "";
  File file = SPIFFS.open(path, "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return data;
  }

  while (file.available()) {
    data += (char)file.read();
  }

  file.close();
  return data;
}

// equivalent of the setup function for connecting the device to a point on this device's network
void connectionOfDevice() {
  const char *ssid = "TAKEMINAKATA-WiFi";
  const char *password = "";

  // Set up SoftAP (Access Point) without a password
  WiFi.softAP(ssid, password);

  Serial.println("SoftAP started. Waiting for clients...");

  // Start the server
  server.begin();

  // Call the loop function equivalent for this function
  connectionOfDeviceLoop();

  server.stop();
  WiFi.softAPdisconnect(true);
}

// equivalent of the loop function for connecting the device to a point on this device's network
void connectionOfDeviceLoop() {
  // Check for connected devices every 5 seconds
  delay(5000);
  listConnectedDevices();

  // Check for incoming clients
  WiFiClient client = server.available();
  if (client) {
    handleClient(client);
  }
}

// lists devices connected on the device's network
void listConnectedDevices() {
  Serial.println("Connected devices:");

  // Get the number of connected devices
  int numDevices = WiFi.softAPgetStationNum();

  if (numDevices > 0) {
    Serial.println("Number of devices: " + String(numDevices));

    // Print the MAC addresses of connected devices
    uint8_t connectedDevices[numDevices][6];
    //WiFi.softAPgetStationBSSID(connectedDevices);

    for (int i = 0; i < numDevices; i++) {
      Serial.print("Device " + String(i + 1) + ": MAC Address ");
      for (int j = 0; j < 6; j++) {
        Serial.print(connectedDevices[i][j], HEX);
        if (j < 5) Serial.print(":");
      }
      Serial.println();
    }
  } else {
    Serial.println("No devices connected.");
  }

  Serial.println();
}

// handles get request from a point on this device's network
void handleClient(WiFiClient client) {
  // Read the request from the client
  String request = client.readStringUntil('\r');
  Serial.println("Request: " + request);

  // Parse parameters from the request
  String ssidParam;
  String passwdParam;
  String tokenParam;
  String deviceNameParam;

  // Extracting parameters from the request
  if (request.indexOf("?ssid=") != -1 &&
      request.indexOf("&passwd=") != -1 &&
      request.indexOf("&T=") != -1 &&
      request.indexOf("&DN=") != -1) {

    ssidParam = getValue(request, "ssid");
    passwdParam = getValue(request, "passwd");
    tokenParam = getValue(request, "T");
    deviceNameParam = getValue(request, "DN");
  }

  Serial.println(ssidParam);
  Serial.println(passwdParam);
  Serial.println(tokenParam);
  Serial.println(deviceNameParam);

  // Check if write was successful, otherwise respond with an error
  if (writeData("/ssid.txt", ssidParam) != 1) {
    client.println("error-w-ssid");
    return;
  }

  if (writeData("/passwd.txt", passwdParam) != 1) {
    client.println("error-w-passwd");
    return;
  }

  if (writeData("/token.txt", tokenParam) != 1) {
    client.println("error-w-token");
    return;
  }

  if (writeData("/deviceName.txt", deviceNameParam) != 1) {
    client.println("error-w-devicename");
    return;
  }

  // Send a simple response to the client
  Serial.println("sending response...");
  //client.println("HTTP/1.1 200 OK");
  //client.println("Content-type:text/html");
  //client.println();
  client.println("read-w-all");
  //Serial.println("HTTP/1.1 200 OK");
  //Serial.println("Content-type:text/html");
  //Serial.println();
  //Serial.println("<html><body><h1>Hello from ESP32!</h1></body></html>");
  Serial.println("response sent");

  // Close the connection
  //delay(5000);
  //client.stop();
}

String getValue(String data, String key) {
  String startKey = key + "=";
  int startIndex = data.indexOf(startKey);
  if (startIndex == -1) return ""; // Key not found

  startIndex += startKey.length();

  int endIndex = data.indexOf('&', startIndex);
  if (endIndex == -1) {
    endIndex = data.length();
  }

  return data.substring(startIndex, endIndex);
}

void wifiConnect(String ssid, String password) {
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  /*while*/ if (WiFi.status() != WL_CONNECTED) {
    delay(5000);
    Serial.println("Connection failed");
  } else {
    Serial.println("Connected to WiFi");
  }
}

void postReqLoop() {
  // Make POST request
  timeClient.begin();
  makePostRequest();

  delay(10000);
}

String getCurrentTime() {
  // Update the NTP client and get the current time
  timeClient.update();
  unsigned long epochTime = timeClient.getEpochTime();

  // Adjust for timezone offset
  epochTime += timezoneOffset;

  // Convert epoch time to time_t
  time_t adjustedTime = static_cast<time_t>(epochTime);

  // Convert time_t to struct tm
  struct tm *tm;
  tm = localtime(&adjustedTime);

  char formattedTime[20];
  sprintf(formattedTime, "%04d/%02d/%02d %02d:%02d:%02d",
          tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
          tm->tm_hour, tm->tm_min, tm->tm_sec);

  Serial.println(formattedTime);
  String finalTime = String(formattedTime);

  return finalTime;
}

void makePostRequest() {
  HTTPClient http;

  // Your JSON payload
  if (!mySgp40.begin()) {
    Serial.println("Failed to initialize SGP40");
    while (1);
  }

  scd4xsetup();
  sensors.begin();
  sensors.requestTemperatures();

  String jsonPayload = "{\"T\":\"" + readData("/token.txt") + "\","
                       "\"DN\":\"" + readData("/deviceName.txt") + "\","
                       "\"DT\":\"" + getCurrentTime() + "\","
                       "\"SCD30\":" + readSCD30() + ","
                       "\"SCD40\":" + scd4xmeasurement() + ","
                       "\"SGP40\":" + mySgp40.getVoclndex() + ","
                       "\"SHT85Temp\":" + 0.0 + ","
                       "\"SHT85Humd\":" + scd4xmeasurementHumd() + ","
                       "\"DS18In\":" + 0.0 + ","
                       "\"DS18Out\":" + sensors.getTempCByIndex(0) + ","
                       "\"MQ135\":" + 0.0 + "}";

  // Start the POST request
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  Serial.println("post: ");
  Serial.println(readData("/token.txt"));
  Serial.println(readData("/deviceName.txt"));
  Serial.println(getCurrentTime());
  Serial.println(jsonPayload);
  Serial.println(" :end");

  // Send the POST request with the JSON payload
  int httpResponseCode = http.POST(jsonPayload);

  String response = http.getString();
  Serial.println("Response:");
  Serial.println(response);

  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);

  // Check for a successful POST request
  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);

    // Print the response payload
    String responsePayload = http.getString();
    Serial.println(responsePayload);
  } else {
    Serial.print("HTTP POST request failed, error: ");
    Serial.println(httpResponseCode);
  }

  // End the request
  http.end();
}

void printUint16Hex(uint16_t value) {
  Serial.print(value < 4096 ? "0" : "");
  Serial.print(value < 256 ? "0" : "");
  Serial.print(value < 16 ? "0" : "");
  Serial.print(value, HEX);
}

void printSerialNumber(uint16_t serial0, uint16_t serial1, uint16_t serial2) {
  Serial.print("Serial: 0x");
  printUint16Hex(serial0);
  printUint16Hex(serial1);
  printUint16Hex(serial2);
  Serial.println();
}

void scd4xsetup() {
  uint16_t error;
  char errorMessage[256];

  scd4x.begin(Wire);

  // stop potentially previously started measurement
  error = scd4x.stopPeriodicMeasurement();
  if (error) {
    Serial.print("Error trying to execute stopPeriodicMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  }

  uint16_t serial0;
  uint16_t serial1;
  uint16_t serial2;
  error = scd4x.getSerialNumber(serial0, serial1, serial2);
  if (error) {
    Serial.print("Error trying to execute getSerialNumber(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  } else {
    printSerialNumber(serial0, serial1, serial2);
  }

  // Start Measurement
  error = scd4x.startPeriodicMeasurement();
  if (error) {
    Serial.print("Error trying to execute startPeriodicMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  }

  Serial.println("Waiting for first measurement... (5 sec)");
}

int scd4xmeasurement() {
  uint16_t error;
  char errorMessage[256];

  delay(5000);

  // Read Measurement
  uint16_t co2;
  float temperature;
  float humidity;
  error = scd4x.readMeasurement(co2, temperature, humidity);
  if (error) {
    Serial.print("Error trying to execute readMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  } else if (co2 == 0) {
    Serial.println("Invalid sample detected, skipping.");
  } else {
    Serial.print("Co2:");
    Serial.print(co2);
    Serial.print("\t");
    Serial.print("Temperature:");
    Serial.print(temperature);
    Serial.print("\t");
    Serial.print("Humidity:");
    Serial.println(humidity);
  }

  return co2;
}

int scd4xmeasurementHumd() {
  uint16_t error;
  char errorMessage[256];

  delay(5000);

  // Read Measurement
  uint16_t co2;
  float temperature;
  float humidity;
  error = scd4x.readMeasurement(co2, temperature, humidity);
  if (error) {
    Serial.print("Error trying to execute readMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  } else if (co2 == 0) {
    Serial.println("Invalid sample detected, skipping.");
  } else {
    Serial.print("Co2:");
    Serial.print(co2);
    Serial.print("\t");
    Serial.print("Temperature:");
    Serial.print(temperature);
    Serial.print("\t");
    Serial.print("Humidity:");
    Serial.println(humidity);
  }

  return humidity;
}

/*void fileManagement() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An error occurred while mounting SPIFFS");
    return;
  }

  // Uncomment the following line to write data to SPIFFS
  // writeData("/data.txt", "Hello, ESP32!");

  // Read data from SPIFFS
  String data = readData("/data.txt");
  Serial.println("Data from SPIFFS: " + data);
  }*/

/*void writeData(const char* path, const char* data) {
  File file = SPIFFS.open(path, "w");
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }

  if (file.print(data)) {
    Serial.println("File written successfully");
  } else {
    Serial.println("Write failed");
  }

  file.close();
  }

  String readData(const char* path) {
  String data = "";
  File file = SPIFFS.open(path, "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return data;
  }

  while (file.available()) {
    data += (char)file.read();
  }

  file.close();
  return data;
  }*/

// Initialize SCD30 sensor
void resetSCD30() {
  Wire.beginTransmission(SCD30_ADDRESS);
  Wire.write(0x06);  // Command for a soft reset
  Wire.endTransmission();
  delay(200);
}

float readSCD30() {
  // Reset the SCD30 before starting a new measurement
  if (!airSensor.begin()) {
    Serial.println("Error initializing SCD30!");
    while (1);
  }

  if (airSensor.dataAvailable()) {
    Serial.print("b-4");
    float co2 = airSensor.getCO2();
    Serial.print("after");
    float temperature = airSensor.getTemperature();
    float humidity = airSensor.getHumidity();

    Serial.print("CO2: ");
    Serial.print(co2);
    Serial.print(" ppm, Temperature: ");
    Serial.print(temperature);
    Serial.print(" °C, Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");
    //delay(2000); // You can adjust the delay based on your requirements
    return co2;
  }

  airSensor.reset();

  return 0;
}

void checkI2C() {
  byte error, address;
  int nDevices;

  Serial.println("Scanning...");

  nDevices = 0;
  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("Found device at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println(" !");
      nDevices++;
    } else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);
    }
  }

  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("Done\n");

  delay(5000);  // Wait for the next scan
}
