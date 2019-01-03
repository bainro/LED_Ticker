/*
Project: Wifi controlled LED matrix display
NodeMCU pins    -> EasyMatrix pins
MOSI-D7-GPIO13  -> DIN
CLK-D5-GPIO14   -> Clk
GPIO0-D3        -> LOAD
*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <SPI.h>
#include <TimeAlarms.h>
#include <Time.h>
#include <stdbool.h>
#include <string.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Max72xxPanel.h>

#define SSID ""               // insert your SSID
#define PASS ""               // insert your password

// ******************* char form to sent to the client-browser ************************************
String form =
  "<!DOCTYPE html style='height: 100%'>"
  "<head><title>ESP8266 App</title><meta name='viewport' content='width=device-width, initial-scale=1'></head>"
  "<body style='margin:0; height: 100%'>"
    "<div style=\"background: radial-gradient(hsl(0, 100%, 27%) 4%, hsl(0, 100%, 18%) 9%, hsla(0, 100%, 20%, 0) 9%) 0 0, radial-gradient(hsl(0, 100%, 27%) 4%, hsl(0, 100%, 18%) 8%, hsla(0, 100%, 20%, 0) 10%) 50px 50px, radial-gradient(hsla(0, 100%, 30%, 0.8) 20%, hsla(0, 100%, 20%, 0)) 50px 0, radial-gradient(hsla(0, 100%, 30%, 0.8) 20%, hsla(0, 100%, 20%, 0)) 0 50px, radial-gradient(hsla(0, 100%, 20%, 1) 35%, hsla(0, 100%, 20%, 0) 60%) 50px 0, radial-gradient(hsla(0, 100%, 20%, 1) 35%, hsla(0, 100%, 20%, 0) 60%) 100px 50px, radial-gradient(hsla(0, 100%, 15%, 0.7), hsla(0, 100%, 20%, 0)) 0 0, radial-gradient(hsla(0, 100%, 15%, 0.7), hsla(0, 100%, 20%, 0)) 50px 50px, linear-gradient(45deg, hsla(0, 100%, 20%, 0) 49%, hsla(0, 100%, 0%, 1) 50%, hsla(0, 100%, 20%, 0) 70%) 0 0, linear-gradient(-45deg, hsla(0, 100%, 20%, 0) 49%, hsla(0, 100%, 0%, 1) 50%, hsla(0, 100%, 20%, 0) 70%) 0 0; background-color: #300; background-size: 100px 100px; color: white; display: flex; height:100%; flex-direction: column; justify-content: center; align-items: center;\">"
      "<h1 style='box-sizing: border-box; margin-bottom: .5em; text-align: center; max-width: 90%; background: #00000050; padding: 1em; margin-top: -2em; text-shadow: 1px 1px 1px #3F51B5, -1px -1px 1px #3F51B5, -1px 1px 1px #3F51B5, -1px 1px 1px #3F51B5; border-radius: 5px;'>ESP8266 Web Server</h1>"
      "<form action='msg' style='margin: 0 auto; text-align: center'>"
        "<p style='margin-bottom: .5em'>Type your message</p>"
        "<input type='text' name='msg' autofocus style='display: block; margin-bottom:2em;'>" 
        "<label for='weather'>Weather</label>"
        "<input type='checkbox' name='weather' value='true' style='margin-bottom:2em;'>" 
        "<input type='submit' value='Submit' style='display: block; margin: 0 auto; border-radius: 6px; cursor: pointer'>"
      "</form>"
    "</div>"
  "</body>"
  "</html>";

ESP8266WebServer server(80);  // HTTP server will listen at port 80
int pinCS = 0; // Attach CS to this pin, DIN to MOSI and CLK to SCK (cf http://arduino.cc/en/Reference/SPI )
Max72xxPanel matrix = Max72xxPanel(pinCS, 8, 1);
HTTPClient http;
long period;
int offset=1, refresh=0;
String decodedMsg;
bool showWeather = 1;
String tape = "Arduino";
int wait = 20; // In milliseconds. Affects scroll speed
int spacer = 1;
int width = 5 + spacer; // The font width is 5 pixels

/*
  handles the messages coming from the webbrowser, restores a few special characters and 
  constructs the chars that can be sent to the LED display
*/
void handle_msg() {

  matrix.fillScreen(LOW);
  // Send same page so they can send another msg
  server.send(200, "text/html", form);    
  refresh=1;
  String msg = server.arg("msg");
  decodedMsg = msg;
  decodedMsg.replace("+", " ");      
  decodedMsg.replace("%21", "!");  
  decodedMsg.replace("%22", "");  
  decodedMsg.replace("%23", "#");
  decodedMsg.replace("%24", "$");
  decodedMsg.replace("%25", "%");  
  decodedMsg.replace("%26", "&");
  decodedMsg.replace("%27", "'");  
  decodedMsg.replace("%28", "(");
  decodedMsg.replace("%29", ")");
  decodedMsg.replace("%2A", "*");
  decodedMsg.replace("%2B", "+");  
  decodedMsg.replace("%2C", ",");  
  decodedMsg.replace("%2F", "/");   
  decodedMsg.replace("%3A", ":");    
  decodedMsg.replace("%3B", ";");  
  decodedMsg.replace("%3C", "<");  
  decodedMsg.replace("%3D", "=");  
  decodedMsg.replace("%3E", ">");
  decodedMsg.replace("%3F", "?");  
  decodedMsg.replace("%40", "@"); 

  if (server.arg("weather") == "true") {
    showWeather = 1;
    updateWeather();
  }
  else {
    showWeather = 0;
  }

  return;
  
}

