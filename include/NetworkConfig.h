/**
 *  © 2020, Gregor Baues. All rights reserved.
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

/** some generated mac addresses as EthernetShields don't have one by default in HW.
 * Sometimes they come on a sticker on the EthernetShield then use this address otherwise
 * just choose one from below or generate one yourself. Only condition is that there is no 
 * other device on your network with the same Mac address.
 * 
 * 52:b8:8a:8e:ce:21
 * e3:e9:73:e1:db:0d
 * 54:2b:13:52:ac:0c
 * c2:d8:d4:7d:7c:cb
 * 86:cf:fa:9f:07:79
 */

/**
 * @brief Build configuration
 * 
 */

/**
 * @brief Network operational configuration
 * 
 */
#define LISTEN_PORT     2560                                    // default listen port for the server
#define MAC_ADDRESS     {0x52, 0xB8, 0x8A, 0x8E, 0xCE, 0x21}    // MAC address of your networking card found on the sticker on your card or take one from above
                                                                // on ESP32 this will be ignored as all ESP32 with Wifi have their own MAC
#define IP_ADDRESS      10, 0, 0, 101                           // Just in case we don't get an adress from DHCP try a static one; 10.x.y.z as 192.168.x.y are 
                                                                // private network adress ranges available

/**
 * @brief NetworkInterface configuration
 * 
 */
#define MAX_INTERFACES  4                                       // Consume too much memory beyond in general not more than 2 should be required
#define MAX_SOCK_NUM    8                                       // Maximum number of sockets allowed for any WizNet based EthernetShield. The W5100 only supports 4
#define MAX_WIFI_SOCK   4       
                                // ESP32 WiFi library states 4 
#ifdef ESP32
    #define MAX_ETH_BUFFER  512  // maximum length we read in one go from a TCP packet. 512 is for ESP32 devices
#else    
    #define MAX_ETH_BUFFER  128  // maximum length we read in one go from a TCP packet. 128 is for Arduinpo devices
#endif 
               
#define MAX_OVERFLOW    MAX_ETH_BUFFER / 2                      // length of the overflow buffer to be used for a given connection.
#define MAX_JMRI_CMD    MAX_ETH_BUFFER / 2                      // MAX Length of a JMRI Command
#define OUTBOUND_RING_SIZE 2048



/**
 * @todo - MAC address automation
 * @todo - Wifi setup process in case no permanent setup yet done
 * @todo - RingBuffer hack to be reviewed
 * 
 */