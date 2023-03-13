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
#include <CommandTokenizer.h>
#include <TransportProcessor.h>

Connection *TransportProcessor::currentConnection;

HttpRequest httpReq;

uint32_t _rseq[MAX_SOCK_NUM] = {0}; // sequence number for packets recieved per connection
uint32_t _sseq[MAX_SOCK_NUM] = {0}; // sequence number for commands send to the Commandstation per connection
uint32_t _pNum = 0;                 // number of total packets recieved


// char protocolName[6][11] = {"JMRI", "WITHROTTLE", "HTTP", "MQTT", "CTRL", "UNKNOWN"}; // change for Progmem

bool diagNetwork = false;      // if true diag data will be send to the connected telnet client
uint8_t diagNetworkClient = 0; // client id for diag output

/**
 * @brief callback provided to the tokenizer
 * 
 * @param s 
 * @param token 
 */
void TransportProcessor::tokenHandler(scanType s, char *token) {
    csProtocol p;
    bool queue = false;
    INFO(F("Handling token:%d:%s" CR), (int) s, token);
    // currentConnection contains the connection from which the c$scanned stream has been recieved
    switch(s) {
        case DCCEX: {

            if (token[1] == '!') {
                p = _CTRL;
            } else {
                p = _DCCEX;
            }
            queue = true;
            break;
        }
        case WITHROTTLE:{
            p = _WITHROTTLE;
            queue = true;
            break;
        }
        case HTTP:{ 
            // need to extract the payload before sending as the CommandStation only 
            // supports DCCEX or WIHROTTLE format for now  
            break;
        }
        case JSON:{
            // idem HTTP 
            break;
        }
        case UNDEFINED:{
            // nothing to be done should not end up here 
            break;
        }
        default: {
            // here we have an ERROR 
        break;
        }
    }
    _sseq[currentConnection->id];
    if(queue) DccExInterface::queue(currentConnection->id, p, token);
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
    
    IPAddress remote = c->client->remoteIP();
    INFO(F("Client #[%d] Received packet #[%d] of size:[%d] from [%d.%d.%d.%d]" CR), c->id, _pNum, count, remote[0], remote[1], remote[2], remote[3]);
    _rseq[c->id]++; // increase the number of packets recieved 
    // tokenize the recived information and send the token to the 
    currentConnection = c;
    tokenizer.scanCommands((char *) buffer, count, &TransportProcessor::tokenHandler);
    _pNum++;
    TRC(F("Tokenizer done ..." CR));
}

