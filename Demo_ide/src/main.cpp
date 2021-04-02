#include <html.h>
#include <acs_config.h>
#include <Arduino_JSON.h>
#include <SHT31.h>
#define SHT31_ADDRESS 0x44
// #define CLOUD
#define EEPROM_LED 511
#define EEPROM_GPIO5 413
#define EEPROM_GPIO4 414
#define EEPROM_SSID 415
#define EEPROM_PASS 447
#define EEPROM_OFFSET 412
#define EEPROM_SCALE 411
#define VERSION "This from divice, v1.0.1"
ESP8266WebServer server(80);
IPAddress ip_ap(192, 168, 1, 1);
IPAddress gateway_ap(192, 168, 1, 1);
IPAddress subnet_ap(255, 255, 255, 0);
HTTPClient http;
String json_get;
int ledStatus;
String ssid;
String pass;
uint32_t ui32_counter = 0;
JSONVar myJSON;
JSONVar js_get;
int offset = 5;
int _scale = 110;
const char *host = "https://renanblog.tk";
const char *url = "/firmware.bin";
long lastDBCheck = 0;
SHT31 sht;
bool getRTCJson(void);
void mainpage();
void off_LED();
void on_gpio_4();
void off_gpio_4();
void on_gpio_5();
void off_gpio_5();
void on_LED();
void get_statusLED();
void checkConnection();
boolean readWifi_EEPROM();
void write_EEPROM();
void clear_EEPROM();
void scanWifi();
void get_IP();
void restart_ESP();
void offset_rp();
//--------
void checkTimer();
void FalseTheFlag();
double ProcessIrms();
//--------
int DBcount;
void OTA()
{
  WiFiClient client;
  if (client.connect("http://renanblog.tk", 80))
  {
    Serial.println("connection failed");
    return;
  }
  else
  {
    Serial.println("Connected!!");
    server.send(200, "text/html", "Đã kết nối! Nếu thành công sẽ tự restart");
  }
  ESPhttpUpdate.setLedPin(2, LOW);
  server.close();
  t_httpUpdate_return ret = ESPhttpUpdate.update(client, "http://renanblog.tk/firmware.bin");
  switch (ret)
  {
  case HTTP_UPDATE_FAILED:
    server.begin();
    Serial.println("Update failed.");
    break;
  case HTTP_UPDATE_NO_UPDATES:
    Serial.println("Update no Update.");
    break;
  case HTTP_UPDATE_OK:
    Serial.println("Update ok."); // may not be called since we reboot the ESP
    break;
  }
}
// WiFiServer server(80);
void setup()
{
  // put your setup code here, to run once:
  Wire.begin();
  sht.begin(SHT31_ADDRESS, 13, 2);
  Wire.setClock(3400000);
  pinMode(5, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(0, INPUT);
  Serial.begin(115200);
  EEPROM.begin(512); //Khởi tạo bộ nhớ EEPROM
  delay(10);
#ifdef CLOUD
  Serial.println(VERSION);
#endif
  if (readWifi_EEPROM())
  {
    checkConnection();
  }
  else
  {
    // WiFi.disconnect();
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAPConfig(ip_ap, gateway_ap, subnet_ap);
    WiFi.softAP(AP_SSID, AP_PASS);
    Serial.println("Soft Access Point mode!");
    Serial.print("Please connect to ");
    Serial.println(AP_SSID);
    Serial.print("Web Server IP Address: ");
    Serial.println(ip_ap);
  }
  server.on("/", mainpage);
  server.on("/onLed", on_LED);
  server.on("/offLed", off_LED);
  server.on("/on5", on_gpio_5);
  server.on("/off5", off_gpio_5);
  server.on("/on4", on_gpio_4);
  server.on("/off4", off_gpio_4);
  server.on("/getstatusLed", get_statusLED); //Trình duyệt request mỗi 1000ms
  server.on("/getIP", get_IP);
  server.on("/writeEEPROM", write_EEPROM);
  server.on("/offset", offset_rp);
  server.on("/restartESP", restart_ESP);
  server.on("/clearEEPROM", clear_EEPROM);
  server.on("/scanWifi", scanWifi);
  server.on("/OTA", OTA);
  server.begin();
}
void loop()
{
  // put your main code here, to run repeatedly:
  server.handleClient();
  long currentMillis = millis();
  if (currentMillis - lastDBCheck > 100)
  {
    if (!digitalRead(0))
    {
      DBcount++;
      if (DBcount >= 50)
      {
        DBcount = 0;
        if (WiFi.status() == WL_CONNECTED)
        {
          Serial.println("Begin to OTA");
          // HTTPClient http;
          // WiFiClient * tcp = http.getStreamPtr();
          // Serial.println(&tcp);
          OTA();
        }
        else
        {
          // WiFi.disconnect();
          WiFi.mode(WIFI_AP_STA);
          WiFi.softAPConfig(ip_ap, gateway_ap, subnet_ap);
          WiFi.softAP(AP_SSID, AP_PASS);
          Serial.println("Soft Access Point mode!");
          Serial.print("Please connect to ");
          Serial.println(AP_SSID);
          Serial.print("Web Server IP Address: ");
          Serial.println(ip_ap);
        }
      }
    }
    else
    {
      if (DBcount >= 2)
      {
        DBcount = 0;
        // WiFi.disconnect();
        WiFi.mode(WIFI_AP_STA);
        WiFi.softAPConfig(ip_ap, gateway_ap, subnet_ap);
        WiFi.softAP(AP_SSID, AP_PASS);
        Serial.println("Soft Access Point mode!");
        Serial.print("Please connect to ");
        Serial.println(AP_SSID);
        Serial.print("Web Server IP Address: ");
        Serial.println(ip_ap);
      }
    }
    lastDBCheck = millis();
  }
  checkTimer();
  if (Flag.doProcess)
  {
    Peak = ProcessIrms() + offset;
  }
  if (Peak < 0)
    Peak = 0;
  if (Flag.t1s)
  {
    Flag.doProcess = 1;
    sht.read();
    Serial.print("\t");
    Serial.print(sht.getTemperature(), 1);
    Serial.print("\t");
    Serial.println(sht.getHumidity(), 1);
    StartTimer.tprocess = millis();
  }
  if (Flag.t5s)
  {
    getRTCJson();
  }
  FalseTheFlag();
}
String prepareHtmlPage()
{
  String htmlPage;
  htmlPage.reserve(1024); // prevent ram fragmentation
  htmlPage = F("HTTP/1.1 200 OK\r\n"
               "Content-Type: text/html\r\n"
               "Connection: close\r\n" // the connection will be closed after completion of the response
               "Refresh: 5\r\n"        // refresh the page automatically every 5 sec
               "\r\n"
               "<!DOCTYPE HTML>"
               "<html>"
               "Analog input:  ");
  htmlPage += analogRead(A0);
  htmlPage += F("</html>"
                "\r\n");
  return htmlPage;
}
void mainpage()
{
  String s = FPSTR(MainPage);
  server.send(200, "text/html", s);
}
void off_LED()
{
  digitalWrite(LED_PIN, HIGH);
  EEPROM.write(EEPROM_LED, 0);
  EEPROM.commit();
  myJSON["led"] = 0;
  Serial.println(JSON.stringify(myJSON));
  server.send(200, "text/html", JSON.stringify(myJSON));
}
void offset_rp()
{
  String ofs = server.arg("offset");
  String scs = server.arg("scale");
  char *chr1, *chr2;
  chr1 = &ofs[0];
  chr2 = &scs[0];
  sscanf(chr1, "%i", &offset);
  sscanf(chr2, "%i", &_scale);
  EEPROM.write(EEPROM_SCALE, _scale);
  EEPROM.write(EEPROM_OFFSET, offset);
  EEPROM.commit();
  JSONVar js;
  js["offset"] = offset;
  js["scale"] = _scale;
  server.send(200, "text/html", JSON.stringify(js));
}
void on_LED()
{
  digitalWrite(LED_PIN, LOW);
  EEPROM.write(EEPROM_LED, 1);
  EEPROM.commit();
  myJSON["led"] = 1;
  Serial.println(JSON.stringify(myJSON));
  server.send(200, "text/html", JSON.stringify(myJSON));
}
void off_gpio_5()
{
  digitalWrite(5, LOW);
  EEPROM.write(EEPROM_GPIO5, 0);
  EEPROM.commit();
  myJSON["gpio5"] = 0;
  Serial.print("GPIO5 ---->");
  Serial.println(JSON.stringify(myJSON));
  server.send(200, "text/html", JSON.stringify(myJSON));
}
void on_gpio_5()
{
  digitalWrite(5, HIGH);
  EEPROM.write(EEPROM_GPIO5, 1);
  EEPROM.commit();
  myJSON["gpio5"] = 1;
  Serial.print("GPIO5 ---->");
  Serial.println(JSON.stringify(myJSON));
  server.send(200, "text/html", JSON.stringify(myJSON));
}
void off_gpio_4()
{
  digitalWrite(4, LOW);
  EEPROM.write(EEPROM_GPIO4, 0);
  EEPROM.commit();
  myJSON["gpio4"] = 0;
  Serial.print("GPIO4 ---->");
  Serial.println(JSON.stringify(myJSON));
  server.send(200, "text/html", JSON.stringify(myJSON));
}
void on_gpio_4()
{
  digitalWrite(4, HIGH);
  EEPROM.write(EEPROM_GPIO4, 1);
  EEPROM.commit();
  myJSON["gpio4"] = 1;
  Serial.print("GPIO4 ---->");
  Serial.println(JSON.stringify(myJSON));
  server.send(200, "text/html", JSON.stringify(myJSON));
}
void get_statusLED()
{

  ledStatus = myJSON["led"];
  bool gpio5 = myJSON["gpio5"];
  bool gpio4 = myJSON["gpio4"];
  if (digitalRead(5) != gpio5)
  {
    EEPROM.write(EEPROM_GPIO5, digitalRead(5));
    myJSON["gpio5"] = digitalRead(5);
  }
  if (digitalRead(4) != gpio4)
  {
    EEPROM.write(EEPROM_GPIO4, digitalRead(4));
    myJSON["gpio4"] = digitalRead(4);
  }
  // if (digitalRead(LED_PIN) != ledStatus)
  // {
  //   Serial.println(JSON.stringify(myJSON));
  //   EEPROM.write(EEPROM_LED, !digitalRead(LED_PIN));
  //   myJSON["led"] = digitalRead(LED_PIN);
  // }
  EEPROM.commit();
  myJSON["datetime"] = "Error!!";
  myJSON["datetime"] = JSON.stringify(js_get["datetime"]);
  if (sht.getTemperature() > 100)
  {
    myJSON["humi"] = "n/a (SDA: GPIO13 (xanh lá))";
    myJSON["temp"] = "n/a (SCK: GPIO2 (vàng))";
  }
  else
  {
    myJSON["humi"] = sht.getHumidity();
    myJSON["temp"] = sht.getTemperature();
  }
  //Serial.println(JSON.stringify(myJSON));
  server.send(200, "text/html", JSON.stringify(myJSON));
}
void get_IP()
{
  JSONVar js;
  js["ip"] = WiFi.localIP().toString();
  js["offset"] = offset;
  js["scale"] = _scale;
  js["datetime"] = JSON.stringify(js_get["datetime"]);
  js["ssid"] = ssid;
  js["pass"] = pass;
  // Serial.println(js["datetime"]);
  //Serial.println(JSON.stringify(js));
  server.send(200, "text/html", JSON.stringify(js));
}
//---------------SETUP WIFI------------------------------
void checkConnection()
{
  Serial.println();
  Serial.print("Check connecting to ");
  Serial.println(ssid);
  WiFi.disconnect();
  WiFi.begin(ssid, pass);
  int count = 0;
  while (count < 100)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      WiFi.mode(WIFI_AP_STA);
      Serial.println();
      Serial.print("Connected to ");
      Serial.println(ssid);
      Serial.print("Web Server IP Address: ");
      Serial.println(WiFi.localIP());
      WiFi.softAPConfig(ip_ap, gateway_ap, subnet_ap);
      WiFi.softAP(AP_SSID, AP_PASS);
      Serial.println("Connected!");
      Serial.print("Please connect to ");
      Serial.println(AP_SSID);
      Serial.print("Web Server IP Address: ");
      Serial.println(ip_ap);
      return;
    }
    delay(100);
    Serial.print(".");
    count++;
  }
  Serial.println("Timed out.");
  WiFi.disconnect();
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(ip_ap, gateway_ap, subnet_ap);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.println("Soft Access Point mode!");
  Serial.print("Please connect to ");
  Serial.println(AP_SSID);
  Serial.print("Web Server IP Address: ");
  Serial.println(ip_ap);
  json_get = "{\"time\": \"Could not connect to server!!\"}";
}
//------------------Read/Write/Clear EEPROM Wifi
boolean readWifi_EEPROM()
{
  Serial.println("Reading EEPROM...");
  // ledStatus = char(EEPROM.read(EEPROM_LED));
  if (EEPROM.read(EEPROM_SSID) != 0)
  {
    myJSON["offset"] = EEPROM.read(EEPROM_OFFSET);
    offset = myJSON["offset"];
    offset = (offset < 0) ? 5 : offset;
    myJSON["scale"] = EEPROM.read(EEPROM_SCALE);
    _scale = myJSON["scale"];
    myJSON["led"] = EEPROM.read(EEPROM_LED);
    myJSON["gpio5"] = EEPROM.read(EEPROM_GPIO5);
    myJSON["gpio4"] = EEPROM.read(EEPROM_GPIO4);
    ssid = "";
    pass = "";
    for (uint16_t i = EEPROM_SSID; i < EEPROM_PASS; ++i)
    {
      ssid += char(EEPROM.read(i));
    }
    Serial.print("SSID: ");
    Serial.println(ssid);
    for (uint16_t i = EEPROM_PASS; i < 511; ++i)
    {
      pass += char(EEPROM.read(i));
    }
    Serial.print("PASSWORD: ");
    Serial.println(pass);
    ssid = ssid.c_str();
    pass = pass.c_str();
    Serial.print("First EEPROM read---->");
    Serial.println(JSON.stringify(myJSON));
    ledStatus = myJSON["led"];
    int gpio5, gpio4;
    gpio5 = myJSON["gpio5"];
    gpio4 = myJSON["gpio4"];
    Serial.printf("%i%i%i\r\n", ledStatus, gpio5, gpio4);
    Serial.print(myJSON["led"]);
    Serial.print(myJSON["gpio5"]);
    Serial.print(myJSON["gpio4"]);
    Serial.print("Trạng thái chân LED: ");
    if (ledStatus == false)
    {
      digitalWrite(LED_PIN, LOW);
      Serial.println("LOW (ON)");
    }
    else
    {
      digitalWrite(LED_PIN, HIGH);
      Serial.println("HIGH (OFF)");
    }
    Serial.print("Trạng thái chân GPIO5: ");
    if (gpio5 == 0)
    {
      digitalWrite(5, LOW);
      Serial.println("LOW (OFF)");
    }
    else
    {
      digitalWrite(5, HIGH);
      Serial.println("HIGH (ON)");
    }
    Serial.print("Trạng thái chân GPIO4: ");
    if (gpio4 == 0)
    {
      digitalWrite(4, LOW);
      Serial.println("LOW (OFF)");
    }
    else
    {
      digitalWrite(4, HIGH);
      Serial.println("HIGH (ON)");
    }

    return true;
  }
  else
  {
    Serial.println("Data wifi not found!");
    return false;
  }
}
void write_EEPROM()
{
  ssid = server.arg("ssid");
  pass = server.arg("pass");
  Serial.println("Clear EEPROM!");
  for (uint16_t i = EEPROM_SSID; i < 511; ++i)
  {
    EEPROM.write(i, 0);
    delay(10);
  }
  for (uint16_t i = 0; i < ssid.length(); ++i)
  {
    EEPROM.write(EEPROM_SSID + i, ssid[i]);
  }
  for (uint16_t i = 0; i < pass.length(); ++i)
  {
    EEPROM.write(EEPROM_PASS + i, pass[i]);
  }
  EEPROM.commit();
  Serial.println("Writed to EEPROM!");
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("PASS: ");
  Serial.println(pass);
  String s = "Đã lưu thông tin wifi";
  WiFi.begin(ssid, pass);
  server.send(200, "text/html", s);
}
void clear_EEPROM()
{
  Serial.println("Clear EEPROM!");
  for (int i = 416; i < 511; ++i)
  {
    EEPROM.write(i, 0);
    delay(10);
  }
  EEPROM.commit();
  String s = "Đã xóa bộ nhớ EEPROM";
  server.send(200, "text/html", s);
}
//------------------Scan/Resart ESP---------------------------
void restart_ESP()
{
  String s = "Đã khởi động lại ESP8266";
  server.send(200, "text/html", s);
  ESP.restart();
}
void scanWifi()
{
  int n = WiFi.scanNetworks();
  String s = "<div><b>Danh sách quét được</b></div>";
  for (int i = 0; i < n; i++)
  {
    s += "<p class = \"center_p\" onClick = \"document.getElementById('ssid').value = '";
    s += WiFi.SSID(i);
    s += "'\"><u>";
    s += WiFi.SSID(i);
    s += " ";
    // s+= WiFi.channel(i); s+= " ";
    s += WiFi.RSSI(i) * (-1);
    s += "% ";
    s += WiFi.encryptionType(i) == ENC_TYPE_NONE ? " -> Open" : "";
    s += "</u></p>";
  }
  s += "<p>";
  s += VERSION;
  s += "</p>";
  server.send(200, "text/html", s);
}
//-------------------------Read ACS712-------------------------------------
double ProcessIrms()
{
  if (millis() - StartTimer.tprocess < timeprocess) //150ms
  {
    int readValue = analogRead(analogInPin);
    maxValue = (readValue > maxValue) ? readValue : maxValue;
    minValue = (readValue < minValue) ? readValue : minValue;
  }
  else
  {
    Vpp = (maxValue - minValue) - offset;
    Vpp = (Vpp < 0) ? 0 - Vpp : Vpp;
    VppOld = (Vpp >= 0) ? Vpp : VppOld;
    double VppSmooth = (Vpp >= 0) ? (VppOld + (Vpp - VppOld) * 1) : VppSmooth; //Thay đổi hệ số nhân
    Serial.print("Max: ");
    Serial.println(maxValue);
    Serial.print("Min: ");
    Serial.println(minValue);
    _Ireal = double(round(((Vpp / _scale) * 100)) / 100);
    maxValue = 0;
    minValue = 525;
    Flag.doProcess = 0;
    Serial.print("Max-Min: ");
    Serial.println(Vpp);
    //double _Ireal = double(round(((offset1/110)*100))/100);
    Serial.print("---------->I: ");
    Serial.println(_Ireal);
    myJSON["acs"] = _Ireal;
  }
  return Vpp;
}
void FalseTheFlag()
{
  Flag.t100ms = false;
  Flag.t250ms = false;
  Flag.t500ms = false;
  Flag.t1s = false;
  Flag.t5s = false;
  Flag.t10s = false;
}
void checkTimer()
{
  if ((millis() - StartTimer.t100ms) >= 100)
  {
    Flag.t100ms = true;
    StartTimer.t100ms = millis();
  }
  if ((millis() - StartTimer.t250ms) >= 250)
  {
    Flag.t250ms = true;
    StartTimer.t250ms = millis();
  }
  if ((millis() - StartTimer.t500ms) >= 500)
  {
    Flag.t500ms = true;
    StartTimer.t500ms = millis();
  }
  if ((millis() - StartTimer.t1s) >= 1000)
  {
    Flag.t1s = true;
    StartTimer.t1s = millis();
  }
  if ((millis() - StartTimer.t5s) >= 5000)
  {
    Flag.t5s = true;
    StartTimer.t5s = millis();
  }
  if ((millis() - StartTimer.t10s) >= 10000)
  {
    Flag.t10s = true;
    StartTimer.t10s = millis();
  }
}
bool getRTCJson(void)
{
  int httpCodeGet;
  //String _url = ;
  http.begin(URL_TIME_API);                           //Specify request destination
  http.addHeader("Content-Type", "application/json"); //Specify content-type header
  httpCodeGet = http.GET();                           //Recever the request
  Serial.println("Getting......");
  if (httpCodeGet >= 200 && httpCodeGet < 250)
  {
    json_get = http.getString();
    js_get = JSON.parse(json_get);
    //Serial.println(json_get);
    //    json_get.toCharArray(input, json_get.length());
    //    delay(1000);
    //    Serial.println(input);
    //    myObject = JSON.parse(input);
    //    if (JSON.typeof(myObject) == "undefined") {
    //      Serial.println("Parsing input failed!");
    //    }
    return 1;
  }
  return 0;
}