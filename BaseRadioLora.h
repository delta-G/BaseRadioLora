/*

BaseRadioLora  --  runs on Arduino Nano and acts as a serial to LoRa bridge
                   for connecting to my robot
     Copyright (C) 2017  David C.

     This program is free software: you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published by
     the Free Software Foundation, either version 3 of the License, or
     (at your option) any later version.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program.  If not, see <http://www.gnu.org/licenses/>.

     */

#ifndef _RH_01_H_
#define _RH_01_H_
#include "Arduino.h"

#include <SPI.h>

//#define RH_RF95_MAX_MESSAGE_LEN 80
#include <RH_RF95.h>

#include <RobotSharedDefines.h>

#include <StreamParser.h>


void setup();
void loop();

void processRadioBuffer(uint8_t*);

void handleRawRadio(uint8_t*);

void sendToRadio(char*);
void sendToRadioRaw(char*);
void listenToRadio();

#endif /* _RH_01_H_ */
