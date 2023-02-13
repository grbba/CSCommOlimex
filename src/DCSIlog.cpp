/**
 * @file DCSIlog.cpp
 * @author Gregor Baues
 * @brief 
 * @version 0.1
 * @date 2023-02-09
 * 
 * @copyright Copyright (c) 2023
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
 * 
 */

#include "DCSIlog.h"
#include <DCSIconfig.h>
#include <StreamUtils.h>
#include <MsgPacketizer.h>
#include <DccExInterface.h>

#ifdef DCCI_CS // only for the commandstation

unsigned int FSHlength(const __FlashStringHelper * FSHinput) {
  PGM_P FSHinputPointer = reinterpret_cast<PGM_P>(FSHinput);
  unsigned int stringLength = 0;
  while (pgm_read_byte(FSHinputPointer++)) {
    stringLength++;
  }
  return stringLength;
}
/**
 * @brief dropin replacement for the StringFormatter::diag in the DIAG.h file of the CS
 * This way all DIAG output will be send to the _NWSTA for display/further processing 
 * in order to further send the message down to the original client who enabled the DIAGS 
 * to be send a CTRL message exists to set the client id - [dc #]  - dc: diag client # number 
 * e.g. [dc 0]. eg. if in this case the client is a telnet terminal the message shall show up there.
 * 
 * @param input 
 */
void DCSILog::diag(const FSH *input...)
{
    StringPrint stream;
    DccMessage diagMsg;
    // get the format sting from Progmem
    const byte inputLength = FSHlength(input);
    char inputBuffer[inputLength +1];
    memcpy_P(inputBuffer, input, inputLength + 1);  //+1 for the null terminator

    char buffer[MAX_MESSAGE_SIZE];

    va_list args;
    va_start(args, input);
    vsnprintf(buffer, MAX_MESSAGE_SIZE-1, inputBuffer, args);

    stream.print(F("<* "));  
    stream.print(buffer);
    // send(&stream,input,args);           // send a stream whatever to get he output from the DIAG
    stream.print(F(" *>\n"));           // then add postfix print(F(" *>\n"));

    diagMsg.client = 0; // TODO make sure we get the right client add CTRL messsage to set the client in
                        // TODO DCSIlog object

    if (stream.str().length() > MAX_MESSAGE_SIZE) { 
        WARN(F("Warning DIAG message has been truncated before being send"));    // warn that ths string will be truncated (ev ad an ellipse to show this ...)
        diagMsg.msg = stream.str().substring(0,MAX_MESSAGE_SIZE-1);
    } else {
        diagMsg.msg = stream.str().substring(0,stream.str().length()-1);
    }

    INFO(F("Sending Diagnostics: %s" CR),diagMsg.msg.c_str());
    DCCI.queue(OUT, _DIAG, diagMsg);     // queue the msg to be send with protocol DIAG
    
    // test for msg size .. if > max message size truncate and send a warning as additional DIAG message  ... 
    // queue the string to be send  with protocol DIAG
    // if we send messages from the interface the use DCSILOG as protocol
}
/**
 * @brief the idea is to just sen ids for error & warnings in order to keep the code on the CS clean and less cluttered
 * but at the same time for dev mode we'd like to get the full set of data according to the loglevel.
 * @todo How to swithc from full to id mode in release ? Retink the #define codes etc.
 * @param t  W for Warning or E for Error
 * @param ec id of the message to be shown on the _NWSTA
 */
void DCSILog::flow(char t,int ec) {
    StringPrint stream;
    DccMessage diagMsg;
    char buffer[MAX_MESSAGE_SIZE];
    
    sprintf(buffer, "%c%d", t, ec);
    stream.print(buffer);
    diagMsg.msg = stream.str();
    diagMsg.client = 0;
    diagMsg.sta = _DCCSTA;

    DCCI.queue(OUT, _CTRL, diagMsg);     // queue the msg to be send with protocol CTRL
}

#endif

DCSILog dccLog = DCSILog();