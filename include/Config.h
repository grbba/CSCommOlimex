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
 *  You should have received a copy of the GNU General Public License.
 *  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef Config_h
#define Config_h

#include <Arduino.h>
#include <DCSIlog.h>


/**
 * @brief set compile options 
 * 
 */

#define DCCEX_ENABLED


#define DCC_SSID  "DeepSpace"
#define DCC_WPWD  "qsdfghjklm!"


#define dccexcom 0    // build dccex integration
#define netdiag 0     // enable diagnotics to be send to a network client



#if (dccexcom)
    #define DCCEX_ENABLED
#endif

#if (netdiag)
    #define NDIAG_ENABLED
#endif


#endif