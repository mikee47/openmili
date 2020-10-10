/*
 * PL1167_nRF24.h
 *
 *  Created on: 29 May 2015
 *      Author: henryk
 */

#pragma once

#include "AbstractPL1167.h"
#include <nRF24/RF24.h>

class PL1167_nRF24 : public AbstractPL1167
{
public:
	PL1167_nRF24(RF24& radio) : _radio(radio)
	{
	}

	int open();
	int setPreambleLength(uint8_t preambleLength);
	int setSyncword(uint16_t syncword0, uint16_t syncword3);
	int setTrailerLength(uint8_t trailerLength);
	int setCRC(bool crc);
	int setMaxPacketLength(uint8_t maxPacketLength);
	int writeFIFO(const uint8_t data[], size_t data_length);
	int transmit(uint8_t channel);
	int receive(uint8_t channel);
	int readFIFO(uint8_t data[], size_t& data_length);

private:
	int recalc_parameters();
	int internal_receive();

	RF24& _radio;

	bool _crcEnabled{true};
	uint8_t _preambleLength{1};
	uint16_t _syncword0{0}, _syncword3{0};
	uint8_t _syncwordLength{4};
	uint8_t _trailerLength{4};
	uint8_t _maxPacketLength{8};

	uint8_t _channel{0};

	uint8_t _nrf_pipe[5];
	uint8_t _nrf_pipe_length{0};

	uint8_t _packet_length{0};
	uint8_t _receive_length{0};
	uint8_t _preamble{0};
	uint8_t _packet[32];
	bool _received{false};
};
