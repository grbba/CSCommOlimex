/**
* scan incomming stream for commands no differentiation on the first char of what we see as of today
* 
* manages overflow i.e. if we have the beginning of a command without having found the terminator
* before the end of the block of char read available we keep those in a buffer end prepend this
* to the next block of char available where we hopefully will find the end delimiter of a command
* if not i.e. we see the start of a new command just discard everything before and issue a warning
* delimiter  for 
* DCCEX/JMRI: < -payload- > or <! -payload- > <! -payload- > messages are management/ctrl messages for the communication part of the 
*             CS and will not be send/parsed by the DCCEX parser for controlling the layout
* WITHROTTLE: 
* (star)/D/PT/PR/HU/M/R/Q/N -payload- \n  end of the command is always a \n
* HTTP: PUT/PATCH/POST/GET/DELETE/CONNECT/TRACE/OPTIONS/HEAD      
*       end of the message is always the start of a new message ... (?)
* HTTP messages will not be send to the CommandStation as such the Networkstation functions as endpoint 
*               extacting DCCEX/JMRI or WiThrottle messages as payloads
* MQTT:         same as for HTTP (MQTT is used to control any CommandStation over the API deployed at OVH 
*               at this point in time) - check my current MQ imlementation ...
*/ 

/*
*  Lest start ith DCCEX/JMRI and WITHROTTLE ( plus conrol messages such as <! but for the networkstation)
* for the latter two options <!! > for example could be interpreted locally or a 
* different command frame like ()
*/

#include <Arduino.h>
#include <DCSIconfig.h>
#include <DCSIlog.h>
#include <CommandTokenizer.h>

// #include <string.h>

CommandTokenizer::scanState CommandTokenizer::stateStartScan() { 
                CommandToken *ct;
                ct = findScanType(*current);
                if (*current != '\0' && ct != NULL) {
                    if ( ct->getCmdType() != UNDEFINED ) {  
                        currentCmdType = ct->getCmdType();
                        start = current;
                        // INFO(F("%s start found" CR), ct->getName());
                        return START_TOKEN;
                    }
                    currentCmdType = UNDEFINED;
                    return STARTSCAN; 
                } else {
                    return STARTSCAN; 
                }
}
CommandTokenizer::scanState CommandTokenizer::stateOverflow() {
    int clen = (end - start)+1;
    if (clen >= MAX_MESSAGE_SIZE/2) {
        // WARN command too long
        // send warning and ignore the command
    }
    memset(overflow, 0, MAX_MESSAGE_SIZE/2); // clean the buffer
    strncpy(overflow, start, clen-1); // remove the CRLF
    // std::cout << "Handling overflow copying: " << overflow << std::endl;
    return(FINAL);
}
// we never actual get here ? check this
CommandTokenizer::scanState CommandTokenizer::stateFinal() {
    INFO(F("Tokenization finished" CR));
    return FINAL;
}
CommandTokenizer::scanState CommandTokenizer::stateStartToken() { 
    return(IN_TOKEN);
}
CommandTokenizer::scanState CommandTokenizer::stateInToken(char *scanBuffer, const int len) { 
    // check if we are at the end of the string -> overflow
    if (current == &scanBuffer[len]) {
        // std::cout << "Overflow dcc" << std::endl; 
        end = current;
        return(OVERFLOW); 
    }
    if (strchr(token[currentCmdType]->getEndToken(), *current) != NULL ) { // here we have to check for the end of the type we are scanning
        end = current;
        return(END_TOKEN);
    }
    // no end found we are still inside the command
    // we need to check if we didn't exceed the limits here 
    return(IN_TOKEN);
}
CommandTokenizer::scanState CommandTokenizer::stateEndToken(char *scanBuffer, const int len) {
    char buffer[MAX_MESSAGE_SIZE] = {'\0'}; // this is for temp storage of the command to be send to the CS
    // std::cout << token[currentCmdType]->getName() <<  " end found : ";  // get the right one
    
    int clen = (end - start)+1;
    if (clen >= MAX_MESSAGE_SIZE) {
        ERR(F("Token is too long: ignoring" CR));
        // try to recover and find the next token
        return STARTSCAN;
    }
    memset(buffer, 0, MAX_MESSAGE_SIZE); // clean the buffer
    strncpy(buffer, start, clen);
    INFO(F("Queuing buffer %d %d %s" CR), (current == &scanBuffer[len]), clen, buffer);
    if( current == &scanBuffer[len] ) {
        return(FINAL); // we have reached the end of the buffer;
    } else {
        start = --current;
        currentCmdType = UNDEFINED;
        return(STARTSCAN);
    } 
}
void CommandTokenizer::scanCommands(char *in, const int inl) {

    // all possible token i can find are listed in here
    // this needs to be completed when there are new tokens to be added

    int len = inl + strlen(overflow);   // overall length we need overflow plus length of the incomming stream
    char scanBuffer[len];               // allocate the buffer for all of the content

    memset(scanBuffer, 0, len);                        // clean the content buffer
    strcat(scanBuffer, overflow);                      // copy the overflow into the buffer
    strcat(scanBuffer, in);                            // append the incomming buffer
    memset(overflow, 0, MAX_MESSAGE_SIZE/2+1);         // clean the overflow buffer once it has been used

    start = scanBuffer;                                // set start & end pointers
    current = scanBuffer;

    char buffer[MAX_MESSAGE_SIZE] = {'\0'}; // this is for temp storage of the command to be send to the CS
    scanState state = STARTSCAN;
    
    // std::cout << "Scanning " << scanBuffer << std::endl;
    // std::cout << "Current state " << state << std::endl;

    while (state != FINAL) {
        switch(state) {
            case STARTSCAN :{
                state = stateStartScan();
                break;
            }
            case START_TOKEN: {
                state = stateStartToken();
                break;
            }
            case IN_TOKEN: {
                state = stateInToken(scanBuffer, len);
                break;
            }
            case END_TOKEN: {
                state = stateEndToken(scanBuffer, len);
                break;
            }
            case FINAL : {
                state = stateFinal();
                break;
            }
            case OVERFLOW : {
                state = stateOverflow();
                break;
            }
            default : {
                // std::cout << " Error " << std::endl;
                break;
            }
        }
        if(current == &scanBuffer[len]) {
            if (state != OVERFLOW) {
                state = FINAL; // we have reached the end of the buffer;
            }
        } else {
            current++;
        }  
    }
    // std::cout << "state " << state << std::endl;
    return;
}

