/*********
  (Origin from Rui Santos)

  Instructions:
    1. Add your wifi credentials in "WiFiCredentials.h"
    2. Flash the device
    3. Open serial monitor and check the IP of the device
    4. Open your browser and go to the specified IP

  Bruce Helsen:
    8 Oct 2019:
      - Changed the code to blink
      - Removed one of the two outputs
      - Remaining output is attached to the onboard LED
      - Added a watchdog timer in case something goes wrong
      - Current state will be stored in EEPROM, so if something goes wrong the module restarts and sets the same output as before.

*********/

// Load Wi-Fi library
#include "WiFiCredentials.h"
#include <WiFi.h>
#include "EEPROM.h"
#include "esp_system.h"

const int wdtTimeout = 3000;  //time in ms to trigger the watchdog
hw_timer_t *timer = NULL;

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String output2State = "off";
int outputState = 0;

// Assign output variables to GPIO pins
const int output2 = 2;


// the current address in the EEPROM (i.e. which byte
// we're going to write to next)
int addr = 0;
#define EEPROM_SIZE 1

void IRAM_ATTR resetModule() {
  ets_printf("reboot\n");
  esp_restart();
}

void setup() {

  if (!EEPROM.begin(EEPROM_SIZE))
  {
    Serial.println("failed to initialise EEPROM"); delay(1000000);
  }
  Serial.println(" bytes read from Flash . Values are:");
  for (int i = 0; i < EEPROM_SIZE; i++)
  {
    outputState = byte(EEPROM.read(i));
    Serial.print(outputState); Serial.print(" ");
  }


  timer = timerBegin(0, 80, true);                  //timer 0, div 80
  timerAttachInterrupt(timer, &resetModule, true);  //attach callback
  timerAlarmWrite(timer, wdtTimeout * 1000, false); //set time in us
  timerAlarmEnable(timer);                          //enable interrupt

  Serial.begin(115200);
  // Initialize the output variables as outputs
  pinMode(output2, OUTPUT);

  // Set outputs to LOW
  digitalWrite(output2, LOW);


  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

double lastTime = 0;

void loop() {
  yield();
  while (WiFi.status() == WL_CONNECTED) {

    timerWrite(timer, 0); //reset timer (feed watchdog)
    WiFiClient client = server.available();   // Listen for incoming clients
    //if (output2State == "on") {
    if (outputState) {
      if ((millis() - lastTime) > 100) {
        digitalWrite(output2, !digitalRead(output2));
        lastTime = millis();
      }
    }

    if (client) {                             // If a new client connects,
      Serial.println("New Client.");          // print a message out in the serial port
      String currentLine = "";                // make a String to hold incoming data from the client
      while (client.connected()) {            // loop while the client's connected
        if (client.available()) {             // if there's bytes to read from the client,
          char c = client.read();             // read a byte, then
          Serial.write(c);                    // print it out the serial monitor
          header += c;

          if (c == '\n') {                    // if the byte is a newline character
            // if the current line is blank, you got two newline characters in a row.
            // that's the end of the client HTTP request, so send a response:
            if (currentLine.length() == 0) {
              sendHTTPHeader(client);
              // turns the GPIOs on and off
              if (header.indexOf("GET /2/on") >= 0) {
                Serial.println("GPIO 2 on");
                output2State = "on";
                outputState = 1;
                digitalWrite(output2, HIGH);
              } else if (header.indexOf("GET /2/off") >= 0) {
                Serial.println("GPIO 2 off");
                output2State = "off";
                outputState = 0;
                digitalWrite(output2, LOW);
              }
              EEPROM.write(0, outputState);
               EEPROM.commit();
              sendHTML(client);

              // Break out of the while loop
              break;
            } else { // if you got a newline, then clear currentLine
              currentLine = "";
            }
          } else if (c != '\r') {  // if you got anything else but a carriage return character,
            currentLine += c;      // add it to the end of the currentLine
          }
        }
      }

      // Clear the header variable
      header = "";

      // Close the connection
      client.stop();

      Serial.println("Client disconnected.");
      Serial.println("");
    }
  }
}

void sendHTTPHeader(WiFiClient client) {
  // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
  // and a content-type so the client knows what's coming, then a blank line:
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println("Connection: close");
  client.println();
}
void sendHTML(WiFiClient client) {
  // Display the HTML web page
  client.println("<!DOCTYPE html><html>");
  client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
  client.println("<link rel=\"icon\" href=\"data:,\">");
  // CSS to style the on/off buttons
  // Feel free to change the background-color and font-size attributes to fit your preferences
  client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
  client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
  client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
  client.println(".button2 {background-color: #555555;}</style></head>");

  // Web Page Heading
  client.println("<body><h1>ESP32 Web Server</h1>");

  // Display current state, and ON/OFF buttons for GPIO 2
  // client.println("<p>GPIO 2 - State " + output2State + "</p>");
  client.println("<p>GPIO 2 - State " + String(outputState) + "</p>");

  if (outputState == 0) {
    client.println("<p><a href=\"/2/on\"><button class=\"button\">ON</button></a></p>");
  } else {
    client.println("<p><a href=\"/2/off\"><button class=\"button button2\">OFF</button></a></p>");
  }
  client.println("</body></html>");

  // The HTTP response ends with another blank line
  client.println();
}
