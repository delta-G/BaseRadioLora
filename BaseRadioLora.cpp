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

#include "BaseRadioLora.h"

#define MYDEBUG_OUT Serial

#ifdef MYDEBUG_OUT
#define MYDEBUG(x) MYDEBUG_OUT.println(x)
#else
#define MYDEBUG(x)
#endif

RH_RF95 radio(RFM95_CS, RFM95_INT);

StreamParser parser(&Serial, START_OF_PACKET, END_OF_PACKET, handleSerialCommand);

const uint8_t heartBeatPin = 6;
unsigned int heartBeatDelay = 100;



uint32_t pingTimerStart;
uint32_t pingTimerSent;

void heartbeat(){

	static unsigned long pm = millis();
	unsigned long cm = millis();

	if(cm - pm >= heartBeatDelay){
		digitalWrite(heartBeatPin, !digitalRead(heartBeatPin));
		pm = cm;
	}
}



void setup() {

	initRadio();


	pinMode(heartBeatPin, OUTPUT);

	parser.setRawCallback(handleRawSerial);

	Serial.begin(115200);
	delay(100);

	MYDEBUG("<Arduino RH-01 UNO Test!>");

	resetRadio();

	MYDEBUG("<Radio has Initialized.>");

	heartBeatDelay = 500;
}

void loop() {

	listenToRadio();
	parser.run();
	handleOutput();
	heartbeat();

}



void handleRadioCommand(char *p) {
	if (p[1] == 'p') {
		char resp[25];
		snprintf(resp, 25, "<s%lu;r%lu>", (pingTimerSent - pingTimerStart) , (millis() - pingTimerStart));
		Serial.print(resp);
	}
	Serial.print(p);
}

void handleRawRadio(uint8_t *p) {

	int numBytes = p[2];
	//  If this is the data dump (with the robot LORA adding it's snr and rssi

	if ((p[1] == 0x13) && (numBytes == ROBOT_DATA_DUMP_SIZE)
		/*	&& (p[numBytes - 1] == '>')*/) {
		// add our SNR and RSSI
		uint8_t snr = (uint8_t) (radio.lastSNR());
		int rs = radio.lastRssi();
		uint8_t rssi = (uint8_t) (abs(rs)); // @suppress("Function cannot be resolved")
		p[ROBOT_DATA_DUMP_SIZE - 3] = snr;
		p[ROBOT_DATA_DUMP_SIZE - 2] = rssi;

		for (int i = 0; i < numBytes; i++) {
			Serial.write(p[i]);
		}

	} else {

		//  If properly formatted message
//		Serial.print("<Proper Message>");
		if ((numBytes < 100) && (p[numBytes - 1] == '>')) {
			for (int i = 0; i < numBytes; i++) {
				Serial.write(p[i]);
			}
		}
	}

}



void handleRawSerial(char *p) {
	uint8_t len = p[2];
	addToHolding((uint8_t*) p, len);
//	if(flushOnNextRaw){
//		flushOnNextRaw = false;
//		flush();
//	}
	flush();
//	sendToRadio((uint8_t*)p, len);
}

void handleSerialCommand(char *p) {
	if (strcmp(p, "<FFE>") == 0) {
		MYDEBUG("<FLUSHING ON COMMAND>");
		flush();
	} else {
		if (p[1] == 'l') {
//			addToHolding(p);
//			flush();
			sendToRadio(p);
			delay(2000);
			handleConfigString(p);
		} else if (p[1] == 'P') {
			MYDEBUG("<PINGING RADIO>");
			pingTimerStart = millis();
			sendToRadio(p);
			pingTimerSent = millis();
		} else if (p[1] == 'r') {
			Serial.println("<Responding to Serial>");
		} else {
			addToHolding(p);
//		sendToRadio(p);
		}
	}
}





///  END
