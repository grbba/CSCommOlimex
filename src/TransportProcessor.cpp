/*
 * © 2020, 2023 Gregor Baues. All rights reserved.
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
#include <Config.h>
#include <DCSIlog.h>
#include <DccExInterface.h>


#include "NetworkInterface.h"
#include "HttpRequest.h"
#include "TransportProcessor.h"


HttpRequest httpReq;

uint16_t _rseq[MAX_SOCK_NUM] = {0}; // sequence number for packets recieved per connection
uint16_t _sseq[MAX_SOCK_NUM] = {0}; // sequence number for replies send per connection
uint16_t _pNum = 0;                 // number of total packets recieved
unsigned int _nCmds = 0;            // total number of commands processed

// char protocolName[6][11] = {"JMRI", "WITHROTTLE", "HTTP", "MQTT", "CTRL", "UNKNOWN"}; // change for Progmem

bool diagNetwork = false;      // if true diag data will be send to the connected telnet client
uint8_t diagNetworkClient = 0; // client id for diag output

/**
 * @brief Set the App Protocol. The detection is done upon the very first message recieved. The client will then be bound to that protocol. Its very brittle 
 * as e.g. The N message as first message for WiThrottle is not a requirement by the protocol; If any client talking Withrottle doesn't implement this the detection 
 * will default to JMRI. For HTTP we base this only on a subset of the HTTP verbs which can be used.
 * 
 * @param a First character of the recieved buffer upon first connection
 * @param b Second character of the recieved buffer upon first connection
 * @return csProtocol 
 */
csProtocol setAppProtocol(char a, char b, Connection *c)
{
    csProtocol p = UNKNOWN_CS_PROTOCOL;
    switch (a)
    {
    case 'G': // GET
    case 'C': // CONNECT
    case 'O': // OPTIONS
    case 'T': // TRACE
    {
        p = _HTTP;
        break;
    }
    case 'D': // DELETE or D plus hex value
    {
        if (b == 'E')
        {
            p = _HTTP;
        }
        else
        {
            p = _WITHROTTLE;
        }
        break;
    }
    case 'P':
    {
        if (b == 'T' || b == 'R')
        {
            p = _WITHROTTLE;
        }
        else
        {
            p = _HTTP; // PUT / PATCH / POST
        }
        break;
    }
    case 'H':
    {
        if (b == 'U')
        {
            p = _WITHROTTLE;
        }
        else
        {
            p = _HTTP; // HEAD
        }
        break;
    }
    case 'M':
    case '*':
    case 'R':
    case 'Q': // That doesn't make sense as it's the Q or close on app level
    case 'N':
    {
        p = _WITHROTTLE;
        break;
    }
    case '<':
    {
        p = _DCCEX;   
        c->start_delimiter = '<';
        break;
    }
    default:
    {
        // here we don't know
        p = UNKNOWN_CS_PROTOCOL;
        break;
    }
    }
    INFO(F("Client speaks: [%s]" CR), DCCI.decode(p));
    return p;
}

/**
 * @brief Parses the buffer to extract commands to be executed before being send to the CommandStation
 */
void processStream(Connection *c, TransportProcessor *t)
{
    uint8_t i, j, k, l = 0;
    uint8_t *_buffer = t->buffer;

    TRC(F("Buffer: [%s]" CR), _buffer);
    memset(t->command, 0, MAX_JMRI_CMD); // clear out the command

    // copy overflow into the command
    if ((i = strlen(c->overflow)) != 0)
    {
        // TRC(F("Copy overflow to command: %s"), c->overflow);
        strncpy(t->command, c->overflow, i);
        k = i;
    }
    // reset the overflow
    memset(c->overflow, 0, MAX_OVERFLOW);

    // check if there is again an overflow and copy if needed
    if ((i = strlen((char *)_buffer)) == MAX_ETH_BUFFER - 1)
    {
        TRC(F("Possible overflow situation detected: %d "), i);
        j = i;
        TRC(F("> init search index %d" CR), i);
        while (_buffer[i] != c->end_delimiter)
        {
            i--;
            TRC(F("> search index %d" CR), i);
            if (i <= 0) {
               TRC(F("No valid delimiter found; wrong command"));
            } 
        }

        i++; // start of the buffer to copy
        l = i;
        k = j - i; // length to copy

        for (j = 0; j < k; j++, i++)
        {
            c->overflow[j] = _buffer[i];
            // TRC(F("%d %d %d %c"),k,j,i, buffer[i]);
        }
        _buffer[l] = '\0'; // terminate buffer just after the last '>'
        // TRC(F("New buffer: [%s] New overflow: [%s]"), (char*) buffer, c->overflow );
    }
    // breakup the buffer using its changed length
    i = 0;
    k = strlen(t->command); // current length of the command buffer telling us where to start copy in
    l = strlen((char *)_buffer);
    // DBG(F("Command buffer cid[%d]: [%s]:[%d:%d:%d:%x]"), c->id,  t->command, i, l, k, c->delimiter );
    unsigned long _startT = micros();
    _nCmds = 0;
    while (i < l)
    {
        // DBG(F("l: %d - k: %d - i: %d - %c"), l, k, i, _buffer[i]);
        t->command[k] = _buffer[i];
        if (_buffer[i] == c->end_delimiter)
        { // closing bracket need to fix if there is none before an opening bracket ?

            t->command[k + 1] = '\0';

            TRC(F("Command: [%d:%s]" CR), _rseq[c->id], t->command);

            // Sanity check : the first character must be an < otherwise something is fishy only if prootocol is JMRI
            // if Withrottle something else applies and we need to check actually in a list of possible options
            // cf setAppProtocol
            if (t->command[0] != c->start_delimiter) {

                ERR(F("Wrong command syntax: missing %c" CR), c->start_delimiter);

            } else { 
                if(t->command[1] == '!') {   // tag as ctrl command so no need to test for that on the CS
                    c->p = _CTRL;
                }
                INFO(F("Queuing: %s - %s" CR), &t->command[0], DCCI.decode(c->p));
                // DCCI.queue(c->id, c->p, &t->command[0]);
            }
            _rseq[c->id]++;
            _nCmds++; 
            j = 0;
            k = 0;
        }
        else
        {
            k++;
        }
        i++;
    }
    unsigned long _endT = micros();
    char time[10] = {0};
    ultoa(_endT - _startT, time, 10);
    INFO(F("[%d] Commands processed in [%s]uS\n"), _nCmds, time);
}


