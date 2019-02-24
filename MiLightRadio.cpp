/*
 * MiLightRadio.cpp
 *
 *  Created on: 29 May 2015
 *      Author: henryk
 */

#include "MiLightRadio.h"

#define PACKET_ID(packet) ( ((packet[1] & 0xF0)<<24) | (packet[2]<<16) | (packet[3]<<8) | (packet[7]) )

static const uint8_t CHANNELS_V1[] = { 9, 40, 71 };
static const uint8_t CHANNELS_V2[] = { 8, 39, 70 };

#define CHANNELS ((_version == MILIGHT_V2) ? CHANNELS_V2 : CHANNELS_V1)

const unsigned NUM_CHANNELS = 3;


#define V2_OFFSET_JUMP_START 0x54

static const uint8_t V2_OFFSETS[][4] = {
  { 0x45, 0x1F, 0x14, 0x5C },
  { 0x2B, 0xC9, 0xE3, 0x11 },
  { 0xEE, 0xDE, 0x0B, 0xAA },
  { 0xAF, 0x03, 0x1D, 0xF3 },
  { 0x1A, 0xE2, 0xF0, 0xD1 },
  { 0x04, 0xD8, 0x71, 0x42 },
  { 0xAF, 0x04, 0xDD, 0x07 },
  { 0xE1, 0x93, 0xB8, 0xE4 }
};

static inline uint8_t V2_OFFSET(uint8_t byte, uint8_t key, uint8_t jumpStart)
{
	unsigned res = V2_OFFSETS[byte - 1][key % 4];
	if (jumpStart > 0 && key >= jumpStart && key <= (jumpStart + 0x80U)) {
		res += 0x80U;
	}
	return res;
}

/*
#define V2_OFFSET(byte, key, jumpStart) ( \
  V2_OFFSETS[byte-1][key%4] \
    + \
  ((jumpStart > 0 && key >= jumpStart && key <= jumpStart+0x80) ? 0x80 : 0) \
)
*/


uint8_t xorKey(uint8_t key)
{
  // Generate most significant nibble
  const uint8_t shift = (key & 0x0F) < 0x04 ? 0 : 1;
  const uint8_t x = (((key & 0xF0) >> 4) + shift + 6) % 8;
  const uint8_t msn = (((4 + x) ^ 1) & 0x0F) << 4;
  // Generate least significant nibble
  const uint8_t lsn = (((key & 0x0F) + 4) ^ 2) & 0x0F;
  return msn | lsn;
}

uint8_t decodeByte(uint8_t byte, uint8_t s1, uint8_t xorKey, uint8_t s2)
{
  uint8_t value = byte - s2;
  value ^= xorKey;
  value -= s1;
  return value;
}

uint8_t encodeByte(uint8_t byte, uint8_t s1, uint8_t xorKey, uint8_t s2)
{
  uint8_t value = byte + s1;
  value ^= xorKey;
  value += s2;
  return value;
}

void decodeV2Frame(uint8_t *frame)
{
  uint8_t key = xorKey(frame[0]);
  for (unsigned i = 1; i <= 8; i++) {
    frame[i] = decodeByte(frame[i], 0, key, V2_OFFSET(i, frame[0], V2_OFFSET_JUMP_START));
  }
}


int MiLightRadio::begin(MilightVersion version)
{
	int retval = _pl1167.open();
	if (retval < 0) {
		return retval;
	}

	retval = _pl1167.setCRC(true);
	if (retval < 0) {
		return retval;
	}

	retval = _pl1167.setPreambleLength(3);
	if (retval < 0) {
		return retval;
	}

	retval = _pl1167.setTrailerLength(4);
	if (retval < 0) {
		return retval;
	}

	if (version == MILIGHT_V2) {
		retval = _pl1167.setSyncword(0x7236, 0x1809);
	} else {
		retval = _pl1167.setSyncword(0x147A, 0x258B);
	}
	if (retval < 0) {
		return retval;
	}

	_version = version;
	retval = _pl1167.setMaxPacketLength(packetLength());
	if (retval < 0) {
		return retval;
	}

	available();

	return 0;
}

bool MiLightRadio::available()
{
	if (_waiting) {
		return true;
	}

	if (_pl1167.receive(CHANNELS[0]) > 0) {
		unsigned packet_length = packetLength();
		if (_pl1167.readFIFO(_packet, packet_length) < 0) {
			return false;
		}
		if (packet_length == 0 || packet_length != _packet[0] + 1U) {
			return false;
		}

		uint32_t packet_id = PACKET_ID(_packet);
		if (packet_id == _prev_packet_id) {
			_dupes_received++;
		}
		else {
			_prev_packet_id = packet_id;
		}
		_waiting = true;
	}

	return _waiting;
}

int MiLightRadio::read(uint8_t frame[], size_t &frame_length)
{
	if (!_waiting) {
		frame_length = 0;
		return -1;
	}

	if (frame_length > frameLength()) {
		frame_length = frameLength();
	}

	if (frame_length > _packet[0]) {
		frame_length = _packet[0];
	}


	memcpy(frame, _packet + 1, frame_length);
	_waiting = false;

	if (_version == MILIGHT_V2) {
		decodeV2Frame(frame);
	}

	return _packet[0];
}

int MiLightRadio::write(uint8_t frame[], size_t frame_length)
{
	if (frame_length > frameLength()) {
		return -1;
	}

	memcpy(_out_packet + 1, frame, frame_length);
	_out_packet[0] = frame_length;

	int retval = resend();
	if (retval < 0) {
		return retval;
	}
	return frame_length;
}

int MiLightRadio::resend()
{
	for (unsigned i = 0; i < NUM_CHANNELS; i++) {
		_pl1167.writeFIFO(_out_packet, _out_packet[0] + 1);
		_pl1167.transmit(CHANNELS[i]);
	}
	return 0;
}