void setup(void) {
  matrix.setIntensity(15); // Use a value between 0 and 15 for brightness
  setTime(6, 50, 55, 13, 3, 2018);

  matrix.setRotation(0, 3);
  matrix.setRotation(1, 3);
  matrix.setRotation(2, 3);
  matrix.setRotation(3, 3);
  matrix.setRotation(4, 3);
  matrix.setRotation(5, 3);
  matrix.setRotation(6, 3);
  matrix.setRotation(7, 3);
  matrix.setPosition(0, 7, 0); // The first display is at <0, 7> 
  matrix.setPosition(1, 6, 0); // setPosition(byte display, byte x, byte y)
  matrix.setPosition(2, 5, 0); 
  matrix.setPosition(3, 4, 0); 
  matrix.setPosition(4, 3, 0); 
  matrix.setPosition(5, 2, 0); 
  matrix.setPosition(6, 1, 0);
  matrix.setPosition(7, 0, 0);
                               
  WiFi.begin(SSID, PASS);                         // Connect to WiFi network
  while (WiFi.status() != WL_CONNECTED) {         // Wait for connection
    delay(500);
  }
  
  // Set up the endpoints for HTTP server
  server.on("/", []() {
    server.send(200, "text/html", form);
  });
  server.on("/msg", handle_msg);
  server.begin();                                 // Start the server     
  
  Alarm.timerRepeat(0, 10, 0, updateWeather);
  updateWeather();

}


void loop(void) {

  Alarm.delay(0); //required for timers
  int hour_int = hour() + 1;
  int min_int = minute();
  char hour_str[3];
  char min_str[3];
  
  if (hour_int >= 13) {
    hour_int -= 12;
  }

  itoa(hour_int, hour_str, 10);
  itoa(min_int, min_str, 10);
  
  if (min_str[1] == '\0') {
    min_str[1] = min_str[0];
    min_str[0] = '0';
    min_str[2] = '\0';
  }

  matrix.fillScreen(LOW);

  //loop to scroll message across right side and show time on left
  for ( int i = 0 ; i < width * decodedMsg.length() + matrix.width() - 20 - spacer; i++ ) {
    server.handleClient();                        // checks for incoming messages
    if (refresh==1) i=0;
    refresh=0;

    int letter = i / width;
    int x = (matrix.width() - 1) - i % width;
 
    while ( x + width - spacer >= 20 && letter >= 0 ) {
      if ( letter < decodedMsg.length() ) {
        matrix.drawChar(x, 0, decodedMsg[letter], HIGH, LOW, 1);
      }
      letter--;
      x -= width; 
    }
    
    //Draw letters in black to allow natural scrolling on split screen
    matrix.drawChar(15, 0, 3, LOW, LOW, 1);
    matrix.drawChar(15, 0, 0, LOW, LOW, 1);
    matrix.drawChar(15, 0, 1, LOW, LOW, 1);
    matrix.drawChar(15, 0, 8, LOW, LOW, 1);

    /*
    display time on left 3 displays
    shift digits to the left when hour < 10
    and give the extra room to the right side
    */
  
    if (hour_int >= 10) {
      matrix.drawChar(-1, 0, '1', HIGH, LOW, 1); 
      matrix.drawChar(4, 0, hour_str[1], HIGH, LOW, 1);
      matrix.drawPixel(10, 2, HIGH);
      matrix.drawPixel(10, 4, HIGH);
      matrix.drawChar(12, 0, min_str[0], HIGH, LOW, 1);
      matrix.drawChar(18, 0, min_str[1], HIGH, LOW, 1);
    }
    else {
      matrix.drawChar(0, 0, hour_str[0], HIGH, LOW, 1);
      matrix.drawPixel(6, 2, HIGH);
      matrix.drawPixel(6, 4, HIGH);
      matrix.drawChar(8, 0, min_str[0], HIGH, LOW, 1);
      matrix.drawChar(14, 0, min_str[1], HIGH, LOW, 1);
    }
    
    matrix.write(); // Send bitmap to display

    delay(wait);
  }
}

void updateWeather () {

  if (showWeather) {

    //Eugene city ID for openweathermap.org api: 4992263
    //api key: 879dd7eae0cae1164644721a0e6ac6b9

    http.begin("http://api.openweathermap.org/data/2.5/weather?lat=44&lon=-123&APPID=879dd7eae0cae1164644721a0e6ac6b9");

    /* Left to implement */
    //Present what is forecasted to occur in the next 3 hrs (ie clear skies, cloudy, moderate rain, etc)
      //just grab the first 3hr response in the /forecast JSON data
    //Concat high & low for next 8 3hr periods
      //pull 1st 8 3hr forecasts, calc high and low 
  
    int httpCode = http.GET();

    if (httpCode == 200) {
      const size_t bufferSize = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(1) + 2*JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(6) + JSON_OBJECT_SIZE(12) + 390;
      StaticJsonBuffer<1000> jsonBuffer;
      JsonObject& root = jsonBuffer.parseObject(http.getString());
      JsonObject& weather = root["weather"][0];
      JsonObject& main = root["main"];
      String weather_description = weather["description"]; // "clear sky"
      float main_temp_float = main["temp"]; // 271.57
      main_temp_float = main_temp_float  * (9.0/5.0) - 459.67;
      char main_temp[7];
      int main_temp_int = static_cast<int>(main_temp_float);
      itoa(main_temp_int, main_temp, 10);
      String main_humidity = main["humidity"]; 
      String wind_speed = root["wind"]["speed"]; 
      decodedMsg = weather_description +  "    Temp:" + main_temp + "F   Humidity:" + main_humidity + "%   Wind:" + wind_speed + "mph";
    } 
    else {
      char code_str[5];
      char error[40] = "http status code current weather:";
      itoa(httpCode, code_str, 10);
      strcat(error, code_str);
      decodedMsg = error;
    }

    http.end();

  }
    
}


