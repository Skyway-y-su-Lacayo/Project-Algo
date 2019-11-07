#pragma once

struct InputPacketData
{
	uint32 sequenceNumber = 0;
	real32 horizontalAxis = 0.0f;
	real32 verticalAxis = 0.0f;
	uint16 buttonBits = 0;
};

uint16 packInputControllerButtons(const InputController &input);

void unpackInputControllerButtons(uint16 buttonBits, InputController &input);
