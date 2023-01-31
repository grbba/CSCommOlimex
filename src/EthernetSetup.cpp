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
#include <DCSIlog.h>
#include <WiFi.h>

#include "EthernetSetup.h"

#define Ethernet ETH    // rename externally provided Class instance)

static bool eth_connected = false;
/**
 * @brief event handler inserted at the setup stage
 * 
 * @param event 
 */
void WiFiEvent(WiFiEvent_t event)
{
  switch (event) {
    case SYSTEM_EVENT_ETH_START:
      Serial.println("ETH Started");
      //set eth hostname here
      ETH.setHostname("esp32-ethernet");
      break;
    case SYSTEM_EVENT_ETH_CONNECTED:
      Serial.println("ETH Connected");
      break;
    case SYSTEM_EVENT_ETH_GOT_IP:
      Serial.print("ETH MAC: ");
      Serial.print(ETH.macAddress());
      Serial.print(", IPv4: ");
      Serial.print(ETH.localIP());
      if (ETH.fullDuplex()) {
        Serial.print(", FULL_DUPLEX");
      }
      Serial.print(", ");
      Serial.print(ETH.linkSpeed());
      Serial.println("Mbps");
      eth_connected = true;
      break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      eth_connected = false;
      break;
    case SYSTEM_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      eth_connected = false;
      break;
    default:
      break;
  }
}

byte EthernetSetup::setup() {
    
    TRC("Ethernet Setup::setup() ... " CR);

    WiFi.onEvent(WiFiEvent); // doesn't seem to have any effect on the Ethernet side
    // check for the WiFi if that does someting 
    
    if (!Ethernet.begin()) {
      ERR("Ethernet did not start" CR);
    } 
    Ethernet.linkUp();
    Ethernet.fullDuplex();
  
// check below on all sorts of error conditions ...
    INFO(F("Starting server on Ethernet connection ..." CR));
    server = new EthernetServer(port);
    server->begin();
    server->available();
    connected = true;
    maxConnections = MAX_SOCK_NUM;
  
    if (connected)
    {
        ip = Ethernet.localIP();
        INFO(F("Local IP address:      [%d.%d.%d.%d]" CR), ip[0], ip[1], ip[2], ip[3]);
        INFO(F("Listening on port:     [%d]" CR), port);
        dnsip = Ethernet.dnsIP(); 
        INFO(F("DNS server IP address: [%d.%d.%d.%d] " CR), dnsip[0], dnsip[1], dnsip[2], dnsip[3]);
        INFO(F("Number of connections: [%d]" CR), maxConnections);
        return true; 
    }
    return false; // something went wrong
}

void EthernetSetup::print() {
   Log.trace("EthernetSetup::server: %x" CR, server);
   Log.trace("EthernetSetup::udp: %x" CR, udp);
}

// byte EthernetSetup::setup() 
// {
//     INFO(F("Initialize Ethernet with DHCP"));
//     if (Ethernet.begin() == 0)
//     {
//         WARN(F("Failed to configure Ethernet using DHCP ... Trying with fixed IP"));
//         Ethernet.begin(IPAddress(IP_ADDRESS)); // default ip address

//         if (Ethernet.hardwareStatus() == EthernetNoHardware)
//         {
//             ERR(F("Ethernet shield was not found. Sorry, can't run without hardware. :("));
//             return 0;
//         };
//         if (Ethernet.linkStatus() == LinkOFF)
//         {
//             ERR(F("Ethernet cable is not connected."));
//             return 0;
//         }
//     }

//     maxConnections = MAX_SOCK_NUM;

//     if (Ethernet.hardwareStatus() == EthernetW5100)
//     {
//         INFO(F("W5100 Ethernet controller detected."));
//         maxConnections = 4;  // Max supported officaly by the W5100 but i have been running over 8 as well. Perf has to be evaluated though comparing 4 vs. 8 connections
//     }
//     else if (Ethernet.hardwareStatus() == EthernetW5200)
//     {
//         INFO(F("W5200 Ethernet controller detected."));
//         maxConnections = 8;
//     }
//     else if (Ethernet.hardwareStatus() == EthernetW5500)
//     {
//         INFO(F("W5500 Ethernet controller detected."));
//         maxConnections = 8;
//     }

//    INFO(F("Network Protocol:      [%s]"), protocol ? "UDP" : "TCP");
//     switch (protocol)
//     {
//         case UDPR:
//         { 
//             udp = new EthernetUDP();
//             byte udpState = udp->begin(port);
//             if (udpState) 
//             {
//                 TRC(F("UDP status: %d"), udpState);
//                 maxConnections = 1;             // there is only one UDP object listening for incomming data
//                 connected = true;
//             }
//             else
//             {
//                 ERR(F("UDP failed to start"));
//                 connected = false;
//             }
//             break;
//         };
//         case TCP:
//         {
//             server = new EthernetServer(port);
//             server->begin();
//             connected = true;
//             break;
//         };
//         case MQTT:
//         {
//             // do the MQTT setup stuff ...
//         };
//         default:
//         {
//             ERR(F("\nUnkown Ethernet protocol; Setup failed"));
//             connected = false;
//             break;
//         }
//     }
//     if (connected)
//     {
//         ip = Ethernet.localIP();
//         INFO(F("Local IP address:      [%d.%d.%d.%d]"), ip[0], ip[1], ip[2], ip[3]);
//         INFO(F("Listening on port:     [%d]"), port);
//         dnsip = Ethernet.dnsIP(); 
//         INFO(F("DNS server IP address: [%d.%d.%d.%d] "), dnsip[0], dnsip[1], dnsip[2], dnsip[3]);
//         INFO(F("Number of connections: [%d]"), maxConnections);
//         return true; 
//     }
//     return false; // something went wrong
// }

EthernetSetup::EthernetSetup() {}
EthernetSetup::EthernetSetup(uint16_t p, protocolType pt ) { port = p; protocol = pt; }
EthernetSetup::~EthernetSetup() {}

