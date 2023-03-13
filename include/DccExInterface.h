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
#ifdef DCCI_CS
#include <avr/pgmspace.h>
#endif
#include <DCSIlog.h>
#ifndef DCCI_CS
#include <Transport.h>
#endif

#include <DCSIconfig.h>
#include "MsgPacketizer.h"
#include "Queue.h"
#include "Pool.h"

constexpr char protocol01[] PROGMEM = "DccEx";
constexpr char protocol02[] PROGMEM = "WiThrottle";
constexpr char protocol03[] PROGMEM = "Reply";
constexpr char protocol04[] PROGMEM = "Diag";
constexpr char protocol05[] PROGMEM = "Mqtt";
constexpr char protocol06[] PROGMEM = "Http";
constexpr char protocol07[] PROGMEM = "Ctrl";
constexpr char protocol08[] PROGMEM = "Unknown";

constexpr char node01[] PROGMEM = "CommandStation";
constexpr char node02[] PROGMEM = "NetworkProxy";
constexpr char node03[] PROGMEM = "Unknown";

const char *const csProtocolNames[] PROGMEM = {protocol01, protocol02, protocol03, protocol04, protocol05, protocol06, protocol07, protocol08};
const char *const comStationNames[] PROGMEM = {node01, node02, node03};

/**
 * @brief comStation is used to identify the type of participant. In general there shall be only
 * one Network station but there may be multiple 'client' workstations such as the Commandstation
 * in the end this dpends on the number of how many connections are available on the network station
 * side. Today there is only a serial protocol possible ( We assume that there is no WiFi/Ethernet on the
 * command station as this consumes too much ressources on an Uno or Mega)
 */
typedef enum
{
    _DCCSTA,     // CommandStation
    _NWSTA,      // NetworkStation
    _UNKNOWN_STA // Unspecified -> don't know howto handle message
} comStation;
// static_cast enum to int is ok as enum is implemented as int
// static_cast int to enum works as well but invalid enum values will be accepted so needs sanity check
/**
 * @brief "Hardware" elated protocols to comuunicate with a connected MCU. As of now
 * only Serial is supported
 */
typedef enum
{
    _SRL, // serial
    _I2C, // i2c
    _SPI, // spi
    UNKNOWN_COM_PROTOCOL
} comProtocol;

/**
 * @brief the type of command going over the wire. only DCCEX / WITHROTTLE or Contol commands are allowed
 * the networkstation will function as MQTT & HTTP endpoint and only transmit the the DCCEX type commands
 * the API endpoint will translate to DCCEX commands no WITHROTTLE over HTTP and/or MQTT yet
 */
#ifdef DCCI_CS
typedef enum
{
    _DCCEX,             //< > encoded
    _WITHROTTLE,        // Withrottle
    _REPLY,             // Message comming back from the commandstation after the execution of a command; all replys will be forwarded to the originating client
    _DIAG,              // Diagnostic messages comming back from the commandstation
    _MQTT,              // MQTT messages they are handled only on the NW station just like HTTP
    _HTTP,              //  HTTP endpoint
    _CTRL,              // to be set when sending to the CS if the the msg send starts with "<!" avoids the need to do the check on the CS
    UNKNOWN_CS_PROTOCOL // DO NOT remove; used for sizing and testing conditions
} csProtocol;
#endif

#define HANDLERS                                  \
    static void dccexHandler(DccMessage &m);      \
    static void wiThrottleHandler(DccMessage &m); \
    static void notYetHandler(DccMessage &m);     \
    static void replyHandler(DccMessage &m);      \
    static void diagHandler(DccMessage &m);       \
    static void ctrlHandler(DccMessage &m);

#ifndef DCCI_CS
#define HANDLER_INIT                                          \
    const _tcsProtocolHandler handlers[UNKNOWN_CS_PROTOCOL] = \
        {dccexHandler, notYetHandler, ctrlHandler, replyHandler, diagHandler, notYetHandler, notYetHandler};
#else
#define HANDLER_INIT                                          \
    const _tcsProtocolHandler handlers[UNKNOWN_CS_PROTOCOL] = \
        {dccexHandler, notYetHandler, notYetHandler, notYetHandler, notYetHandler, notYetHandler, ctrlHandler};
#endif
//      _DCCEX          _WITHROTTLE,    _REPLY        _DIAG           _MQTT           _HTTP           _CTRL

/**
 * @brief DccMessage is the struct serailazed and send over the
 *        wire to either the command or network station
 *
 */
class DccMessage
{
public:
    int8_t sta;         // station allowed values are comming from the comStation enum
                        // but as msgpack doesn't really work on enums(?)
    int16_t mid;        // message id; sequence number
    int8_t client;      // client id ( socket number from Wifi or Ethernet to be checked if we need to also have the original channel)
    int8_t p;           // either JMRI or WITHROTTLE in order to understand the content of the msg payload
    MsgPack::str_t msg; // going to CS this is a command and a reply on return
    MSGPACK_DEFINE(sta, mid, client, p, msg);

    // function pointer to either the write function(sending the message to the otherside over the com link) or the recieve function(i.e.processing the recieved message)
    using _tcsProcessHandler = void (*)(DccMessage *);
    _tcsProcessHandler process;

