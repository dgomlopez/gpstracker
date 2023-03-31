/**************************************************************************************
*  Tracker GPS (prototype)  v1.0                                                      *
*                                                                                     *
*  Author: Daniel Gómez López                                                         *
*                                                                                     *
*                                                                                     *
*  This code gets different parameters by AT commands like                            *
*  be the speed and coordinates among other parameters of a GPS device                *
*  (SIM7000G) managed through an ESP32-WROVER-B. These parameters are processed       *
*  and stored in a JSON and sent via MQTT to a graphical interface                    *
*  designed in QT.                                                                    *
***************************************************************************************/

// PINS
#define PIN_TX      27
#define PIN_RX      26
#define PWR_PIN     4

// Serial port
#define SerialMon Serial
#define SerialAT Serial1
#define UART_BAUD   115200

// MODEM
#define TINY_GSM_MODEM_SIM7000
// The following values ​​must be modified (GSM_PN, apn, gprsUser, gprsPass -> "" if not used)
#define GSM_PIN ""
const char apn[]           = "";
const char gprsUser[]      = "";
const char gprsPass[]      = "";
const char *broker         = "broker.hivemq.com";
const char *topicInit      = "TrackGPS/init";
const char *topicTracker   = "TrackGPS/gpsData";

// OLED
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#include <TinyGsmClient.h> // Modem library
#include <PubSubClient.h>  // MQTT library
#include <Ticker.h>
#include <SPI.h>
#include <ArduinoJson.h>  // JSON library
#include <Wire.h>
#include <Adafruit_GFX.h> // OLED library
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1); // I2C connected
TinyGsm       modem(SerialAT);
TinyGsmClient client(modem);
PubSubClient  mqtt(client);

// GPS raw parameters -> SIM7000 Series_AT Command Manual_V1.06 (Table 15-1, pag. 224)
int year = 0;
int month = 0;
int day = 0;
int hour = 0;
int minutes = 0;

float lat = 0;
float lon = 0;
float speed_over_ground = 0;

// Battery
float voltage_bat;
float perc_bat;

// MQTT
uint32_t lastReconnectAttempt = 0;

 // logo -> 128x32px
const unsigned char logo [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xff, 0xf0, 0x0f, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0x00, 0x00, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xfc, 0x00, 0x00, 0x3f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xf8, 0x00, 0x00, 0x1f, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xf8, 0x00, 0x00, 0x1f, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xf8, 0x00, 0x00, 0x1f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xfc, 0x00, 0x00, 0x3f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0x00, 0x00, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xff, 0xf0, 0x0f, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// GPS

void enableGPS(void)
{
    modem.sendAT("+SGPIO=0,4,1,1"); // GPIO4 set HIGH to enable the antenna
    if (modem.waitResponse(10000L) != 1) { // Wait modem response for 1 seconds
        SerialMon.println(F("GPS start fail"));
    }
    if(modem.enableGPS()){ // Send AT command CGNSPWR=1 and return True when em responds with OK
    SerialMon.println("GPS enabled");
    }
}

void disableGPS(void)
{
    modem.sendAT("+SGPIO=0,4,1,0"); // GPIO4 set LOW one finished to prevent power leaking to the antenna
    if (modem.waitResponse(10000L) != 1) { // Wait modem response for 1 seconds
        SerialMon.println(F("GPS stop fail"));
    }
    modem.disableGPS(); // Send AT command CGNSPWR=0 and return True when modem responds with OK
    SerialMon.println(F("GPS disabled"));

}

String getStringValue(String data, char separator, int index) // Splitting a string and return the parts
{
  int strValue = 0;
  String strPart = "";

  for (int i = 0; i < data.length(); i++){
    if (data[i] == separator){
      strValue++;
    }else if (strValue == index){
      strPart.concat(data[i]);
    }else if (strValue > index){
      return strPart;
      break;
    }
  }
  return strPart;
}

void gpsRAW() // Obtain parameters. AT Command Manual - Table 15-1: AT+CGNSINF return Parameters (224 page)
{
  String gps_raw = modem.getGPSraw();

  if (gps_raw != "") {
    String date = getStringValue(gps_raw, ',', 2); // date
    year = date.substring(0, 4).toInt();
    month = date.substring(4, 6).toInt();
    day = date.substring(6, 8).toInt();
    hour = date.substring(8, 10).toInt();
    minutes = date.substring(10, 12).toInt();

    lat = getStringValue(gps_raw, ',', 3).toFloat(); // latitude
    lon = getStringValue(gps_raw, ',', 4).toFloat(); // longitude

    speed_over_ground = getStringValue(gps_raw, ',', 6).toFloat(); // Speed
  }

  Serial.println(F("** GPS data **"));
  Serial.print(F("  lat:")); Serial.println(lat); 
  Serial.print(F("  lon:")); Serial.println(lon);
  Serial.print(F("  speed:")); Serial.print(speed_over_ground); Serial.println(F(" km/h"));
  Serial.print(F("  time&date: ")); Serial.print(hour+2); Serial.print(":"); Serial.print(minutes); Serial.print("  "); Serial.print(day); Serial.print("/"); Serial.print(month); Serial.print("/"); Serial.println(year);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 8);   
  display.print("lat: ");
  display.print(lat);
  display.setCursor(0, 16);
  display.print("lon: ");
  display.print(lon);
  display.setCursor(0, 24);
  display.print("speed: ");
  display.print(speed_over_ground);
  display.print(" km/h");
  //display.display();
  batteryCharge();                   
}

