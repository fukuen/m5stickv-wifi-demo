/*--------------------------------------------------------------------
Copyright 2020 fukuen

This M5StickV WiFi demo is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

The Arduino WiFiEsp library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with The Arduino WiFiEsp library.  If not, see
<http://www.gnu.org/licenses/>.
--------------------------------------------------------------------*/

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Wire.h>
#include <WiFiEsp.h>

TFT_eSPI lcd;

#ifdef M5STICKV
#define AXP192_ADDR 0x34
#define PIN_SDA 29
#define PIN_SCL 28
#endif

#define WIDTH 280
#define HEIGHT 240
#define XOFFSET 40
#define YOFFSET 60

char ssid[] = "aterm-a7acc0-g";      // your network SSID (name)
char pass[] = "0c3e7d989e4af";       // your network password
int status = WL_IDLE_STATUS;

char server[] = "arduino.cc";

// Initialize the Ethernet client object
WiFiEspClient client;

char buf[1024];

bool axp192_init() {
  Serial.printf("AXP192 init.\n");
  sysctl_set_power_mode(SYSCTL_POWER_BANK3,SYSCTL_POWER_V33);

  Wire.begin((uint8_t) PIN_SDA, (uint8_t) PIN_SCL, 400000);
  Wire.beginTransmission(AXP192_ADDR);
  int err = Wire.endTransmission();
  if (err) {
    Serial.printf("Power management ic not found.\n");
    return false;
  }
  Serial.printf("AXP192 found.\n");

  // Clear the interrupts
  Wire.beginTransmission(AXP192_ADDR);
  Wire.write(0x46);
  Wire.write(0xFF);
  Wire.endTransmission();
  Wire.beginTransmission(AXP192_ADDR);
  Wire.write(0x23);
  Wire.write(0x08); //K210_VCore(DCDC2) set to 0.9V
  Wire.endTransmission();
  Wire.beginTransmission(AXP192_ADDR);
  Wire.write(0x33);
  Wire.write(0xC1); //190mA Charging Current
  Wire.endTransmission();
  Wire.beginTransmission(AXP192_ADDR);
  Wire.write(0x36);
  Wire.write(0x6C); //4s shutdown
  Wire.endTransmission();
  Wire.beginTransmission(AXP192_ADDR);
  Wire.write(0x91);
  Wire.write(0xF0); //LCD Backlight: GPIO0 3.3V
  Wire.endTransmission();
  Wire.beginTransmission(AXP192_ADDR);
  Wire.write(0x90);
  Wire.write(0x02); //GPIO LDO mode
  Wire.endTransmission();
  Wire.beginTransmission(AXP192_ADDR);
  Wire.write(0x28);
  Wire.write(0xF0); //VDD2.8V net: LDO2 3.3V,  VDD 1.5V net: LDO3 1.8V
  Wire.endTransmission();
  Wire.beginTransmission(AXP192_ADDR);
  Wire.write(0x27);
  Wire.write(0x2C); //VDD1.8V net:  DC-DC3 1.8V
  Wire.endTransmission();
  Wire.beginTransmission(AXP192_ADDR);
  Wire.write(0x12);
  Wire.write(0xFF); //open all power and EXTEN
  Wire.endTransmission();
  Wire.beginTransmission(AXP192_ADDR);
  Wire.write(0x23);
  Wire.write(0x08); //VDD 0.9v net: DC-DC2 0.9V
  Wire.endTransmission();
  Wire.beginTransmission(AXP192_ADDR);
  Wire.write(0x31);
  Wire.write(0x03); //Cutoff voltage 3.2V
  Wire.endTransmission();
  Wire.beginTransmission(AXP192_ADDR);
  Wire.write(0x39);
  Wire.write(0xFC); //Turnoff Temp Protect (Sensor not exist!)
  Wire.endTransmission();

  fpioa_set_function(23, (fpioa_function_t)(FUNC_GPIOHS0 + 26));
  gpiohs_set_drive_mode(26, GPIO_DM_OUTPUT);
  gpiohs_set_pin(26, GPIO_PV_HIGH); //Disable VBUS As Input, BAT->5V Boost->VBUS->Charing Cycle

  msleep(20);
  return true;
}

void printWifiStatus() {
  // print the SSID of the network you're attached to
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength
  long rssi = WiFi.RSSI();
  Serial.print("Signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void setup() {
  Serial.begin(115200);
  axp192_init();

  /* LCD init */
  lcd.begin();
  lcd.setRotation(1);

  lcd.fillRect(0, 0, WIDTH, HEIGHT, TFT_BLACK);
  lcd.setTextFont(0);
  lcd.setTextColor(TFT_WHITE);
  lcd.setCursor(0 + XOFFSET, 0 + YOFFSET);
  lcd.printf("running.");

  Serial1.begin(115200, 34, 35);  // initialize serial for ESP WiFi module
  WiFi.init(&Serial1);    // initialize ESP WiFi module

  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
      Serial.println("WiFi shield not present");
      // don't continue
      while (true);
  }

  // attempt to connect to WiFi network
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(ssid, pass);
  }

  printWifiStatus();

  Serial.println();
  Serial.println("Starting connection to server...");
  // if you get a connection, report back via serial
  if (client.connect(server, 80)) {
    Serial.println("Connected to server");
    // Make a HTTP request
    client.println("GET /asciilogo.txt HTTP/1.1");
    client.println("Host: arduino.cc");
    client.println("Connection: close");
    client.println();
  }
}

void loop() {
  // if there are incoming bytes available
  // from the server, read them and print them
  while (client.available()) {
//    char c = client.read();
//    Serial.write(c);
    int size = client.readBytes(buf, 256);
    Serial.write(buf, size);
  }

  // if the server's disconnected, stop the client
  if (!client.connected()) {
    Serial.println();
    Serial.println("Disconnecting from server...");
    client.stop();

    // do nothing forevermore
    while (true);
  }
}
