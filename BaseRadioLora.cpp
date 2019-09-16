#include "BaseRadioLora.h"

#define RFM95_CS 10
#define RFM95_RST 9
#define RFM95_INT 2

#define RF95_FREQ 915.0

#define MAX_MESSAGE_SIZE_RH RH_RF95_MAX_MESSAGE_LEN


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
	listenToRadio();

	reportSignalStrength();

	parser.run();
}


void listenToRadio() {
	if (radio.available()) {

		uint8_t buf[MAX_MESSAGE_SIZE_RH];
		uint8_t len = sizeof(buf);

		if (radio.recv(buf, &len)) {
			processRadioBuffer(buf);
		}
	}

}

void reportSignalStrength() {

	static uint32_t pm = millis();
	uint32_t cm = millis();

	if (cm - pm >= 5000) {
		pm = cm;
		Serial.print("<SNR,");
		Serial.print(radio.lastSNR());
		Serial.print("_,_RSSI,");
		Serial.print(radio.lastRssi());
		Serial.print(">");
	}
}

void processRadioBuffer(uint8_t *aBuf) {

	static boolean receiving = false;
	static char commandBuffer[100];
	static int index;

	// radio.racv doesn't put any null terminator, so we can't use
	// string functions, have to scroll through and pick stuff out.
	for (int i = 0; i < MAX_MESSAGE_SIZE_RH; i++) {
		char c = aBuf[i];

		if (c == START_OF_PACKET) {
			if ((aBuf[i + 1] >= 0x11) && (aBuf[i + 1] <= 0x13)) {
				handleRawData(&aBuf[i]);
				i += (aBuf[i+2] -1);
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
				Serial.print(commandBuffer);
			}
		}
	}
}

void handleRawData(uint8_t* p){

//	Serial.print("<Raw_Dump>");
	int numBytes = p[2];

	for(int i=0; i<numBytes; i++){
		Serial.write(p[i]);
	}
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