// MODEM

void modemPowerOn()
{
    pinMode(PWR_PIN, OUTPUT);
    digitalWrite(PWR_PIN, LOW);
    delay(1000);    // SIM7000 Hardware Design Datasheet -> Ton mintues = 1s min. (27 page)
    digitalWrite(PWR_PIN, HIGH);
    SerialMon.println(F("Modem power on..."));
}

void modemPowerOff()
{
    pinMode(PWR_PIN, OUTPUT);
    digitalWrite(PWR_PIN, LOW);
    delay(1200);    // SIM7000 Hardware Design Datasheet -> Toff mintues = 1.2s min. (28 page)
    digitalWrite(PWR_PIN, HIGH);
    SerialMon.println(F("Modem power off..."));
}


void modemRestart()
{
    SerialMon.println(F("Modem restarting..."));
    modemPowerOff();
    delay(1000);
    modemPowerOn();
}

// MQTT 

void mqttCallback(char *topic, byte *payload, unsigned int len) // To receive messages
{
    SerialMon.print(F("Message arrived ["));
    SerialMon.print(topic);
    SerialMon.print(F("]: "));
    SerialMon.write(payload, len);
    SerialMon.println();
}

boolean mqttConnect() // Connect to client
{
    SerialMon.print(F("Connecting to... "));
    SerialMon.print(broker);

    // Connect to MQTT Broker
    boolean status = mqtt.connect("TrackerGPS"); 

    if (status == false) {
        SerialMon.println(F(" fail"));
        return false;
    }
    SerialMon.println(F(" success"));
    mqtt.publish(topicInit, "TrackerGPS started"); // Publishes a message to the specified topic.
    return mqtt.connected(); // Checks whether the client is connected to the server.
}

// BATTERY
float mapBatt(float x, float in_min, float in_max, float out_min, float out_max) { // Map voltage (V) - porcentage (%)
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
void batteryCharge() // Checking battery charge
{
  pinMode(A7, INPUT);

  float value_bat = 0.0;
  int samples = 21; // Number of readings to average

  for (int i = 0; i < samples; i++) {  // Averaging to improve accuracy
    value_bat += analogRead(A7);
    delay(100);
  }
  
  value_bat /= samples;
  voltage_bat = ((value_bat / 4096.0) * 2.0 * 3600)/1000; // 12 bits -> [0 to 4095]; voltage divider (equal resistor) -> * 2; V reference -> 3600mV ; Convert mV in V -> / 1000
  perc_bat = mapBatt(voltage_bat, 2.7, 4.2, 0, 100);
  
  if (perc_bat < 0.0) {
    perc_bat = 0;
  } else if (perc_bat > 100.0) {
    perc_bat = 100;
  }

  /* 
  // Battery test
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  */
  display.setCursor(90, 0);
  if (!value_bat){
    display.print("USB");
    Serial.println(F("** Connected to USB **"));
  }else{
  display.print((int)perc_bat);
  display.print(" %");
  Serial.println(F("** Battery level **"));
  Serial.print(F("  Voltage Value = ")); Serial.print(value_bat); Serial.println(" V");
  Serial.print(F("  Voltage = ")); Serial.print(voltage_bat); Serial.println(" V");
  Serial.print(F("  Battery level = ")); Serial.print(perc_bat); Serial.println(" %");
  }
  display.display(); 
  delay(2000);
  
  /*
  // Battery test
  display.setCursor(0, 8);   
  display.print("Value: ");
  display.print(value_bat);
  display.setCursor(0, 16);
  display.print("Voltage: ");
  display.print(voltage_bat);
  display.display(); 
  delay(2000);
  */
}

// DISPLAY

void logo_oled() // Initial logo screen
{
  display.clearDisplay();
  display.drawBitmap(0, 0, logo, 128, 32, WHITE);
  display.display();
  delay(300);
}

void oled_text() // Reset display
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
}

