/**
 * @file DccExInterface.cpp
 * @author Gregor Baues
 * @brief Implementation of a communication protocol between two MCU type devices.
 * Developped for the purpose of offloading Ethernet, Wifi etc communictions
 * which are ressource consuming from one of the devices esp UNO, Megas etc.
 * This has been developped for the DCC-EX commandstation running eiher on an Uno or Mega.
 * Although serial, Ethernet and Wifi code exists all the CS functionality leaves
 * little to no space for the communication part.
 * By offloading this into a separate MCU also other evolutions could be imagined.
 * the communication to avoid unnecessary overhead concentrates on UART Serial first as this
 * is build in I2C and/or SPI could be imagined at a future state
 * @version 0.1
 * @date 2023-01-13
 *
 * @copyright Copyright (c) 2022, 2023
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


#ifdef NW_COM
_tDccQueue DccExInterface::inco;
_tDccQueue DccExInterface::outg;
#endif

/**
 * @brief callback function upon reception of a DccMessage. Adds the message into the incomming queue 
 * Queue elements will be processed then in the recieve() function called form the loop()
 *
 * @param msg DccMessage object deliverd by MsgPacketizer
 */
void foofunc2(DccMessage msg)
{

    // INFO(F("Recieved:[%d:%d:%d]: %s"), dmsg.mid, dmsg.client, dmsg.p, dmsg.msg.c_str());
    // Serial.println(dmsg.msg.c_str());
    // msgPack doesn't like the uint64_t etc types ...

    const int qs = DCCI.getQueue(IN)->size();
    if (qs < DCCI.getQueue(IN)->max_size())
    { // test if queue is full

        TRC(F("Recieved:[%d:%d:%d:%d]: %s" CR), qs, msg.mid, msg.client, msg.p, msg.msg.c_str());
        DCCI.getQueue(IN)->push(msg); // push the message into the incomming queue
    }
    else
    {
        ERR(F("Incomming queue is full; Message has not been processed" CR));
    }
}

/**
 * @brief           init the serial com port with the command/network station as well as the
 *                  queues if needed
 *
 * @param _s        HardwareSerial port ( this depends on the wiring between the two boards
 *                  which Hw serial are hooked up together)
 * @param _speed    Baud rate at which to communicate with the command/network station
 */
auto DccExInterface::setup(HardwareSerial *_s, uint32_t _speed) -> void
{
    INFO(F("Setting up DccEx Network interface connection ..." CR));
    s = _s;
    speed = _speed;
    s->begin(speed);
    init = true;
#ifdef CS_COM
    outgoing = new _tDccQueue(); // allocate space for the Queues
    incomming = new _tDccQueue();
#endif
    MsgPacketizer::subscribe(*s, recv_index, &foofunc2);
    INFO(F("Setup done ..." CR));
}

/**
 * @brief process all that is in the incomming queue and reply
 *
 */
auto DccExInterface::recieve() -> void
{
    if (!DCCI.getQueue(IN)->empty())
    {
#ifdef NW_COM
        // we get replies from the CS here
        DccMessage m = DCCI.getQueue(IN)->front();
        INFO(F("Processing message from CS: %s" CR), m.msg.c_str());
        // pass them onto the client
#endif
#ifdef CS_COM
        DccMessage m = DCCI.getQueue(IN)->peek();
        // we get commands from NW here: process them and send the reply from the CS
        INFO(F("Processing message from NW: %s" CR), m.msg.c_str());
        // send to the DCC part he commands and get the reply
        char buffer[MAX_MESSAGE_SIZE] = {0};
        sprintf(buffer, "reply from CS: %d:%d:%s", m.client, m.mid, m.msg.c_str());
        // buffer[strlen(buffer)] = '\0';
        queue(m.client, _REPLY, buffer);
#endif       
        DCCI.getQueue(IN)->pop();
    }
    return;
};

/**
 * @brief creates a DccMessage and adds it to the outgoing queue
 *
 * @param c  client from which the message was orginally recieved
 * @param p  protocol for the CS DCC(JMRI), WITHROTTLE etc ..
 * @param msg the messsage ( outgoing i.e. going to the CS i.e. will mostly be functional payloads plus diagnostics )
 */
void DccExInterface::queue(uint16_t c, uint8_t p, char *msg)
{

    MsgPack::str_t s = MsgPack::str_t(msg);
    DccMessage m;
    
    m.client = c;
    m.p = p;
    m.msg = s;
    m.mid = seq++;

    INFO("Queuing [%d:%d:%s]:[%s]" CR, m.mid, m.client, decode((csProtocol) m.p), m.msg.c_str());
    // MsgPacketizer::send(Serial1, 0x12, m);

    outgoing->push(m);
    return;
}

void DccExInterface::queue(queueType q, DccMessage packet)
{
    packet.mid = seq++; //  @todo shows that we actually shall package app payload with ctlr payload
                        // user part just specifies the app payload the rest get added around as
                        // wrapper here
    switch (q)
    {
    case IN:
        if (incomming->size() < incomming->max_size())
        {
            // still space available
            incomming->push(packet);
        }
        else
        {
            ERR(F("Incomming queue is full; Message hasn't been queued"));
        }
        break;
    case OUT:
        if (outgoing->size() < outgoing->max_size())
        {
            // still space available
            outgoing->push(packet);
        }
        else
        {
            ERR(F("Outgoing queue is full; Message hasn't been queued"));
        }
        break;
    default:
        ERR(F("Can not queue: wrong queue type must be IN or OUT"));
        break;
    }
}

/**
 * @brief write pending messages in the outgoing queue to the serial connection
 *
 */
void DccExInterface::write()
{
    // while (!outgoing->empty()) {  // empty the queue
    if (!outgoing->empty())
    { // be nice and only write one at a time
        // only send to the Serial port if there is something in the queu
#ifdef CS_COM
        DccMessage m = outgoing->peek();
#endif
#ifdef NW_COM
        DccMessage m = outgoing->front();
#endif

        TRC("Sending [%d:%d:%d]: %s" CR, m.mid, m.client, m.p, m.msg.c_str());
        // TRC(F("Sending Message... " CR));
        MsgPacketizer::send(*s, 0x34, m);
        outgoing->pop();
    }
    return;
};

void DccExInterface::loop()
{
    write();   // write things the outgoing queue to Serial to send to the party on the other end of the line
    recieve(); // read things from the incomming queue and process the messages any repliy is put into the outgoing queue
    // update();    // check the com port read what is avalable and push the messages into the incomming queue

    MsgPacketizer::update(); // send back replies and get commands/trigger the callback
};

auto DccExInterface::decode(csProtocol p) -> const char *
{
    // need to check if p is a valid enum value
    if ((p > 4) || (p < 0)) {
        ERR("Cannot decode csProtocol %d returning unkown", p);
        return csProtocolNames[UNKNOWN_CS_PROTOCOL];
    }
    return csProtocolNames[p];
}

DccExInterface::DccExInterface(){};
DccExInterface::~DccExInterface(){};

DccExInterface DCCI = DccExInterface();