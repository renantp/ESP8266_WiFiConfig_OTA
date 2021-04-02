//=========Biến chứa mã HTLM Website==//
#ifndef HTML_H
#define HTML_H
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <EEPROM.h>
#include <ESP8266httpUpdate.h>
#include <ArduinoOTA.h>
#define LED_PIN 2
#define AP_SSID "Renan Wifi Config"
#define AP_PASS "iotvision@2021"
#define STA_SSID "Lespoir"
#define STA_PASS "lespoirxinchao"
#define URL_TIME_API "http://worldtimeapi.org/api/timezone/Asia/Ho_Chi_Minh"
const char MainPage[] PROGMEM = R"=====(
  <!DOCTYPE html> 
  <html>
   <head> 
       <title>WIFI CONFIG</title> 
       <style> 
          body {text-align:center;}
          h1 {color:#003399;}
          a {text-decoration: none;color:#FFFFFF;}
          .bt_off {height:50px; width:100px; margin:10px 0;background-color:#FF6600;border-radius:5px;}
          .bt_on {height:50px; width:100px; margin:10px 0;background-color:#00FF00;border-radius:5px;}
          .bt_write {height:30px; width:70px; margin:10px 0;background-color:#00FF00;border-radius:5px;}
          .bt_restart {height:30px; width:70px; margin:10px 0;background-color:#FF6600;border-radius:5px;}
          .bt_clear {height:30px; width:70px; margin:10px 0;background-color:#00FF00;border-radius:5px;}
          .bt_update {height:50px; width:90px; margin:10px 0;background-color:#3273DB;border-radius:5px;}
          .left_p {text-align: left; margin-left: 10px; color:DodgerBlue;}
          .center_p {text-align: center; margin-left: 10px; color:DodgerBlue;}
       </style>
       <meta name="viewport" content="width=device-width,user-scalable=0" charset="UTF-8">
   </head>
   <body> 
      <h1>IoTVision Web Server</h1> 
      <div>Trạng thái trên chân LED: <b <pan id="trangthaiLed"><pan></b></div>
       <div>
        <button class="bt_on" onclick="getdata('onLed')">ON</button>
        <button class="bt_off" onclick="getdata('offLed')">OFF</button>
      </div>
      <div>Trạng thái trên chân GPIO5: <b <pan id="trangthai5"><pan></b></div>
       <div>
        <button class="bt_on" onclick="getdata('on5')">ON</button>
        <button class="bt_off" onclick="getdata('off5')">OFF</button>
      </div>
      <div>Trạng thái trên chân GPIO4: <b <pan id="trangthai4"><pan></b></div>
       <div>
        <button class="bt_on" onclick="getdata('on4')">ON</button>
        <button class="bt_off" onclick="getdata('off4')">OFF</button>
      </div>
      <div>Dòng điện: <b <pan id="acs712"><pan></b></div>
      <div>Thiết lập Wifi</div>
      <div class = "center_p">Tên Wifi: <input id="ssid"/></div>
      <div class = "center_p">MK Wifi: <input id="pass"/></div>
      <div class = "center_p">Offset: <input id="offset" type="number" name="offset" min="0" max="30"/></div>
      <div class = "center_p">Scale: <input id="scale" type="number" name="scale" min="1" max="225"/></div>
      <div>
        <button class="bt_write" onclick="writeEEPROM()">KẾT NỐI</button>
        <button class="bt_write" onclick="scanWifi()">SCAN</button>
        <button class="bt_write" onclick="offset()">OS&SC</button>
        <button class="bt_restart" onclick="restartESP()">RESTART</button>
        <button class="bt_clear" onclick="clearEEPROM()">XOÁ</button>
      </div>
      <div><button class="bt_update" onclick="updateOTA()">UPDATE FIRMWARE</div>
      <div id>Offset: <pan id="offsetnow"></pan> Scale: <pan id="scalenow"></pan></div>
      <div>Temp: <pan id="temp"></pan></div><div> Humi: <pan id="humi"></pan></div>
      <div id>IP connected: <pan id="ipconnected"></pan></div>
      <div id>Time: <pan id="timeapi"></pan></div>
      <div id="reponsetext"></div>
      <script>
        //-----------Tạo đối tượng request----------------
        function create_obj(){
          td = navigator.appName;
          if(td == "Microsoft Internet Explorer"){
            obj = new ActiveXObject("Microsoft.XMLHTTP");
          }else{
            obj = new XMLHttpRequest();
          }
          return obj;
        }
        var xhttp = create_obj();
        //------------------------------------------------
        window.onload = function(){
          xhttp.open("GET","/getIP",true);
          xhttp.onreadystatechange = process_ip;//nhận reponse 
          xhttp.send();
        }
        //-----------Kiểm tra response IP------------------
        function process_ip(){
          if(xhttp.readyState == 4 && xhttp.status == 200){
            //------Updat data sử dụng javascript----------
            ketqua = JSON.parse(xhttp.responseText); 
            document.getElementById("ipconnected").innerHTML=ketqua.ip;
            document.getElementById("offset").innerHTML=ketqua.offset;
            document.getElementById("scale").innerHTML=ketqua.scale;
            document.getElementById("scalenow").innerHTML=ketqua.scale;
            document.getElementById("offsetnow").innerHTML=ketqua.offset;
            document.getElementById("timeapi").innerHTML=ketqua.datetime;
            document.getElementById("ssid").innerHTML=ketqua.ssid;
            document.getElementById("pass").innerHTML=ketqua.pass;
          }
        }
        //==============================================================
        //-----------Thiết lập dữ liệu và gửi request ON/OFF D4---
        function getdata(url){
          xhttp.open("GET","/"+url,true);
          xhttp.onreadystatechange = process_onofLed;//nhận reponse 
          xhttp.send();
        }
        //-----------Kiểm tra response ON/OFF D4------------------
        function process_onofLed(){
          if(xhttp.readyState == 4 && xhttp.status == 200){
            //------Updat data sử dụng javascript----------
            ketqua = JSON.parse(xhttp.responseText); 
            document.getElementById("trangthaiLed").innerHTML=ketqua.led;   
            document.getElementById("trangthai5").innerHTML=ketqua.gpio5;
            document.getElementById("trangthai4").innerHTML=ketqua.gpio4;
          }
        }
         //-----------Thiết lập dữ liệu và gửi request Status D4----
        //---Ham update duu lieu tu dong---
        var xhttp2 = create_obj();
        setInterval(function() {
          getstatusLed();
        }, 450);// 300ms van OK
        function getstatusLed(){
          xhttp2.open("GET","/getstatusLed",true);
          xhttp2.onreadystatechange = process_onofLed2;//nhận reponse 
          xhttp2.send();
        }
        function process_onofLed2(){
          if(xhttp2.readyState == 4 && xhttp.status == 200){
            //------Updat data sử dụng javascript----------
            ketqua = JSON.parse(xhttp2.responseText); 
            document.getElementById("trangthaiLed").innerHTML=ketqua.led;
            document.getElementById("trangthai5").innerHTML=ketqua.gpio5;    
            document.getElementById("trangthai4").innerHTML=ketqua.gpio4;
            document.getElementById("acs712").innerHTML=ketqua.acs;
            document.getElementById("timeapi").innerHTML=ketqua.datetime;
            document.getElementById("temp").innerHTML=ketqua.temp;
            document.getElementById("humi").innerHTML=ketqua.humi;
          }
        }
        //===============================================================
        //-----------Thiết lập dữ liệu và gửi request ssid và password---
        function writeEEPROM(){
          var ssid = document.getElementById("ssid").value;
          var pass = document.getElementById("pass").value;
          xhttp.open("GET","/writeEEPROM?ssid="+ssid+"&pass="+pass,true);
          xhttp.onreadystatechange = process_reponse;//nhận reponse 
          xhttp.send();
        }
        function offset(){
          var os = document.getElementById("offset").value;
          var sc = document.getElementById("scale").value;
          xhttp.open("GET","/offset?offset="+os+"&scale="+sc,true);
          xhttp.onreadystatechange = function(){
            if(xhttp.readyState == 4 && xhttp.status == 200){
              //------Updat data sử dụng javascript----------
              ketqua = JSON.parse(xhttp.responseText); 
              document.getElementById("offsetnow").innerHTML=ketqua.offset;
              document.getElementById("scalenow").innerHTML=ketqua.scale;       
            }
          };
          xhttp.send();
        }
        function clearEEPROM(){
          xhttp.open("GET","/clearEEPROM",true);
          xhttp.onreadystatechange = process_reponse;//nhận reponse 
          xhttp.send();
        }
        function restartESP(){
          xhttp.open("GET","/restartESP",true);
          xhttp.onreadystatechange = process_reponse;//nhận reponse 
          xhttp.send();
        }
        function scanWifi(){
          xhttp.open("GET","/scanWifi",true);
          xhttp.onreadystatechange = process_reponse;//nhận reponse 
          xhttp.send();
        }
        function updateOTA(){
          xhttp.open("GET","/OTA",true);
          xhttp.onreadystatechange = process_reponse;//nhận reponse 
          xhttp.send();
        }
        //-----------Kiểm tra response ------------------
        function process_reponse(){
          if(xhttp.readyState == 4 && xhttp.status == 200){
            //------Updat data sử dụng javascript----------
            ketqua = xhttp.responseText; 
            document.getElementById("reponsetext").innerHTML=ketqua;       
          }
        }
      </script>
   </body> 
  </html>
)=====";

#endif