void setup ()
{
  // Serial init
  Serial.begin(UART_BAUD);
  delay(10);
  SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX); // For AT check

  // Oled check
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Turn on SSD1306 and set I2C address.
    Serial.println(F("SSD1306 allocation failed")); 
    for(;;); // Stop execution
  }

  // OLED
  logo_oled();

  // Modem power on
  modemPowerOn();
  
  // Modem init
  SerialMon.println(F("Init modem..."));
  oled_text();
  display.setCursor(0, 0);
  display.print("Init modem...");
  display.display(); 
  if (!modem.init()) { // Restart generally takes longer than modem.init() but ensures the module doesn't have lingering connections
      SerialMon.println(F("Failed to restart modem, attempting to continue without restarting"));
      oled_text();
      display.setCursor(0, 0);
      display.print("Init modem...");
      display.setCursor(0, 10);
      display.print("FAIL");
      display.display(); 
      delay(1000); 
  }else{
    oled_text();
    display.setCursor(0, 0);
    display.print("Init modem...");
    display.setCursor(0, 10);
    display.print("OK");
    display.display(); 
    delay(1000);
  }

  if (GSM_PIN && modem.getSimStatus() != 0) { // Unlock your SIM card with a PIN if needed
      modem.simUnlock(GSM_PIN);
      SerialMon.println(F("SIM... unlock"));
      oled_text();
      display.setCursor(0, 0);
      display.print("SIM...");
      display.setCursor(0, 10);
      display.print("unlock");
      display.display(); 
      delay(1000);
  } else {
    SerialMon.println(F("SIM... lock"));
    oled_text();
    display.setCursor(0, 0);
    display.print("SIM...");
    display.setCursor(0, 10);
    display.print("lock");
    display.display(); 
    delay(200);
  }

  // Modem connect
  SerialMon.print(F("Network..."));
  oled_text();
  display.setCursor(0, 0);
  display.print("Network...");
  display.display(); 
  delay(200); 
  if (!modem.waitForNetwork()) { // Wait for the GSM/GPRS module to connect to the cellular network.
      SerialMon.println(F("not found"));
      oled_text();
      display.setCursor(0, 0);
      display.print("Network...");
      display.setCursor(0, 10);
      display.print("not found");
      display.display(); 
      delay(10000);
      return;
  }else{
      SerialMon.println(F("found"));
      oled_text();
      display.setCursor(0, 0);
      display.print("Network...");
      display.setCursor(0, 10);
      display.print("found");
      display.display(); 
      delay(200);
  }
  
  if (modem.isNetworkConnected()) { // Check if the GSM/GPRS module is connected to the cellular network.
      SerialMon.println(F("Network... connected"));
      oled_text();
      display.setCursor(0, 0);
      display.print("Network...");
      display.setCursor(0, 0);
      display.print("connected");
      display.display(); 
      delay(200); 
  }

  SerialMon.print(F("Connecting to... "));
  SerialMon.print(apn);
  oled_text();
  display.setCursor(0, 0);
  display.print("Connecting to...");
  display.setCursor(0, 10);
  display.print(apn);
  display.display(); 
  delay(200);
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) { // Connects the GSM/GPRS module to the cellular network and establishes a GPRS connection.
      SerialMon.println(F("FAIL"));
      display.setCursor(0, 20);
      display.print(F("FAIL"));
      display.display(); 
      delay(10000);
      return;
  }else{
      SerialMon.println(F("OK"));
      display.setCursor(0, 20);
      display.print(F("OK"));
      display.display();
      delay(200);
  }
  
  if (modem.isGprsConnected()) { // 
      SerialMon.println(F("GPRS... connected"));
      oled_text();
      display.setCursor(0, 0);
      display.print("GPRS...");
      display.setCursor(0, 0);
      display.print("connected");
      display.display(); 
      delay(200);
  }

  // MQTT init
  mqtt.setServer(broker, 1883);     // Create connection to a broker
  mqtt.setCallback(mqttCallback);   // To receive messages
  
}

