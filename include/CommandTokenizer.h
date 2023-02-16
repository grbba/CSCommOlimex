/**
 * @file CommandTokenizer.h
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
#include <NetworkConfig.h>
#include <DCSIlog.h>


typedef enum {
    DCCEX,
    WITHROTTLE,
    HTTP,
    JSON,
    UNDEFINED
} scanType;

class CommandToken {
    String start_t; 
    String end_t; 
    String name; 
    scanType cmdType = UNDEFINED;

public:
    CommandToken( String n,  String s,  String e, scanType st) {
        name = n;
        start_t = s;
        end_t = e;
        cmdType = st;
    }

    const String* getStartToken() {
        return &start_t;
    }

    const String* getEndToken() {
        return &end_t;
    }

    const String* getName() {
        return &name;
    }

    const scanType getCmdType() {
        return cmdType;
    }
};

// one token for each command type managed by the endpoint

// static CommandToken dcc_t("JMRI", "<", ">", DCCEX);                          
// WITHROTTLE commands
// Secndary chars: "TRU" for possible disambiguation ( R overlaps with HTTP)
 // static CommandToken wit_t(( char*)"WiThrottle", ( char*)"*DPTRHMRQN",( char*)"\n", WITHROTTLE);     
 // static CommandToken json_t(( char*)"Json", ( char*)"{",( char*)"}", JSON);  
// HTTP request formatted commands need to extract transform what can be send to the CS
// Secndary chars: "UAOERP" for possible disambiguation 
 static CommandToken http_t(( char*)"HTTP", ( char*)"PGDCTOH",( char*)"\0\n", HTTP);   

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
    CommandToken *token[scanType::UNDEFINED +1] = {NULL};    // in the order of scanType so that we can use the enum to acces the araay
    
    char *start;
    char *current;
    char *end;
    const char *tmp;
    scanType currentCmdType = UNDEFINED;
    
    void (*callback)(scanType s, char * buffer);

    char overflow[MAX_MESSAGE_SIZE/2] = {'\0'}; 

    scanState stateStartScan();
    scanState stateOverflow();
    scanState stateFinal();
    scanState stateStartToken();
    scanState stateInToken(char *scanBuffer, const int len);
    scanState stateEndToken(char *scanBuffer, const int len);

public:

    void setup() {
        token[DCCEX]        = new CommandToken("JMRI", "<", ">", DCCEX);                    // JMRI / DC commands
        token[WITHROTTLE]   = new CommandToken("WiThrottle","*DPTRHMRQN","\n", WITHROTTLE); // WITHROTTLE commands; Secndary chars: "TRU" for possible disambiguation ( R overlaps with HTTP)
        token[JSON]         = new CommandToken("Json","{", "}", JSON);                      // JSON like http extract payload before sending to CS
        token[HTTP]         = new CommandToken("HTTP", "PGDCTOH","\0\n", HTTP);             // HTTP request -> prep before send to the CS - Secondary chars: "UAOERP" for possible disambiguation 
        token[UNDEFINED]    = NULL; 
    }

    CommandToken *findScanType (char c){
        for(int i = 0; i <= scanType::UNDEFINED; i++) {
            CommandToken *ct = token[i];
            if (ct != NULL) {
                if (strchr(token[i]->getStartToken()->c_str(), c)) {
                    return token[i];
                }
            } 
        } // The char c never appears in any of the start_t arrays;
        return token[UNDEFINED];
    }
    void scanCommands(char *in, const int len, void (*handler)(scanType s, char * buffer) );
    // static void testScan();

    CommandTokenizer() {
        setup();
    }
};

extern CommandTokenizer tokenizer;

#endif 
