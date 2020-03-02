/*

 RadioCommon  --  runs on Arduino Nano and acts as a serial to LoRa bridge
 for connecting to my robot  The common parts of RobotRadioLora and
 BaseRadioLora
 Copyright (C) 2020  David C.

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

#ifndef RADIOCOMMON_H_
#define RADIOCOMMON_H_

#include "Arduino.h"

#include <SPI.h>


#include <RH_RF95.h>

#include <RobotSharedDefines.h>

#define DEBUG_OUT Serial

#ifdef DEBUG_OUT
#define DEBUG(x) DEBUG_OUT.println(x)
#else
#define DEBUG(x)
#endif

#define HOLDING_BUFFER_SIZE 248

#define RFM95_CS 10
#define RFM95_RST 9
#define RFM95_INT 2

#define RF95_FREQ 915.0

//  This currently works out to 251  (255 buffer size - 4 byte header)
#define MAX_MESSAGE_SIZE_RH RH_RF95_MAX_MESSAGE_LEN

void initRadio();

void listenToRadio();
void handleOutput();
void processRadioBuffer(uint8_t*, uint8_t);

void addToHolding(uint8_t*, uint8_t);
void addToHolding(char*);
void sendToRadio(uint8_t*, uint8_t);
void sendToRadio(char*);
void flush();

void handleConfigString(char*);
void resetRadio();




#endif /* RADIOCOMMON_H_ */
