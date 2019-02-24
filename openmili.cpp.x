/**
 * On a Raspberry Pi 2 compile with:
 *
 * g++ -Ofast -mfpu=vfp -mfloat-abi=hard -march=armv7-a -mtune=arm1176jzf-s -I/usr/local/include -L/usr/local/lib -lrf24-bcm PL1167_nRF24.cpp MiLightRadio.cpp openmili.cpp -o openmilight
 *
 * for receiver mode run with:
 * sudo ./openmilight
 *
 * for sender mode run with:
 * sudo ./openmilight "B0 F2 EA 6D B0 02 f0"
 *
 *
 *
 * Version 1
 *
 * CH1 ON: 	B0 17 57 3A 01 03 12
 * CH1 OFF: B0 17 57 3A 01 04 13
 *
 *
 */

#include <cstdlib>
#include <iostream>
#include <string.h>

using namespace std;

#include <RF24/RF24.h>

#include "PL1167_nRF24.h"
#include "MiLightRadio.h"

RF24 radio(RPI_V2_GPIO_P1_11, RPI_V2_GPIO_P1_24, BCM2835_SPI_SPEED_1MHZ);

PL1167_nRF24 prf(radio);
MiLightRadio mlr(prf);

static enum {
	IDLE,
	HAVE_NIBBLE,
	COMPLETE,
} state;

void receiveLoop()
{
	printf("Listening mode\n");

	while (true) {
		if (mlr.available()) {
			printf("\n");
			uint8_t packet[MAX_PACKET_LEN];
			size_t packet_length = sizeof(packet);
			mlr.read(packet, packet_length);

			for (unsigned i = 0; i < packet_length; i++) {
				printf("%02X ", packet[i]);
			}
		}

		unsigned dupesReceived = mlr.dupesReceived();
		static unsigned dupesPrinted;
		for (; dupesPrinted < dupesReceived; dupesPrinted++) {
			printf(".");
		}
	}
}

void transmit(const char* packetBytes)
{
	printf("sending: %s\n", packetBytes);

	uint8_t outgoingPacket[MAX_PACKET_LEN];
	memset(outgoingPacket, 0, sizeof(outgoingPacket));

	// convert input into hex
	unsigned index = 0;
	for (unsigned counter = 0; *packetBytes; ++packetBytes) {
		int n = 0;
		if (*packetBytes >= 'a' && *packetBytes <= 'f') {
			n = *packetBytes - 'a' + 10;
		}
		else if (*packetBytes >= 'A' && *packetBytes <= 'F') {
			n = *packetBytes - 'A' + 10;
		}
		else if (*packetBytes >= '0' && *packetBytes <= '9') {
			n = *packetBytes - '0';
		}
		else if (*packetBytes == ' ') {
			index++;
		}
		else {
			cout << "cannot decode" << endl;
			exit(1);
		}
		outgoingPacket[index] = outgoingPacket[index] * 16 + unsigned(n);
	}
	unsigned packetLength = index + 1;

	printf("sending packet\n");
	for (unsigned i = 0; i < packetLength; i++) {
		printf("%02X ", outgoingPacket[i]);
	}
	printf("\n");

	mlr.write(outgoingPacket, packetLength);
	for (unsigned repeat = 1; repeat <= 3; ++repeat) {
		delay(50);
		mlr.resend();
	}
}

int main(int argc, char** argv)
{
	char* packetBytes = 0;

	MilightVersion version = MILIGHT_V1;

	unsigned argnum = 1;

	// Optional, first argument may be "v1" or "v2"
	if (argc >= 2 && tolower(argv[1][0]) == 'v') {
		version = MilightVersion(argv[1][1] - '0');
		++argnum;

		if (version < MILIGHT_V1 || version > MILIGHT_V2) {
			printf("Please specify version 1 or 2\n");
			return 1;
		}
	}

	mlr.begin(version);
	printf("Using protocol version %u\n", version);

	if (argc <= argnum) {
		receiveLoop();
	} else {
		transmit(argv[argnum]);
	}

	return 0;
}
