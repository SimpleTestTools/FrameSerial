#ifndef __FRAME_SERIAL_H__
#define __FRAME_SERIAL_H__

#include "PacketSerial.h"

#define HEADER_SIZE (4)
#define CRC32_SIZE (4)

struct frame_s
{
    uint16_t type;
    size_t size;
    const uint8_t *data;
};

template<typename EncoderType, uint8_t PacketMarker = 0, size_t ReceiveBufferSize = 256>
class FrameSerial_ : public PacketSerial_<EncoderType, PacketMarker, ReceiveBufferSize>
{
public:
    /*
     * Wrap the buffer in a frame and send.
     */
    void sendFrame(uint16_t frame_type, const uint8_t* buffer, size_t size) const
    {
        // Allocate buffer to store frame + data
        uint8_t framed_buf[HEADER_SIZE + size + CRC32_SIZE];

        // Put at start of buffer, little endian
        framed_buf[0] = frame_type & 0xFF;
        framed_buf[1] = (frame_type >> 8) & 0xFF;
        framed_buf[2] = size & 0xFF;
        framed_buf[3] = (size >> 8) & 0xFF;

        // Copy in data
        memcpy(framed_buf + HEADER_SIZE, buffer, size);

        // Compute CRC32
        uint32_t crc32 = 0xDEADBEEF; // TODO(jott): compute

        // Place at end of buffer, little endian
        framed_buf[HEADER_SIZE + size + 0] = crc32 & 0xFF;
        framed_buf[HEADER_SIZE + size + 1] = (crc32 >> 8) & 0xFF;
        framed_buf[HEADER_SIZE + size + 2] = (crc32 >> 16) & 0xFF;
        framed_buf[HEADER_SIZE + size + 3] = (crc32 >> 24) & 0xFF;

        // Send via original method
        PacketSerial_<EncoderType, PacketMarker, ReceiveBufferSize>::send(framed_buf, sizeof(framed_buf));
    }

    bool decodePacket(const uint8_t *buf, size_t len, struct frame_s *pkt)
    {
        // Validate CRC32
        const uint32_t packet_crc32 = (
                (buf[len - 4]) +
                (buf[len - 3] << 8) +
                (buf[len - 2] << 16) +
                (buf[len - 1] << 24)
            );

        uint32_t crc = 0xDEADBEEF; // TODO(jott): compute

        if (packet_crc32 != crc)
        {
            return false; // Invalid CRC32
        }

        // Decode header, little endian
        const uint16_t frame_type = buf[0] + (buf[1] << 8);
        const size_t frame_size = buf[2] + (buf[3] << 8);

        if ((HEADER_SIZE + frame_size + CRC32_SIZE) != len)
        {
            return false; // Frame size doesn't match packet size
        }

        // Fill in frame data
        pkt->type = frame_type;
        pkt->size = frame_size;
        pkt->data = &(buf[HEADER_SIZE]);

        return true;
    }
};


/// A default COBS FrameSerial class.
typedef FrameSerial_<COBS> FrameSerial;

#endif // __FRAME_SERIAL_H__
