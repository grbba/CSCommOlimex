/**
 * @file DCSIDisplay.cpp
 * @author Gregor Baues
 * @brief Driving the TFT 2.8 display on the NetworksStation
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
 */

#include <Arduino.h>
#include <DCSIconfig.h>
#include <DCSIlog.h>
#include <DCSIDisplay.h>

#include <SPI.h>
#include <TFT_eSPI.h>

/**
 * @brief Initalize the TFT display and show the splash screen
 * 
 */
void DCSIDisplay::setup() {
    INFO(F("Screen setup" CR));
    screen.begin();
    screen.setRotation(1);
    screen.fillScreen(TFT_DARKGREY);
    screen.setTextFont(2);
}

DCSIDisplay display = DCSIDisplay();