#ifndef _RH_01_H_
#define _RH_01_H_
#include "Arduino.h"

#include <SPI.h>
#include <RH_RF95.h>

#include <RobotSharedDefines.h>

#include <StreamParser.h>


void setup();
void loop();

void sendToRadio(char*);

void controllerDataToRaw(char*);

#endif /* _RH_01_H_ */
