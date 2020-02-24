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

#define HOLDING_BUFFER_SIZE 248

#define RFM95_CS 10
#define RFM95_RST 9
#define RFM95_INT 2

#define RF95_FREQ 915.0

//  This currently works out to 251  (255 buffer size - 4 byte header)
#define MAX_MESSAGE_SIZE_RH RH_RF95_MAX_MESSAGE_LEN

RH_RF95 radio(RFM95_CS, RFM95_INT);

StreamParser parser(&Serial, START_OF_PACKET, END_OF_PACKET, handleSerial);

uint32_t lastFlushTime;
uint32_t maxFlushInterval = 10000;

uint8_t holdingBuffer[HOLDING_BUFFER_SIZE];
uint8_t holdingSize = 0;

//boolean flushOnNextRaw = true;

void setup() {
	pinMode(RFM95_RST, OUTPUT);
	digitalWrite(RFM95_RST, HIGH);

	parser.setRawCallback(handleSerialRaw);

	Serial.begin(115200);
	delay(100);

	DEBUG("Arduino RH-01 UNO Test!");

	// manual reset
	digitalWrite(RFM95_RST, LOW);
	delay(10);
	digitalWrite(RFM95_RST, HIGH);
	delay(10);

	while (!radio.init()) {
		DEBUG("LoRa radio init failed");
		while (1)
			;
	}
	DEBUG("LoRa radio init OK!");

	// Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
	if (!radio.setFrequency(RF95_FREQ)) {
		DEBUG("setFrequency failed");
		while (1)
			;
	}
	DEBUG("Set Freq to: ");
	DEBUG(RF95_FREQ);

	radio.setTxPower(23, false);

	DEBUG("Arduino RH-01 UNO Test!");
}

void loop() {
	listenToRadio();
	parser.run();
	if (holdingSize == 0) {
		lastFlushTime = millis(); // don't start timer if we don't have anything to send.
	}
	if (millis() - lastFlushTime >= maxFlushInterval) {
		flush();
	}
}

void listenToRadio() {
	if (radio.available()) {

		uint8_t buf[MAX_MESSAGE_SIZE_RH] = { 0 };
		uint8_t len = sizeof(buf);

		if (radio.recv(buf, &len)) {
			processRadioBuffer(buf, len);
		}
	}
}

void processRadioBuffer(uint8_t *aBuf, uint8_t aLen) {

	static boolean receiving = false;
	static char commandBuffer[100];
	static int index;

//	flushOnNextRaw = true;
	uint8_t len = aLen;
	if (len > MAX_MESSAGE_SIZE_RH) {
		len = MAX_MESSAGE_SIZE_RH;
	}

	// radio.racv doesn't put any null terminator, so we can't use
	// string functions, have to scroll through and pick stuff out.
	for (int i = 0; i < len; i++) {
		char c = aBuf[i];

		if (c == START_OF_PACKET) {
			if ((aBuf[i + 1] >= 0x11) && (aBuf[i + 1] <= 0x14)) {
				handleRawRadio(&aBuf[i]);
				i += (aBuf[i + 2] - 1);
				continue;
			}
			receiving = true;
			index = 0;
			commandBuffer[0] = 0;
		}
		if (receiving) {
			commandBuffer[index] = c;
			commandBuffer[++index] = 0;
			if (index >= 100) {
				index--;
			}
			if (c == END_OF_PACKET) {
				receiving = false;
				handleRadioCommand(commandBuffer);
			}
		}
	}
}

void handleRadioCommand(char *p) {
	if (p[1] == 'l') {
		handleConfigString(p);
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

void addToHolding(uint8_t *p, uint8_t aSize) {
	if (HOLDING_BUFFER_SIZE - holdingSize < aSize) {
		//  Not enough room so clear the buffer now
		flush();
	}
	memcpy(holdingBuffer + holdingSize, p, aSize);
	holdingSize += aSize;
}

void addToHolding(char *p) {
	addToHolding((uint8_t*) p, strlen(p));
}

void sendToRadio(char *p) {
	sendToRadio((uint8_t*) p, strlen(p));
}

void sendToRadio(uint8_t *p, uint8_t aSize) {
	radio.send(p, aSize);
	radio.waitPacketSent();
}

void flush() {
	sendToRadio(holdingBuffer, holdingSize);
	holdingSize = 0;
	lastFlushTime = millis();
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
			delay(500);
			sendToRadio(p);
			delay(500);
			handleConfigString(p);
		} else {
			addToHolding(p);
//		sendToRadio(p);
		}
	}
}

void handleConfigString(char *p) {
	switch (p[2]) {
	case 'M': {
		// set mode 0-3 from setModemConfig
		uint8_t entry = atoi((const char*) (p + 3));
		if (entry > 3) {
			entry = 3;
		}
		switch (entry) {
		case 0:
			radio.setModemConfig(RH_RF95::Bw125Cr45Sf128);
			break;
		case 1:
			radio.setModemConfig(RH_RF95::Bw500Cr45Sf128);
			break;
		case 2:
			radio.setModemConfig(RH_RF95::Bw31_25Cr48Sf512);
			break;
		case 3:
			radio.setModemConfig(RH_RF95::Bw125Cr48Sf4096);
			break;
		default:
			radio.setModemConfig(RH_RF95::Bw125Cr45Sf128);
			break;

		}
		break;
	}
	case 'B': {
		// set bandwidth to  option: 7800,10400,15600,20800,31250,41700,62500,125000,250000,500000
		//  sketchy if below 62500 although I hear 31250 works ok sometimes
		uint32_t entry = atoi((const char*) (p + 3));
		radio.setSignalBandwidth(entry);
		break;
	}
	case 'S': {
		// set mode 0-3 from setModemConfig
		uint8_t entry = atoi((const char*) (p + 3));
		if (entry > 3) {
			entry = 3;
		}
		radio.setSpreadingFactor(entry);
		break;
	}
	case 'C': {
		// set mode 0-3 from setModemConfig
		uint8_t entry = atoi((const char*) (p + 3));
		if (entry > 3) {
			entry = 3;
		}
		radio.setCodingRate4(entry);
		break;
	}
	default:
		break;
	} // end switch
}
