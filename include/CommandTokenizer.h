/**
 * @file DCSICommand.h
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
#ifndef CommandTokenizer_h
#define CommandTokenizer_h

#include <Arduino.h>
#include <DCSIconfig.h>
#include <DCSIlog.h>


typedef enum {
    DCCEX,
    WITHROTTLE,
    HTTP,
    JSON,
    UNDEFINED
} scanType;

class CommandToken {
    const char *start_t;
    const char *end_t;
    const char *name;
    scanType cmdType = UNDEFINED;

public:
    CommandToken(const char *n, const char *s, const char *e, scanType st) {
        name = n;
        start_t = s;
        end_t =e ;
        cmdType = st;
    }

    const char *getStartToken() {
        return start_t;
    }

    const char *getEndToken() {
        return end_t;
    }

    const char *getName() {
        return name;
    }

    const scanType getCmdType() {
        return cmdType;
    }
};

// one token for each command type managed by the endpoint

static CommandToken dcc_t((const char*)"JMRI", (const char*)"<",(const char*)">", DCCEX);                          // JMRI / DC commands
static CommandToken wit_t((const char*)"WiThrottle", (const char*)"*DPTRHMRQN",(const char*)"\n", WITHROTTLE);     // WITHROTTLE commands
static CommandToken json_t((const char*)"Json", (const char*)"{",(const char*)"}", JSON);                          // JSON formatted commands need to extract transform what can be send to the CS


class CommandTokenizer {
private:

    typedef enum {
        STARTSCAN,
        START_TOKEN, IN_TOKEN, END_TOKEN,
        OVERFLOW,
        FINAL,
        ERROR
    } scanState;

    // array of all token types understood by the system. if NULL the this is planned but not yet available
    CommandToken *token[scanType::UNDEFINED +1] = {&dcc_t, &wit_t, NULL, &json_t, NULL};  // in the order of scanType so that we can use the enum to acces the araay
    
    char *start;
    char *current;
    char *end;
    const char *tmp;
    scanType currentCmdType = UNDEFINED;

    char overflow[MAX_MESSAGE_SIZE/2] = {'\0'}; 

    scanState stateStartScan();
    scanState stateOverflow();
    scanState stateFinal();
    scanState stateStartToken();
    scanState stateInToken(char *scanBuffer, const int len);
    scanState stateEndToken(char *scanBuffer, const int len);

public:

    CommandToken *findScanType (char c){
        for(int i = 0; i <= scanType::UNDEFINED; i++) {
            CommandToken *ct = token[i];
            if (ct != NULL) {
                if (strchr(token[i]->getStartToken(), c)) {
                    return token[i];
                }
            } 
        } // The char c never appears in any of the start_t arrays;
        return token[UNDEFINED];
    }
    void scanCommands(char *in, const int len);
    static void testScan();
};


#endif 
