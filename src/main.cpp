/**
 * © 2020,2023 Gregor Baues. All rights reserved.
 *  
 * This is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the 
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 * 
 * See the GNU General Public License for more details <https://www.gnu.org/licenses/>
 */

#include <Arduino.h>

#include <NetworkInterface.h>
#include <DccExInterface.h>
#include <DCSIlog.h>
#include <DCSIconfig.h>
#include <DCSIDisplay.h>

#include "freeMemory.h"


void displayIntro();

// (0) Declare NetworkInterfaces
NetworkInterface nwi1;
NetworkInterface nwi2;

// (1) Start NetworkInterface - HTTP callback

// void httpRequestHandler(ParsedRequest *req, Client* client) {
//   INFO(F("\nParsed Request:"));
//   INFO(F("\nMethod:         [%s]"), req->method);
//   INFO(F("\nURI:            [%s]"), req->uri);
//   INFO(F("\nHTTP version:   [%s]"), req->version);
//   INFO(F("\nParameter count:[%d]\n"), *req->paramCount);
// }

// (1) End NetworkInterface - HTTP callback

void setup()
{
  Serial.begin(115200);   
  delay(2000);
  dccLog.begin(LOG_LEVEL_TRACE, &Serial, false); // Start logging subsystem

  // Serial.println("I am alive ...");
                              
  display.setup();  
  displayIntro();


  INFO(F("DCC++ EX NetworkInterface Standalone" CR));

  // setup the serial (or other connection ) to the MEGA
  // start the serial manager by providing the HW Serial port other than Serial all by iself
  // assumes that the mega is wired up to the ESP32 over a level shifter 

  INFO(F("Opening Connection to the CommandStation ..." CR));
  // create the connection to the Command station
  DccExInterface::setup(_NWSTA);  // set up as Network station just use the default values


  // open the connection to the "outside world" over Ethernet (cabled) or WiFi (wireless) 
  // nwi1.setup(ETHERNET, UDPR);                    // ETHERNET/UDP on Port 2560 
  // nwi2.setup(ETHERNET, UDPR, 8888);              // ETHERNET/UDP on Port 8888 
  nwi1.setup(ETHERNET, TCP);                        // ETHERNET/TCP on Port 2560 
  // nwi2.setup(ETHERNET, TCP, 23);                 // ETHERNET/TCP on Port 23 for the CLI
  // nwi1.setup(ETHERNET, TCP, 8888);               // ETHERNET/TCP on Port 8888
  // nwi2.setup(WIFI, TCP);                            // WIFI/TCP on Port 2560
  // nwi1.setHttpCallback(httpRequestHandler);      // HTTP callback

  INFO(F("Network Setup done ...\n"));
  INFO(F("Free RAM after network init: [%d]\n"),freeMemory());

  Log.begin(LOG_LEVEL_TRACE, &Serial, false);     // TODO: Don't know why yet thos has to be done here again ... otherwise the output gets corrupted
  TRC(F("Logging %s %x %x" CR), Log.getLogOutput() == &Serial ? "OK" : "NOK", Log.getLogOutput(), &Serial); 

}

// bool done = false;
// DccMessage m;
// void doOnce(HardwareSerial *sp) {
//     if(!done) {
//       Serial.println("Sending test message to CS ...");
//       MsgPack::str_t s = MsgPack::str_t("test");
//         m.client = 10;
//         m.mid = 101;
//         m.p = 1;
//         m.msg = s;
//         m.sta = _NWSTA;
//         MsgPacketizer::send(*sp, 0x34, m);
//         done = true;
//     }
// }

void loop()
{

// doOnce(&Serial1); // send a test message to see if we have at leasta serial connection

// Handle all the incomming/outgoing messages for the active interfaces
// incomming : from the network to the ComStation and to the Network
  NetworkInterface::loop();

// incomming messages will be queued for the DccExInterface to be consumed
// forward the incomming commands to whomever is needed
// e.g. config commands for the commandstation will be handled locally
// other commands like <s> will be send to the Command station

  DccExInterface::loop();
  // display.loop();
}

#define MAJOR 1
#define MINOR 0
#define PATCH 4

void displayIntro() {

    char month[4];
    int day, year, hour, min, sec;
    sscanf(__DATE__, "%s %i %i", &month[0], &day, &year);
    sscanf(__TIME__, "%i:%i:%i", &hour, &min, &sec);

  display.screen.print("\nDCC++ EX Network Interface\n");
  display.screen.printf("Version %d.%d.%d" , MAJOR, MINOR, PATCH);
  display.screen.printf("-%d%d%d\n(c) 2023 grbba\n\n" , day, hour, min);
}