void echoProcessor(Connection *c, TransportProcessor *t)


{
    byte reply[MAX_ETH_BUFFER];

    memset(reply, 0, MAX_ETH_BUFFER);
    sprintf((char *)reply, "ERROR: malformed content in [%s]", t->buffer);
    TRC(F("%d,%d:Echoing back: %s" CR), c->id, c->client->connected(), reply);
    if (c->client->connected())
    {
        c->client->write(reply, strlen((char *)reply));
        _sseq[c->id]++;
        c->isProtocolDefined = false; // reset the protocol to not defined so that we can recover the next time
    }
}
void jmriProcessor(Connection *c, TransportProcessor *t)
{
    TRC(F("Processing JMRI ..." CR));
    processStream(c, t);
}
void withrottleProcessor(Connection *c, TransportProcessor *t)
{
    TRC(F("Processing WiThrottle ...to be done" CR));
    // processStream(c, t);
}
/**
 * @brief creates a HttpRequest object for the user callback. Some conditions apply esp reagrding the length of the items in the Request
 * can be found in @file HttpRequest.h 
 *  
 * @param client Client object from whom we receievd the data
 * @param c id of the Client object
 */
void httpProcessor(Connection *c, TransportProcessor *t)
{

    if (httpReq.callback == 0)
        return; // no callback i.e. nothing to do
    /**
     * @todo look for jmri formatted uris and execute those if there is no callback. If no command found ignore and 
     * ev. send a 401 error back
     */
    uint8_t i, l = 0;
    ParsedRequest preq;
    l = strlen((char *)t->buffer);
    for (i = 0; i < l; i++)
    {
        httpReq.parseRequest((char)t->buffer[i]);
    }
    if (httpReq.endOfRequest())
    {
        preq = httpReq.getParsedRequest();
        httpReq.callback(&preq, c->client);
        httpReq.resetRequest();
    } // else do nothing and continue with the next packet
}

/**
 * @brief Reads what is available on the incomming TCP stream and hands it over to the protocol handler.
 * 
 * @param c    Pointer to the connection struct contining relevant information handling the data from that connection
 */
void TransportProcessor::readStream(Connection *c, bool read)
{
    int count = 0;
    // read bytes from a TCP client if required 
    if (read) {
        int len = c->client->read(buffer, MAX_ETH_BUFFER - 1); // count is the amount of data ready for reading, -1 if there is no data, 0 is the connection has been closed
        buffer[len] = 0;
        count = len;
    } else {
        count = strlen((char *)buffer);
    }
    
    // figure out which protocol

    if (!c->isProtocolDefined)
    {
        c->p = setAppProtocol(buffer[0], buffer[1], c);
        c->isProtocolDefined = true;

        switch (c->p)
        {
        case _DCCEX:
        {
            c->end_delimiter = '>';
            c->appProtocolHandler = (appProtocolCallback)jmriProcessor;
            break;
        }
        case _WITHROTTLE:
        {
            c->end_delimiter = '\n';
            c->appProtocolHandler = (appProtocolCallback)withrottleProcessor;
            break;
        }
        case _HTTP:
        {
            c->appProtocolHandler = (appProtocolCallback)httpProcessor;
            httpReq.callback = nwi->getHttpCallback();
            break;
        }
        case UNKNOWN_CS_PROTOCOL:
        {
            INFO(F("Requests will not be handeled and packet echoed back" CR));
            c->appProtocolHandler = (appProtocolCallback)echoProcessor;
            break;
        }
        }
    }
    _pNum++;
    IPAddress remote = c->client->remoteIP();
    INFO(F("Client #[%d] Received packet #[%d] of size:[%d] from [%d.%d.%d.%d]" CR), c->id, _pNum, count, remote[0], remote[1], remote[2], remote[3]);
    
    // Clean up if we recieve unwanted \n and/or \r characters at the end 
    // e.g. terminal on MAC we get both on PacketSender as of confguration you get it or not

    if (buffer[count] == '\r' || buffer[count] == '\n') buffer[count] = '\0';
    if (buffer[count-1] == '\r' || buffer[count-1] == '\n') buffer[count-1] = '\0';
    if (buffer[count-2] == '\r' || buffer[count-2] == '\n') buffer[count-2] = '\0';

    // terminate the string Properly
    buffer[count] = '\0'; 
    INFO(F("Packet: [%s]" CR), buffer);
    
    // chop the buffer into CS / WiThrottle commands || assemble command across buffer read boundaries
    c->appProtocolHandler(c, this);
}


