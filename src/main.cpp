/*
 * © 2020 Gregor Baues. All rights reserved.
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
#include <DIAG.h>

#include "DccExInterface.h"
#include "freeMemory.h"
#include "NetworkInterface.h"

// (0) Declare NetworkInterfaces
NetworkInterface nwi1;
NetworkInterface nwi2;

// (1) Declare CommandstationInterface. no error checking for multiple of those yet here can only be one maybe two 
// in the future if multipe serial ports may be possible to create // connections if the com is getting the bottleneck
// DccExInterface _idccex;



// (1) Start NetworkInterface - HTTP callback

void httpRequestHandler(ParsedRequest *req, Client* client) {
  INFO(F("\nParsed Request:"));
  INFO(F("\nMethod:         [%s]"), req->method);
  INFO(F("\nURI:            [%s]"), req->uri);
  INFO(F("\nHTTP version:   [%s]"), req->version);
  INFO(F("\nParameter count:[%d]\n"), *req->paramCount);
}

// (1) End NetworkInterface - HTTP callback



void setup()
{
  delay(2000);
  Log.begin(LOG_LEVEL_TRACE, &Serial, false); // Start logging subsystem
  Serial.begin(115200);                       // Start the serial connection for the Serial monitor / uploads etc ...

  INFO(F("DCC++ EX NetworkInterface Standalone" CR));

  // setup the serial (or other connection ) to the MEGA
  // start the serial manager by providing the HW Serial port other than Serial all by iself
  // assumes that the mega is wired up to the ESP32 over a level shifter 

  INFO(F("Opening serial connection to the CommandStation ..." CR));

  // create the connection to the Command station
  DCCI.setup(_NWSTA);  // set up as Network station just use the default values


  // open the connection to the "outside world" over Ethernet (cabled) or WiFi (wireless) 

  // nwi1.setup(ETHERNET, UDPR);                    // ETHERNET/UDP on Port 2560 
  // nwi2.setup(ETHERNET, UDPR, 8888);              // ETHERNET/UDP on Port 8888 
  nwi1.setup(ETHERNET, TCP);                        // ETHERNET/TCP on Port 2560 
  // nwi2.setup(ETHERNET, TCP, 23);                 // ETHERNET/TCP on Port 23 for the CLI
  // nwi1.setup(ETHERNET, TCP, 8888);               // ETHERNET/TCP on Port 8888
  // nwi2.setup(WIFI, TCP);                         // WIFI/TCP on Port 2560
  // nwi1.setHttpCallback(httpRequestHandler);      // HTTP callback

  INFO(F("Network Setup done ...\n"));
  INFO(F("Free RAM after network init: [%d]\n"),freeMemory());

  // printStats();

  // (2) End starting NetworkInterface

}

bool done = false;
DccMessage m;
void doOnce(HardwareSerial *sp) {
    if(!done) {
      Serial.println("Sending test message to CS ...");
      MsgPack::str_t s = MsgPack::str_t("test");
        m.client = 10;
        m.mid = 101;
        m.p = 1;
        m.msg = s;
        MsgPacketizer::send(*sp, 0x34, m);
        done = true;
    }
}

void loop()
{

doOnce(&Serial1); // send a test message to see if we have at leasta serial connection

// Handle all the incomming/outgoing messages for the active interfaces
// incomming : from the network to the ComStation and to the Network
NetworkInterface::loop();

// incomming messages will be queued for the DccExInterface to be consumed
// forward the incomming commands to whomever is needed
// e.g. config commands for the commandstation will be handled locally
// other commands like <s> will be send to the Command station

DCCI.loop();

  
// Optionally report any decrease in memory (will automatically trigger on first call)
#if ENABLE_FREE_MEM_WARNING
  static int ramLowWatermark = 32767; // replaced on first loop 

  int freeNow = freeMemory();
  if (freeNow < ramLowWatermark)
  {
    ramLowWatermark = freeNow;
    LCD(2,F("Free RAM=%5db"), ramLowWatermark);
  }
#endif
}