void loop()
{
  // Modem check
  if (!modem.isNetworkConnected()) { 
      SerialMon.println(F("Network... disconnected"));
      oled_text();
      display.setCursor(0, 0);
      display.print("Network...");
      display.setCursor(0, 10);
      display.print("disconnected");
      display.display(); 
      if (!modem.waitForNetwork(180000L, true)) {
          SerialMon.println(F(" fail"));
          oled_text();
          display.setCursor(0, 0);
          display.print("Network...");
          display.setCursor(0, 10);
          display.print("not found");
          display.display(); 
          delay(10000);
          return;
      }
      if (modem.isNetworkConnected()) {
          SerialMon.println(F("Network... re-connected"));
          oled_text();
          display.setCursor(0, 0);
          display.print("Network...");
          display.setCursor(0, 10);
          display.print("re-connected");
          display.display(); 
          delay(200);
      }

    // and make sure GPRS/EPS is still connected
    if (!modem.isGprsConnected()) {
        SerialMon.println(F("GPRS... disconnected"));
        oled_text();
        display.setCursor(0, 0);
        display.print("GPRS...");
        display.setCursor(0, 10);
        display.print("disconnected");
        display.display(); 
        delay(300);
        SerialMon.print(F("Connecting to... "));
        SerialMon.print(apn);
        oled_text();
        display.setCursor(0, 0);
        display.print("Connecting to...");
        display.setCursor(0, 10);
        display.print(apn);
        display.display(); 
        if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
            SerialMon.println(F("FAIL"));
            display.setCursor(0, 20);
            display.print("FAIL");
            display.display(); 
            delay(10000);
            return;
        }
        if (modem.isGprsConnected()) { // Checks if the GSM/GPRS module is connected to the cellular network and has an established GPRS connection.
            SerialMon.println(F("GPRS... reconnected"));
            display.setCursor(0, 20);
            display.print("reconnected");
            display.display(); 
            delay(300);
        }
    }
  }

  // MQTT check
  if (!mqtt.connected()) {
        SerialMon.println(F("MQTT... disconnected"));
        oled_text();
        display.setCursor(0, 0);
        display.print("MQTT...");
        display.setCursor(0, 10);
        display.print("disconnected");
        display.display(); 
        delay(300);
        uint32_t t = millis();
        if (t - lastReconnectAttempt > 10000L) { // Reconnect every 10 seconds
            lastReconnectAttempt = t;
            if (mqttConnect()) {
                lastReconnectAttempt = 0;
                SerialMon.println(F("MQTT... connected"));
                oled_text();
                display.setCursor(0, 0);
                display.print("MQTT...");
                display.setCursor(0, 10);
                display.print("connected");
                display.display(); 
                delay(300);
            }
        oled_text();
        display.setCursor(0, 0);
        display.print("Signal...");
        display.setCursor(0, 10);
        display.print("processing");
        display.display(); 
        delay(300);
        }
        delay(100);
        return;
    }
  
  if (!modem.testAT()) { // Check if the GSM/GPRS module is working properly and can respond to AT commands.
    Serial.println(F("MODEM... restarting"));
    oled_text();
    display.setCursor(0, 0);
    display.print("MODEM...");
    display.setCursor(0, 10);
    display.print("restarting");
    display.display(); 
    delay(300);
    modemRestart();
    return;
  }
    // GPS 
    enableGPS(); 
  
    while (1) {
      if (modem.getGPS(&lat, &lon)) { // Shows the values ​​once the gps data is obtained
      gpsRAW();
      break; // To exit the while when we have the data
      }
      delay(2000);
    }
     
    // GPS
    disableGPS(); // To save battery
    
    // JSON - MQTT
    DynamicJsonDocument doc(1024); // JSON capacity
    JsonObject obj=doc.as<JsonObject>(); // Create JSON object
    doc["latitude"]=lat;
    doc["longitude"]=lon;
    doc["speed"]=speed_over_ground;
    doc["hour"]=hour;
    doc["minutes"]=minutes;
    doc["day"]=day;
    doc["month"]=month;
    doc["year"]=year;
    doc["voltageBat"]=voltage_bat;
    doc["percBat"]=perc_bat;
    
    char jsonStr[250]; 
    serializeJson(doc,jsonStr); // Convert the json object to a string before sending it
    mqtt.publish(topicTracker,jsonStr); // JSON is sent
    
    mqtt.loop();  // Keeps the MQTT connection active
}
