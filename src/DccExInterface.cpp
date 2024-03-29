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
#include <DCSIconfig.h>
#include <DCSIlog.h>
#ifndef DCCI_CS
#include "NetworkInterface.h"
#include "Transport.h"
DCCNetwork *network = NetworkInterface::getDCCNetwork();
#endif
#include "DccExInterface.h"
#include "DCSICommand.h"

/**
 * @brief callback function upon reception of a DccMessage. Adds the message into the incomming queue
 * Queue elements will be processed then in the recieve() function called form the loop()
 *
 * @param msg DccMessage object deliverd by MsgPacketizer
 */
void foofunc2(DccMessage msg)
{
    // TRC(F("Enter fooFunc2" CR));
    // const int qs = DCCI.getQueue(IN)->size();
    // const comStation station = static_cast<comStation>(msg.sta); // Dangerous it will always succedd and thus have ev values outside ofthe enum
    // const comStation station = _DCCSTA; // for testing purposes

    if (!DCCI.getQueue(IN)->isFull())
    { // test if queue isn't full

        TRC(F("Recieved from [%s]:[%d:%d:%d:%d]: %s" CR), DCCI.decode(static_cast<comStation>(msg.sta)), DCCI.getQueue(IN)->size(), msg.mid, msg.client, msg.p, msg.msg.c_str());
        DCCI.getQueue(IN)->push(msg); // push the message into the incomming queue
        // TRC(F(" Memory ->" CR));
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
    s = _s;                      // Serial port used for com depends on the wiring
    speed = _speed;              // speed of the connection
    s->begin(speed);             // start the serial port at the given baud rate
    outgoing = new _tDccQueue(); // allocate space for the Queues
    incomming = new _tDccQueue();
    MsgPacketizer::subscribe(*s, recv_index, &foofunc2);
    init = true; // interface has been initatlized
    INFO(F("Setup of %s done ..." CR), comStationNames[sta]);
}
/**
 * @brief process all that is in the incomming queue and reply
 *
 */
auto DccExInterface::recieve() -> void
{
    if (!DCCI.getQueue(IN)->isEmpty())
    {
        DccMessage m = DCCI.getQueue(IN)->pop();
        // if recieved from self then we have an issue
        if (m.sta == sta)
        {
            ERR(F("Wrong sender; Msg seems to have been send to self; Msg has been ignored" CR));
            return;
        }
        TRC("Sending to handler" CR);
        handlers[m.p](m);
    }
    return;
}    
/**
 * @brief creates a DccMessage and adds it to the outgoing queue
 *
 * @param c  client from which the message was orginally recieved
 * @param p  protocol for the CS DCC(JMRI), WITHROTTLE etc ..
 * @param msg the messsage ( outgoing i.e. going to the CS i.e. will mostly be functional payloads plus diagnostics )
 */
void DccExInterface::queue(uint16_t c, csProtocol p, char *msg)
{

    MsgPack::str_t s = MsgPack::str_t(msg);

    DccMessage m;

    m.sta = static_cast<int>(sta);
    m.client = c;
    m.p = static_cast<int>(p);
    m.msg = s;
    m.mid = seq++;

    INFO(F("Queuing [%d:%d:%s]:[%s]" CR), m.mid, m.client, decode((csProtocol)m.p), m.msg.c_str());
    // MsgPacketizer::send(Serial1, 0x12, m);

    outgoing->push(m);
    return;
}
/**
 * @brief queue a DccMessage where the payload corresponds to the csProtocl specified. The first parameter
 * specfies if the message shall be queued in the incomming our outgoing queue
 *
 * @param q
 * @param p
 * @param packet
 */
void DccExInterface::queue(queueType q, csProtocol p, DccMessage packet)
{
    packet.mid = seq++; //  @todo shows that we actually shall package app payload with ctlr payload
                        // user part just specifies the app payload the rest get added around as
                        // wrapper here
    packet.sta = static_cast<int>(sta);
    packet.p = static_cast<int>(p);

    switch (q)
    {
    case IN:
        if (!incomming->isFull())
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
        if (!outgoing->isFull())
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
    if (!outgoing->isEmpty())
    { // be nice and only write one at a time
        // only send to the Serial port if there is something in the queu
        // TRC(F(" -> Memory" CR));
        DccMessage m = outgoing->pop();
        TRC(F("Sending [%d:%d:%d]: %s" CR), m.mid, m.client, m.p, m.msg.c_str());
        // TRC(F("Sending Message... " CR));
        MsgPacketizer::send(*s, 0x34, m);
        // TRC(F(" Memory ->" CR));
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
    if ((p >= UNKNOWN_CS_PROTOCOL) || (p < 0))
    {
        ERR(F("Cannot decode csProtocol %d returning unkown"), p);
        return csProtocolNames[UNKNOWN_CS_PROTOCOL];
    }
    return csProtocolNames[p];
}
auto DccExInterface::decode(comStation s) -> const char *
{
    // need to check if p is a valid enum value
    if ((s > 3) || (s < 0))
    {
        ERR(F("Cannot decode comStation %d returning unkown"), s);
        return comStationNames[_UNKNOWN_STA];
    }
    return comStationNames[s];
}
auto DccExInterface::dccexHandler(DccMessage m) -> void
{
    INFO(F("Processing message from [%s]:[%s]" CR), DCCI.decode(static_cast<comStation>(m.sta)), m.msg.c_str());
    // send to the DCC part he commands and get the reply
    char buffer[MAX_MESSAGE_SIZE] = {0};
    sprintf(buffer, "reply from CS: %d:%d:%s", m.client, m.mid, m.msg.c_str());
    DCCI.queue(m.client, _REPLY, buffer);
};
auto DccExInterface::wiThrottleHandler(DccMessage m) -> void{};
auto DccExInterface::ctrlHandler(DccMessage m) -> void {
    // where does the message come from
    INFO(F("Recieved CTRL message from %s" CR), DCCI.decode((comStation) m.sta));
    switch(m.sta) {
        case _DCCSTA: {
            // we are on the NW station handling a message from the commandstation
            break;
        }
        case _NWSTA: {
            TRC(F("Executing CTRL message %s" CR), m.msg.c_str());
            Cmds.run(m.msg.c_str());
            // we are on the CommandStation handling a message from the NetworkStation
            // so the message shall have the form <! opcode yy zz >
            // get the opcode and get the function to handle it from the opcode hashmap
            // opcodes is until the first whitespace in the command afetr the !
            // timming all leading spaces away
        
            break;
        }
    }
};
auto DccExInterface::notYetHandler(DccMessage m) -> void
{
    if (m.p == UNKNOWN_CS_PROTOCOL)
    {
        ERR(F("Unkown Message protocol; Message ignored" CR));
    }
    else
    {
        WARN(F("%s Message protocol not supported on %s; Message ignored" CR), DCCI.decode((csProtocol)m.p), DCCI.decode((comStation)m.sta));
    }
    return;
};
#ifndef DCCI_CS // only valid on the NW station
auto DccExInterface::replyHandler(DccMessage m) -> void
{

    INFO(F("Processing reply from the CommandStation for client [%d]..." CR), m.client);

    // search for the client in the network ... There must be a better way
    // and send the reply now to the connected client ...

    byte nt = network->getNumberOfTransports();              // #of networkinterfaces which have been instantiated
    transportType *tt = network->getArrayOfTransportTypes(); // for each of the interfaces we know the transport type

    for (byte i = 0; i < nt; i++)
    {
        switch (tt[i])
        {
        case WIFI:
        {
            WiFiTransport *wt = static_cast<WiFiTransport *>(network->transports[i]);
            if (wt->getActive() == 0)
                break; // nothing to be done no clients
            WiFiClient wtc = wt->getClient(m.client);
            if (wtc.connected())
            {
                wtc.write(m.msg.c_str());
                wtc.write(CR); // CR -> just so that we have a nl in the terminal ...
            }
            else
            {
                WARN(F("WiFi client not connected. Can't send reply" CR));
            }
            break;
        }
        case ETHERNET:
        {
            EthernetTransport *et = static_cast<EthernetTransport *>(network->transports[i]);
            if (et->getActive() == 0)
                break; // nothing to be done no clients
            EthernetClient etc = et->getClient(m.client);
            if (etc.connected())
            {
                etc.write(m.msg.c_str());
                etc.write(CR);
            }
            else
            {
                WARN(F("Ethernet client not connected. Can't send reply" CR));
            }
            break;
        }
        default:
        {
            ERR(F("Unknown transport protocol must be either WIFI or ETHERNET"));
            break;
        }
        }
    }
}
auto DccExInterface::diagHandler(DccMessage m) -> void{
    INFO(F("Recieved DIAG: %s" CR), m.msg.c_str());
};
#endif
DccExInterface::DccExInterface(){};
DccExInterface::~DccExInterface(){};

DccExInterface DCCI = DccExInterface();