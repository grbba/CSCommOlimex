/**
 *  © 2023 Gregor Baues. All rights reserved.
 *  
 *  This is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  It is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with CommandStation.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef dccex_interface_h
#define dccex_interface_h

#include <Arduino.h>
#include <DCSIlog.h>
#ifndef DCCI_CS
#include <Transport.h>
#endif
#include <DCSIconfig.h>
#include "MsgPacketizer.h"
#include "Queue.h"

/**
 * @brief comStation is used to identify the type of participant. In general there shall be only
 * one Network station but there may be multiple 'client' workstations such as the Commandstation
 * in the end this dpends on the number of how many connections are available on the network station
 * side. Today there is only a serial protocol possible ( We assume that there is no WiFi/Ethernet on the
 * command station as this consumes too much ressources on an Uno or Mega)
 */
typedef enum
{
    _DCCSTA,        // CommandStation
    _NWSTA,         // NetworkStation
    _UNKNOWN_STA    // Unspecified -> don't know howto handle message
} comStation;
// static_cast enum to int is ok as enum is implemented as int
// static_cast int to enum works as well but invalid enum values will be accepted so needs sanity check
/**
 * @brief "Hardware" elated protocols to comuunicate with a connected MCU. As of now 
 * only Serial is supported
 */
typedef enum
{
    _SRL,        // serial
    _I2C,        // i2c
    _SPI,        // spi
    _UNKNOWN_COM_PROTOCOL
} comProtocol;
/**
 * @brief the type of command going over the wire. only DCCEX / WITHROTTLE or Contol commands are allowed
 * the networkstation will function as MQTT & HTTP endpoint and only transmit the the DCCEX type commands
 * the API endpoint will translate to DCCEX commands no WITHROTTLE over HTTP and/or MQTT yet
 */
#ifdef DCCI_CS
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
#endif
#define HANDLERS  \
    static void dccexHandler(DccMessage m); \
    static void wiThrottleHandler(DccMessage m); \
    static void notYetHandler(DccMessage m); \
    static void replyHandler(DccMessage m); \
    static void diagHandler(DccMessage m); \
    static void ctrlHandler(DccMessage m);

#ifndef DCCI_CS
#define HANDLER_INIT  \
   _tcsProtocolHandler handlers[UNKNOWN_CS_PROTOCOL] = \
   {    dccexHandler,   notYetHandler,  replyHandler,   diagHandler,    notYetHandler, notYetHandler, ctrlHandler}; 
#else
#define HANDLER_INIT  \
   _tcsProtocolHandler handlers[UNKNOWN_CS_PROTOCOL] = \
   {    dccexHandler,   notYetHandler,  notYetHandler,  notYetHandler,  notYetHandler,  notYetHandler,  ctrlHandler}; 
#endif
//      _DCCEX          _WITHROTTLE,    _REPLY        _DIAG           _MQTT           _HTTP           _CTRL

/**
 * @brief DccMessage is the struct serailazed and send over the 
 *        wire to either the command or network station
 * 
 */
class DccMessage {
public:
    int sta;                    // station allowed values are comming from the comStation enum 
                                // but as msgpack doesn't really work on enums(?) 
    int mid;                    // message id; sequence number 
    int client;                 // client id ( socket number from Wifi or Ethernet to be checked if we need to also have the original channel)
    int p;                      // either JMRI or WITHROTTLE in order to understand the content of the msg payload
    MsgPack::str_t msg;         // going to CS this is a command and a reply on return
    MSGPACK_DEFINE(sta, mid, client, p, msg);

    DccMessage() {
        msg.reserve(MAX_MESSAGE_SIZE);        // reserve upfront space; requires that we check that no command exceeds MAX_MESSAGE_SIZE
                                              // avoids heap memory fragmentation and the whole reciev send process runs in constant memory  
    }
}; 

typedef Queue<DccMessage, MAX_QUEUE_SIZE> _tDccQueue;
using  _tcsProtocolHandler = void (*)(DccMessage m);

typedef enum
{
    IN,      
    OUT,      
    UNKNOWN_QUEUE_TYPE
} queueType;

class DccExInterface
{
private:
    comStation      sta = _UNKNOWN_STA;               // needs to be set at init; defines which side this is running either CS or NW
    comProtocol     comp = _UNKNOWN_COM_PROTOCOL;      // sets the com protocol used between CS and NW; Only serial is supported right now 
    HardwareSerial  *s;                               // valid only for Serial; this needs to be refacrored into subclasses 
                                                      // instatated for specific protocols (maybe even at compile tme to reduce size)
    uint32_t        speed;                           
    bool            init = false;
    bool            blocking = true;                  // send immediatly & wait for reply from the CS if false all commands 
                                                      // send will be queued and handled in the loop 
    uint64_t        seq = 0;
    _tDccQueue      *incomming = nullptr;             // incomming queue holding message to be processed
    _tDccQueue      *outgoing = nullptr;              // outgoing queue holding message to be send 

    void write();                                     // writes the messages from the outgoing queue to the com protocol endpoint (Serial only
                                                      // at this point
    const char* csProtocolNames[8] = {"DCCEX", "WTH", "REPLY", "DIAG", "MQTT" , "HTTP", "CTRL", "UNKNOWN"};   //TODO move that to Progmem
    const char* comStationNames[3] = {"CommandStation","NetworkStation","Unknown"};
    
    HANDLERS;
    HANDLER_INIT;

public:
    const uint8_t recv_index = 0x34;
    const uint8_t send_index = 0x12;

    auto getQueue(queueType q) -> _tDccQueue* {
        switch(q) {
         case IN : return incomming; break;
         case OUT: return outgoing; break;
         default : ERR(F("Unknown queue type returning null")); return nullptr;
        }
    }
    /**
     * @brief pushes a DccMessage struct into the designated queue
     * 
     * @param q : incomming or outgoing queue 
     * @param packet : DccMessage struct to be pushed and send over the wire 
     */
    void queue(queueType q, csProtocol p, DccMessage packet);
    void queue(uint16_t c, csProtocol p, char *msg);
    void recieve();        // check the transport to see if tere is something for us
    /**
     * @brief setup the serial interface 
     * 
     * @param *s        - pointer to a serial port. Default is Serial1 as Serial is used for monitor / upload etc..
     * @param speed     - default serial speed is 115200
     */
    void setup(HardwareSerial *s = &Serial1, uint32_t speed = 115200);
    void setup(comStation station) {
        sta = station;                  // sets to network or commandstation mode
        setup();
    }
    void loop();
    auto size(queueType inout) -> size_t {
        if (inout == IN) {
            return incomming->size();
        }
        if (inout == OUT) {
            return outgoing->size();
        }
        ERR(F("Unknown queue in size; specifiy either IN or OUT"));
        return 0;
    }
    auto decode(csProtocol p) -> const char *;
    auto decode(comStation s) -> const char *;

    DccExInterface(); 
    ~DccExInterface();
};

extern DccExInterface DCCI;
#endif