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

#ifndef Transport_h
#define Transport_h

#include <Arduino.h>
#include <ETH.h>
#include <WiFi.h>

#include "NetworkConfig.h"
#include "NetworkInterface.h"
// #include "DccExInterface.h"


// on the ESP32 there is no disticinction of both

typedef WiFiServer EthernetServer;
typedef WiFiUDP EthernetUDP;
typedef WiFiClient EthernetClient;

typedef enum
{
    _DCCEX,          //< > encoded
    _WITHROTTLE,     // Withrottle 
    _REPLY,          // Message comming back from the commandstation after the execution of a command; all replys will be forwarded to the originating client
    _DIAG,           // Diagnostic messages comming back from the commandstation
    _MQTT,           // MQTT messages they are handled only on the NW station just like HTTP
    _HTTP,           //  HTTP endpoint
    _CTRL,           // to be set when sending to the CS if the the msg send starts with "<!" avoids the need to do the check on the CS
    UNKNOWN_CS_PROTOCOL  // DO NOT remove; used for sizing and testing conditions
} csProtocol;

// Needed forward declarations
struct Connection;
class TransportProcessor;

using appProtocolCallback = void (*)(Connection* c, TransportProcessor* t);

struct Connection
{
    uint8_t id;                             // initalized when the pool is setup
// MAYBE AN ISSUE it is strange that the ethernet and WiFi are all managed across the same objects ....
    WiFiClient *client;                     // idem ... WiFiClient is used for all types of connections This was Client in short on the Arduino mega
// ENDISSUE
    char overflow[MAX_OVERFLOW];            // idem
    csProtocol p;                           // dynamically determined upon message reception; first message wins
    char start_delimiter = '\0';            // start end delimiters such as < > for various protocols
    char end_delimiter = '\0';              // idem
    bool isProtocolDefined = false;         // idem
    appProtocolCallback appProtocolHandler; // idem
};

/**
 * @brief The Transport class instatiates a either a Ethernet or WiFi abstraction level. This can be serial as well 
 * but would need to be developmed in a Serial management class such as ETH.h or WiFi.h
 * 
 * @tparam S 
 * @tparam C 
 * @tparam U 
 */
template <class S, class C, class U> class Transport: public AbstractTransport
{

private:
    C                   clients[MAX_SOCK_NUM];          // Client objects created by the connectionPool
    Connection          connections[MAX_SOCK_NUM];      // All the connections build by the connectionPool
    byte                active = 0;                     // number of currently active connections (we may have wifi or eth setup but no client connected)
    bool                connected = false;              // Transport is setup        
    TransportProcessor* t;                              // pointer to the object which handles the incomming/outgoing flow

    void udpHandler(U* udp);                            // Reads from a Udp socket - todo add incomming queue for processing when the flow is faster than we can process commands
    void tcpSessionHandler(S* server);                  // tcpSessionHandler -> connections are maintained open until close by the client
    void connectionPool(S* server);                     // allocates the Sockets at setup time and creates the Connections
    void connectionPool(U* udp);                        // allocates the UDP Sockets at setup time and creates the Connection
   
public:

    uint8_t         id;
    uint16_t        port;
    uint8_t         protocol;               // TCP or UDP  
    uint8_t         transport;              // WIFI or ETHERNET 
    S*              server;                 // WiFiServer or EthernetServer 
    U*              udp;                    // UDP socket object
    uint8_t         maxConnections;         // number of supported connections depending on the network equipment use

    bool setup(NetworkInterface* nwi);      // we get the callbacks from the NetworkInterface 
    void loop(); 
    C getClient(int c) {
        return clients[c];
    }

    bool isConnected() {
        return connected;
    }
    byte getActive() {
        return active;
    }

    Transport<S,C,U>();
    ~Transport<S,C,U>();
    
};

typedef Transport<EthernetServer,EthernetClient,EthernetUDP>  EthernetTransport;
typedef Transport<WiFiServer, WiFiClient, WiFiUDP> WiFiTransport;

#endif // !Transport_h