/*
typedef CommandTokenizer::scanState (CommandTokenizer::*StateFunction)(char *, int);
void CommandTokenizer::scanCommandsHelper(StateFunction *stateFunctions, char *scanBuffer, int len, scanState state) {
    if (state == FINAL) {
        return;
    }

    state = (this->*stateFunctions[state])(scanBuffer, len);
    if (current == &scanBuffer[len]) {
        if (state != OVERFLOW) {
            state = FINAL;
        }
    } else {
        current++;
    }

    scanCommandsHelper(stateFunctions, scanBuffer, len, state);
}
void CommandTokenizer::scanCommands3(char *in, const int inl) {
    StateFunction stateFunctions[] = {
        &CommandTokenizer::stateStartScan2,
        &CommandTokenizer::stateStartToken,
        &CommandTokenizer::stateInToken,
        &CommandTokenizer::stateEndToken,
        &CommandTokenizer::stateFinal,
        &CommandTokenizer::stateOverflow
    };

    int len = inl + strlen(overflow);   // overall length we need overflow plus length of the incoming stream
    char scanBuffer[len];               // allocate the buffer for all of the content

    memset(scanBuffer, 0, len);         // clean the content buffer
    strcat(scanBuffer, overflow);       // copy the overflow into the buffer
    strcat(scanBuffer, in);             // append the incoming buffer

    start = scanBuffer;                 // set start & end pointers
    current = scanBuffer;

    std::cout << "Scanning " << scanBuffer << std::endl;
    std::cout << "Current state " << state << std::endl;

    scanCommandsHelper(stateFunctions, scanBuffer, len, STARTSCAN);

    std::cout << "state " << state << std::endl;
    return;
}
*/

CommandTokenizer parser;
char stream[] = "sfs dq<! dia 2>Mx245_\n. <!! local 12>< hhh fo><bar me"; // this generates an overflow
char stream2[] = "sfs dq>{hhh}Mx267ABC_\n"; // overflow test
char stream3[] = "abcdefg"; //no command test
char stream4[] = "<incomplete"; //incomplete 
char stream5[] = "<waytoooooooooooooooooooooooooolongxx>"; // way too long token plus carry over from the previous

char simple[] = "<s>";
//char simple[] = "<s><R 1 1 1>";

void CommandTokenizer::testScan() {
    // parser.scanCommands(simple, strlen(simple));
    INFO(F("-- Generic test plus overflow %d %d --" CR ), strlen(stream), strlen(stream2));
    INFO(F("-- Scanning %s and %s --" CR ), stream, stream2);
    parser.scanCommands(stream, strlen(stream));
    parser.scanCommands(stream2, strlen(stream2)); 
    
    INFO(F("-- No token in the stream test --" CR ));
    parser.scanCommands(stream3, strlen(stream3)); 
    
    INFO(F("-- Incomplete 1st & only command & no cont in next string -- " CR ));
    parser.scanCommands(stream4, strlen(stream4)); 
    
    INFO(F("-- Token too long -- " CR ));
    parser.scanCommands(stream5, strlen(stream5));

}


