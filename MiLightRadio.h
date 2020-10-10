/*
 * MiLightRadio.h
 *
 *  Created on: 29 May 2015
 *      Author: henryk
 */

#pragma once

#include "AbstractPL1167.h"

const unsigned MAX_PACKET_LEN{10};

enum MilightVersion {
	MILIGHT_UNKNOWN,
	MILIGHT_V1,
	MILIGHT_V2,
};

class MiLightRadio
{
public:
	MiLightRadio(AbstractPL1167& pl1167) : _pl1167(pl1167)
	{
	}

	int begin(MilightVersion version);
	bool available();
	int read(uint8_t frame[], size_t& frame_length);

	unsigned dupesReceived()
	{
		return _dupes_received;
	}

	int write(uint8_t frame[], size_t frame_length);
	int resend();

private:
	uint8_t frameLength() const
	{
		return (_version == MILIGHT_V2) ? 9 : 7;
	}

	uint8_t packetLength() const
	{
		return frameLength() + 1;
	}

private:
	AbstractPL1167& _pl1167;
	uint32_t _prev_packet_id{0};

	uint8_t _packet[MAX_PACKET_LEN];
	uint8_t _out_packet[MAX_PACKET_LEN];
	MilightVersion _version{MILIGHT_UNKNOWN};
	bool _waiting{false};
	unsigned _dupes_received{0};
};