    static void copy(DccMessage *dest, DccMessage *src)
    {
        dest->sta = src->sta;
        dest->mid = src->mid;
        dest->client = src->client;
        dest->p = src->p;
        dest->msg = src->msg;
    }
    DccMessage()
    {
        TRC(F("DccMessage created %x" CR), this);
        msg.reserve(MAX_MESSAGE_SIZE); // reserve upfront space; requires that we check that no command exceeds MAX_MESSAGE_SIZE
                                       // avoids heap memory fragmentation and the whole reciev send process runs in constant memory
    }
    ~DccMessage()
    {
        TRC(F("DccMessage deleted %x" CR), this);
    }
};

typedef Pool<DccMessage, MAX_QUEUE_SIZE> _tDccPool;     // set to max queue size but actual use may require less items in the queue
typedef Queue<DccMessage *, MAX_QUEUE_SIZE> _tDccQueue; // only manage pointers to the messages in the pool in the queue

using _tcsProcessHandler = void (*)(DccMessage *m);
using _tcsProtocolHandler = void (*)(DccMessage &m);

typedef enum
{
    IN,
    OUT,
    UNKNOWN_QUEUE_TYPE
} queueType;

class DccExInterface
{
private:
    comStation sta = _UNKNOWN_STA;           // needs to be set at init; defines which side this is running either CS or NW
    comProtocol comp = UNKNOWN_COM_PROTOCOL; // sets the com protocol used between CS and NW; Only serial is supported right now
    HardwareSerial *s;                       // valid only for Serial; this needs to be refacrored into subclasses
                                             // instatated for specific protocols (maybe even at compile tme to reduce size)
    uint32_t speed;
    bool init = false;
    bool blocking = true; // send immediatly & wait for reply from the CS if false all commands
                          // send will be queued and handled in the loop
    uint64_t seq = 0;

    _tDccPool msgPool;   // Message pool to be used by the queues - the queues shall only have the pointers to elements in the pool ( the queue size can be different from the pool size to manage more fine grained mem consumption )
    _tDccQueue msgQueue; // holds the pointers to the messages to be processed the messages hold the pointer to the function which shall process the message

    char decodeBuffer[15]; // buffer used for the decodefunctions

    HANDLERS;
    HANDLER_INIT;

    const uint8_t recv_index = 0x34;
    const uint8_t send_index = 0x12;

    DccExInterface()
    {
        TRC(F("CommandStation Network Proxy created" CR));
    };

    auto _igetPool() -> DccMessage *
    {
        return msgPool.allocate();
    }
    auto _iFreePool(DccMessage *m) -> void
    {
        msgPool.release(m);
    }
    auto _igetMsgQueue() -> _tDccQueue *
    {
        return &msgQueue;        
    }
    auto _isetup(HardwareSerial *s = &Serial1, uint32_t speed = 115200) -> void;
    auto _iSetup(comStation station) -> void
    {
        sta = station; // sets to network or commandstation mode
        _isetup();
    }
    auto _iqueue(uint16_t c, csProtocol p, char *msg) -> void;
    auto _iqueue(DccMessage *m) -> void;
    auto _iRecieve(DccMessage *m) -> void;
    auto _iDecode(csProtocol p) -> const char *;
    auto _iDecode(comStation s) -> const char *;
    auto _iLoop() -> void;
    auto _iSize() -> size_t;
    auto _iWrite(DccMessage *m) -> void; // writes the messages from the outgoing queue to the com protocol endpoint (Serial only
                                         // at this point

public:
    DccExInterface(DccExInterface &other) = delete;
    void operator=(const DccExInterface &) = delete;
    static DccExInterface &GetInstance();

    static auto getMsg() -> DccMessage *
    {
        return (GetInstance()._igetPool());
    }
    static auto releaseMsg(DccMessage *m) -> void
    {
        return (GetInstance()._iFreePool(m));
    }
    static auto getMsgQueue() -> _tDccQueue * { return (GetInstance()._igetMsgQueue()); }
    static auto queue(DccMessage *m) -> void { GetInstance()._iqueue(m); }
    static auto queue(uint16_t c, csProtocol p, char *msg) -> void { GetInstance()._iqueue(c, p, msg); }
    static auto recieve(DccMessage *m) -> void { GetInstance()._iRecieve(m); } // check the transport to see if tere is something for us
    static auto write(DccMessage *m) -> void { GetInstance()._iWrite(m); }     // check the transport to see if tere is something for us to send back
    /**
     * @brief setup the serial interface
     *
     * @param *s        - pointer to a serial port. Default is Serial1 as Serial is used for monitor / upload etc..
     * @param speed     - default serial speed is 115200
     */
    static auto setup(HardwareSerial *s = &Serial1, uint32_t speed = 115200) -> void { GetInstance()._isetup(s, speed); }
    static auto setup(comStation station) -> void { GetInstance()._iSetup(station); }
    static auto loop() -> void { GetInstance()._iLoop(); }
    static auto size() -> size_t { return GetInstance()._iSize(); }
    static auto decode(csProtocol p) -> const char * { return GetInstance()._iDecode(p); }
    static auto decode(comStation s) -> const char * { return GetInstance()._iDecode(s); }
};

#endif