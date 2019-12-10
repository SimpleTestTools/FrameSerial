#include "FrameSerial.h"

// Configuration
#define LED_DBG_PIN         (13)
#define FRAME_SIZE_BYTES    (128)

enum PacketType
{
    PACKET_TYPE_NONE = 0,
    PACKET_TYPE_CMD,
    PACKET_TYPE_CMD_RESP,
    PACKET_TYPE_DATA,
};

// Define a data frame of size FRAME_SIZE_BYTES
const size_t SAMPLE_COUNT = (FRAME_SIZE_BYTES / sizeof(uint32_t)) - 1;
struct packet_data_s
{
    uint32_t timestamp;
    uint32_t adc[SAMPLE_COUNT];
};

FrameSerial s;

void setup()
{
    // FrameSerial wraps Serial by default
    s.begin(115200);
    s.setPacketHandler(&handlePacket);

#if defined(LED_DBG_PIN)
    digitalWrite(LED_DBG_PIN, LOW);
    pinMode(LED_DBG_PIN, OUTPUT);
#endif
}

// Local state
static bool test_running = false;
static uint32_t packet_frequency;
static unsigned long last_micros;
static struct packet_data_s data;

void handlePacket(const uint8_t *buf, size_t len)
{
    // Decode the frame from the PacketSerial buffer
    struct frame_s frame = { 0 };
    if (!s.decodePacket(buf, len, &frame)) {
        return;
    }

    // Sanity check the received frame
    if ((frame.type != PACKET_TYPE_CMD) || (frame.size < 1)) {
        return;
    }

    char tmp[64];
    const unsigned char cmd = frame.data[0];
    switch (cmd)
    {
    case 't':
        packet_frequency = atoi((const char*)&(frame.data[1]));
        if (packet_frequency > 0)
        {
            test_running = true;
            last_micros = micros();
            snprintf(tmp, sizeof(tmp), "$TRANSMIT packet_frequency=%ld", packet_frequency);
            s.sendFrame(PACKET_TYPE_CMD_RESP, (uint8_t*)tmp, strlen(tmp));
        }
        else
        {
            test_running = false;
            snprintf(tmp, sizeof(tmp), "$NONE");
            s.sendFrame(PACKET_TYPE_CMD_RESP, (uint8_t*)tmp, strlen(tmp));
        }
        break;

    case 'x':
        test_running = false;
        snprintf(tmp, sizeof(tmp), "$NONE");
        s.sendFrame(PACKET_TYPE_CMD_RESP, (uint8_t*)tmp, strlen(tmp));
        break;

    default:
        break;
    }
}


void loop()
{
    // Service the serial connection
    s.update();

#if defined(LED_DBG_PIN)
    digitalWrite(LED_DBG_PIN, test_running);
#endif

    if (test_running)
    {
        if ((micros() - last_micros) > (1000000 / packet_frequency))
        {
            data.timestamp = micros();
            data.adc[0] = 0xFF00FF00;

            s.sendFrame(PACKET_TYPE_DATA, (const uint8_t*)&data, sizeof(data));

            last_micros = micros();
        }
    }
}
