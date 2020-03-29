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

#define DEBUG_OUT Serial

#ifdef DEBUG_OUT
#define DEBUG(x) DEBUG_OUT.println(x)
#else
#define DEBUG(x)
#endif

RH_RF95 radio(RFM95_CS, RFM95_INT);

StreamParser parser(&Serial, START_OF_PACKET, END_OF_PACKET, handleSerial);

uint8_t heartPins[3] = {3,5,6};
uint16_t heartDelay[3] = {0,0,0};
uint32_t lastHeart[3] = {0,0,0};

uint32_t pingTimerStart;
uint32_t pingTimerSent;

void heartbeat(){
	uint32_t cur = millis();
	for(uint8_t i=0; i<3; i++){
		if(heartDelay[i] == 0){
			digitalWrite(heartPins[i], LOW);
		} else if(heartDelay[i] == 1){
			digitalWrite(heartPins[i], HIGH);
		} else if(cur - lastHeart[i] >= heartDelay[i]){
			digitalWrite(heartPins[i], !digitalRead(heartPins[i]));
			lastHeart[i] = cur;
		}
	}
}

void setup() {

	initRadio();

	pinMode(3, OUTPUT);
	pinMode(5, OUTPUT);
	pinMode(6, OUTPUT);

	parser.setRawCallback(handleSerialRaw);

	Serial.begin(115200);
	delay(100);

	DEBUG("Arduino RH-01 UNO Test!");

	resetRadio();

	DEBUG("Arduino RH-01 UNO Test!");

	heartDelay[1] = 500;
//	heartDelay[2] = 100;
}

void loop() {

//	static unsigned int counter = 0;
//	if(counter % 100 == 0){
//		Serial.print("<LOOP ");
//		Serial.print(counter);
//		Serial.print(" , ");
//		Serial.print(millis());
//		Serial.print(">");
//	}
//	counter++;

	listenToRadio();
	parser.run();
	handleOutput();
	heartbeat();
//	if(counter % 100 == 0){
//		Serial.print("<END ");
//		Serial.print(counter);
//		Serial.print(" , ");
//		Serial.print(millis());
//		Serial.print(">");
//	}
//	counter++;

}



void handleRadioCommand(char *p) {
	if (p[1] == 'p') {
		uint32_t endT = millis();
		char resp[25];
		snprintf(resp, 25, "<s%ul;r%ul>", (pingTimerSent - pingTimerStart) , (endT - pingTimerStart));
		Serial.print(resp);
	}
	Serial.print(p);
}

void handleRawRadio(uint8_t *p) {

	int numBytes = p[2];
	//  If this is the data dump (with the robot LORA adding it's snr and rssi

	if ((p[1] == 0x13) && (numBytes == ROBOT_DATA_DUMP_SIZE)
			&& (p[numBytes - 1] == '>')) {
		// add our SNR and RSSI
		uint8_t snr = (uint8_t) (radio.lastSNR());
		int rs = radio.lastRssi();
		uint8_t rssi = (uint8_t) (abs(rs));
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



void handleSerialRaw(char *p) {
	uint8_t len = p[2];
	addToHolding((uint8_t*) p, len);
//	if(flushOnNextRaw){
//		flushOnNextRaw = false;
//		flush();
//	}
	flush();
//	sendToRadio((uint8_t*)p, len);
}

void handleSerial(char *p) {
	if (strcmp(p, "<FFE>") == 0) {
		DEBUG("FLUSHING ON COMMAND");
		flush();
	} else {
		if (p[1] == 'l') {
//			addToHolding(p);
//			flush();
			sendToRadio(p);
			delay(2000);
			handleConfigString(p);
		} else if (p[1] == 'P') {
			pingTimerStart = millis();
			sendToRadio(p);
			pingTimerSent = millis();
		} else {
			addToHolding(p);
//		sendToRadio(p);
		}
	}
}





///  END
