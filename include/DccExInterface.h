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
#include <DIAG.h>
#include "MsgPacketizer.h"

#ifdef ARDUINO_ARCH_ESP32
    #define NW_COM
#endif
#ifdef ARDUINO_AVR_MEGA2560
    #define CS_COM
#endif

#define MAX_MESSAGE_SIZE 128

#ifdef NW_COM
    #include <etl/queue.h>
    #define MAX_QUEUE_SIZE 50
#endif
#ifdef CS_COM
    #include "Queue.h"
    #define MAX_QUEUE_SIZE 50
#endif

typedef enum
{
    SRL,        // serial
    I2C,        // i2c
    SPI,        // spi
    UNKNOWN_COM_PROTOCOL
} comProtocol;

/**
 * @brief the type of command going over the wire. only DCCEX / WITHROTTLE or Contol commands are allowed
 * the networkstation will function as MQTT & HTTP endpoint and only transmit the the DCCEX type commands
 * the API endpoint will translate to DCCEX commands no WITHROTTLE over HTTP and/or MQTT yet
 */
typedef enum
{
    _DCCEX,          //< > encoded
    _WITHROTTLE,     // Withrottle 
    _CTRL,           // messages starting with # are send to ctrl/manage the cs sie of things
                     // not used by the CS   
    _REPLY,          // Message comming back from the commandstation 
    UNKNOWN_CS_PROTOCOL
} csProtocol;



/**
 * @brief DccMessage is the struct serailazed and send over the 
 *        wire to either the command or network station
 * 
 */
typedef struct {
    int mid;               // message id; sequence number 
    int client;            // client id ( socket number from Wifi or Ethernet)
    int p;                  // either JMRI or WITHROTTLE in order to understand the content of the msg payload
    MsgPack::str_t msg;         // going to CS this is a command and a reply on return
    MSGPACK_DEFINE(mid, client, p, msg);
} DccMessage;

#ifdef NW_COM
    typedef etl::queue<DccMessage, MAX_QUEUE_SIZE, etl::memory_model::MEMORY_MODEL_SMALL> _tDccQueue;
#endif
#ifdef CS_COM
    typedef Queue<DccMessage, MAX_QUEUE_SIZE> _tDccQueue;
#endif

typedef enum
{
    IN,      
    OUT,      
    UNKNOWN_QUEUE_TYPE
} queueType;


class DccExInterface
{
private:
    comProtocol comp = UNKNOWN_COM_PROTOCOL;
    HardwareSerial *s;                               // valid only for Serial
    uint32_t speed; 
    bool init = false;
    bool blocking = true;   // send immediatly & wait for reply from the CS if false all commands send will be queued and handled in the loop 
    uint64_t    seq = 0;
#ifdef CS_COM    
    _tDccQueue *incomming = nullptr;
    _tDccQueue *outgoing = nullptr;
#endif
#ifdef NW_COM
    static _tDccQueue inco;
    static _tDccQueue outg;
    _tDccQueue *incomming = &inco;
    _tDccQueue *outgoing = &outg;
#endif
    void write();  // writes the buffers in the queue to the HwSerial s
    const char* csProtocolNames[5] = {"DCCEX","WTH","CTRL", "REPLY", "UNKNOWN"};

public:
    const uint8_t recv_index = 0x34;
    const uint8_t send_index = 0x12;
    _tDccQueue* getQueue(queueType q) {
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
    void queue(queueType q, DccMessage packet);

    void queue(uint16_t c, uint8_t p, char *msg);


    // void send(char *cmd);  // send the command 
    void recieve();        // check the transport to see if tere is something for us

    /**
     * @brief setup the serial interface 
     * 
     * @param *s        - pointer to a serial port. Default is Serial1 as Serial is used for monitor / upload etc..
     * @param speed     - default serial speed is 115200
     */
    void setup(HardwareSerial *s = &Serial1, uint32_t speed = 115200);
    void loop();
    int size(queueType inout){
        if (inout == IN) {
            return incomming->size();
        }
        if (inout == OUT) {
            return outgoing->size();
        }
        ERR(F("Unknown queue in size; specifiy either IN or OUT"));
        return 0;
    }
    void push( byte q, DccMessage m) {
        incomming->push(m);
        return;
    }

    auto decode(csProtocol p) -> const char *;

    DccExInterface(); 
    ~DccExInterface();
};

extern DccExInterface DCCI; 

#endif