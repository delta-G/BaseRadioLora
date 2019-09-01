#include "RH_01.h"

#define RFM95_CS 10
#define RFM95_RST 9
#define RFM95_INT 2

#define RF95_FREQ 915.0

RH_RF95 radio(RFM95_CS, RFM95_INT);

StreamParser parser(&Serial, START_OF_PACKET, END_OF_PACKET, sendToRadio);

void setup() {
	pinMode(RFM95_RST, OUTPUT);
	digitalWrite(RFM95_RST, HIGH);

	Serial.begin(115200);
	delay(100);

	Serial.println("Arduino RH-01 UNO Test!");

	// manual reset
	digitalWrite(RFM95_RST, LOW);
	delay(10);
	digitalWrite(RFM95_RST, HIGH);
	delay(10);

	while (!radio.init()) {
		Serial.println("LoRa radio init failed");
		while (1)
			;
	}
	Serial.println("LoRa radio init OK!");

	// Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
	if (!radio.setFrequency(RF95_FREQ)) {
		Serial.println("setFrequency failed");
		while (1)
			;
	}
	Serial.print("Set Freq to: ");
	Serial.println(RF95_FREQ);

	radio.setTxPower(23, false);
}

void loop()
{
	// If anything is coming in on radio, just ship it out to serial
	// the serial parsers on either end can handle it however it comes.
	if (radio.available()){

		uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
		uint8_t len = sizeof(buf);

		if(radio.recv(buf, &len)){
			//radio.recv doesn't put a null on the end of the string.
			//it sends packets of bytes.
			//we need to set up a constant size packet system.
			for(uint8_t i = 0; i < RH_RF95_MAX_MESSAGE_LEN; i++){
				if(buf[i] == '>' && buf[i+1] != '<'){
					buf[i + 1] = 0;
				}
			}
			Serial.print((char*)buf);
		}
	}

	parser.run();
}

void sendToRadio(char *p) {
//	Serial.print("Sending ");
//	Serial.println(p);
	if (p[1] == 'X') {
		controllerDataToRaw(p);
	} else {
		uint8_t len = strlen(p);
		radio.send((uint8_t*) p, len);
		radio.waitPacketSent();
	}
}

void controllerDataToRaw(char* p){

	uint8_t rawArray[16];
	rawArray[0] = '<';
	for (int i = 0; i < 14; i++){
		char temp[3] = {p[2+(2*i)], p[3+(2*i)] , 0};
		rawArray[1+i] = strtoul(temp, NULL, HEX);
	}
	rawArray[15] = '>';

	radio.send(rawArray, 16);
	radio.waitPacketSent();

}
