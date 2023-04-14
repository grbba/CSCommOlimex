/*
 *  © 2020, Harald Barth
 *  © 2023, Gregor Baues
 *
 *  This file is part of Asbelos DCC-EX
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

#include "freeMemory.h"
#include <Arduino.h>

// thanks go to  https://github.com/mpflaga/Arduino-MemoryFree
#if defined(__arm__)
extern "C" char *sbrk(int);
#elif defined(__AVR__)
extern char *__brkval;
extern char *__malloc_heap_start;
#elif defined(ESP32)
// do nothing
#else
#error Unsupported board type
#endif

int freeMemory()
{
	char top;
#if defined(__arm__)
	return &top - reinterpret_cast<char *>(sbrk(0));
#elif defined(__AVR__)
	return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#elif defined(ESP32)
	return ESP.getFreeHeap();
#else
#error bailed out alredy above
#endif
}

/*
The chipId portion is based on the GetChipID example sketch which is installed in the Arduino IDE once ESP32 board support is added.
*/

void printStats()
{

	uint32_t chipId = 0; // Holds the CPU ID, e.g. 12734324.

	for (int i = 0; i < 17; i = i + 8)
	{
		chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
	}

	// Most of the ESP.get* functions are defined in esp.h for the appropriate ESP32 chip.
	Serial.printf("ESP32 Chip model: %s\n", ESP.getChipModel());
	Serial.printf("  Revision: %d\n", ESP.getChipRevision());
	Serial.printf("  Core count: %d\n", ESP.getChipCores());
	Serial.printf("  Chip ID: %u\n", chipId);
	Serial.print("  Frequency: ");
	Serial.println(ESP.getCpuFreqMHz());
	Serial.printf("  Cycle count: %u\n", ESP.getCycleCount());
	Serial.printf("  SDK version: %s\n", ESP.getSdkVersion());

	Serial.println("Heap: ");
	Serial.printf("  Total: %u\n", ESP.getHeapSize());
	Serial.printf("  Used: %u\n", ESP.getHeapSize() - ESP.getFreeHeap());
	Serial.printf("  Free: %u\n", ESP.getFreeHeap());
	Serial.printf("  Largest block: %u\n", ESP.getMaxAllocHeap());
	Serial.printf("  Minimum free since boot: %u\n", ESP.getMinFreeHeap());

	Serial.println("Flash: ");
	Serial.printf("  Total: %u\n", ESP.getFlashChipSize());
	Serial.printf("  Speed: %u\n", ESP.getFlashChipSpeed());

	// None of the boards I tried supported these functions.
	// Serial.println( "?Magic? Flash: " );
	// Serial.printf( "  Total: %u\n", ESP.magicFlashChipSize() );
	// Serial.printf( "  Speed: %u\n", ESP.magicFlashChipSpeed() );

	Serial.println("Sketch: ");
	Serial.printf("  Size: %u\n", ESP.getSketchSize());
	Serial.printf("  Free: %u\n", ESP.getFreeSketchSpace());

	if (psramFound())
	{
		Serial.println("PSRAM: ");
		Serial.printf("  Total: %u\n", ESP.getPsramSize());
		Serial.printf("  Used: %u\n", ESP.getPsramSize() - ESP.getFreePsram());
		Serial.printf("  Free: %u\n", ESP.getFreePsram());
		Serial.printf("  Largest block: %u\n", ESP.getMaxAllocPsram());
		Serial.printf("  Minimum free since boot: %u\n", ESP.getMinFreePsram());
	}
	else
		Serial.println("This device lacks PSRAM.");
} // End of printStats() function.
