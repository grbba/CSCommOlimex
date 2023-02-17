/**
 * @file AT1021.h
 * @author Gregor Baues
 * @brief Touch driver for the AR1021 on the Olimex tft board
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

#ifndef AR1021_h
#define AR1021_h

#include <Wire.h>

#define AR1021_ADDRESS 0x4D // I2C address of the AR1021 touch screen controller

// Registers for the AR1021 touch screen controller
#define TOUCH_XH 0x03
#define TOUCH_XL 0x04
#define TOUCH_YH 0x05
#define TOUCH_YL 0x06
#define TOUCH_STATUS 0x1E

#define STMPE_TSC_CTRL 0x40

class AR1021
{
public:
  static void begin()
  {
    Wire.begin();
  }
  static bool isTouched()
  {
    Wire.beginTransmission(AR1021_ADDRESS);
    Wire.write(TOUCH_STATUS);
    Wire.endTransmission();

    Wire.requestFrom(AR1021_ADDRESS, 1);
    if (Wire.available())
    {
      return Wire.read() & 0x01;
    }
    return false;
  }

  static bool isTouched2() {
     return (readRegister8(STMPE_TSC_CTRL) & 0x80);
  }

  static uint8_t readRegister8(uint8_t reg) {
  uint8_t x;
  // use i2c
  Wire.beginTransmission(AR1021_ADDRESS);
  Wire.write((byte)reg);
  Wire.endTransmission();
  Wire.beginTransmission(AR1021_ADDRESS);
  Wire.requestFrom(AR1021_ADDRESS, 1);
  x = Wire.read();
  Wire.endTransmission();

  // Serial.print("$"); Serial.print(reg, HEX);
  // Serial.print(": 0x"); Serial.println(x, HEX);
  return x;

  }

  static int16_t getX()
  {
    uint8_t xh, xl;

    Wire.beginTransmission(AR1021_ADDRESS);
    Wire.write(TOUCH_XH);
    Wire.endTransmission();

    Wire.requestFrom(AR1021_ADDRESS, 2);
    if (Wire.available() >= 2)
    {
      xh = Wire.read();
      xl = Wire.read();
      return ((xh << 8) | xl);
    }
    return -1;
  }
  static int16_t getY()
  {
    uint8_t yh, yl;

    Wire.beginTransmission(AR1021_ADDRESS);
    Wire.write(TOUCH_YH);
    Wire.endTransmission();

    Wire.requestFrom(AR1021_ADDRESS, 2);
    if (Wire.available() >= 2)
    {
      yh = Wire.read();
      yl = Wire.read();
      return ((yh << 8) | yl);
    }
    return -1;
  }

  AR1021() {}
};

#endif

// void AR1021::loop() {
//   if (this.isTouched()) {
//     int16_t x = this.getX();
//     int16_t y = this.getY();
//     Serial.print("Touch detected at ");
//     Serial.print(x);
//     Serial.print(", ");
//     Serial.println(y);
//   }
// }

/*
#define STMPE_TSC_CTRL 0x40

boolean Adafruit_STMPE610::touched(void) {
  return (readRegister8(STMPE_TSC_CTRL) & 0x80);
}
*/

