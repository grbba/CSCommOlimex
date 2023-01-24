/*
 *  © 2023, Gregor Baues. All rights reserved.
 *  
 *  This file is part of the ESP Network Communications Framework for DCC-EX
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
#ifndef DIAG_h
#define DIAG_h

#include <Arduino.h>
#include <ArduinoLog.h>

// Diag config
// set inital log level here as well as if line / files etc shpuld be shown

//

#define FLNAME true
// static bool printFileLine = true;


#define EH_DW(code) \
    do              \
    {               \
        code        \
    } while (0) //wraps in a do while(0) so that the syntax is correct.



#define EH_IFLL(LL,code) if(Log.getLevel() >= LL){code} 
#define EH_IFFL(FL,code) if(FL == true){code}  //File and Line nuber

                               // Log.info("%s:%d ",__FILE__,__LINE__); 
#define INFO(message...) EH_DW(EH_IFLL( LOG_LEVEL_INFO, Log.printTimestamp(); );\
                               EH_IFFL( FLNAME, Log.info("%s:%d:%s",__FILE__,__LINE__,__FUNCTION__); ); \
                               EH_IFLL(LOG_LEVEL_INFO, Log.printLogLevel(Log.getLogOutput(), LOG_LEVEL_INFO);); \
                               Log.info(message);)

#define ERR(message...) EH_DW(EH_IFLL( LOG_LEVEL_ERROR, Log.printTimestamp(); );\
                               Log.error("%s:%d:%s",__FILE__,__LINE__,__FUNCTION__); \
                               EH_IFLL(LOG_LEVEL_ERROR, Log.printLogLevel(Log.getLogOutput(), LOG_LEVEL_ERROR);); \
                               Log.error(message);)

#define WARN(message...) EH_DW(EH_IFLL( LOG_LEVEL_WARNING, Log.printTimestamp(); );\
                               Log.warning("%s:%d:%s",__FILE__,__LINE__,__FUNCTION__); \
                               EH_IFLL(LOG_LEVEL_WARNING, Log.printLogLevel(Log.getLogOutput(), LOG_LEVEL_WARNING);); \
                               Log.warning(message);)

#define TRC(message...) EH_DW(EH_IFLL( LOG_LEVEL_TRACE, Log.printTimestamp(); );\
                               Log.trace("%s:%d:%s",__FILE__,__LINE__,__FUNCTION__); \
                               EH_IFLL(LOG_LEVEL_TRACE, Log.printLogLevel(Log.getLogOutput(), LOG_LEVEL_TRACE);); \
                               Log.trace(message);)

#define FATAL(message...) EH_DW(EH_IFLL( LOG_LEVEL_FATAL, Log.printTimestamp(); );\
                               Log.fatal("%s:%d:%s",__FILE__,__LINE__,__FUNCTION__); \
                               EH_IFLL(LOG_LEVEL_FATAL, Log.printLogLevel(Log.getLogOutput(), LOG_LEVEL_FATAL);); \
                               Log.fatal(message);)

#endif
