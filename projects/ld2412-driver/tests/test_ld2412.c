#include "ld2412.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint8_t tx_buffer[128];
static uint16_t tx_length;
static uint32_t ack_count;
static uint32_t report_count;
static ld2412_ack_t last_ack;
static ld2412_report_t last_report;

static void capture_tx(const uint8_t *data, uint16_t length)
{
    assert(length <= sizeof(tx_buffer));
    memcpy(tx_buffer, data, length);
    tx_length = length;
}

static void capture_ack(const ld2412_ack_t *ack)
{
    last_ack = *ack;
    ack_count++;
}

static void capture_report(const ld2412_report_t *report)
{
    last_report = *report;
    report_count++;
}

static void feed_bytes(ld2412_t *device, const uint8_t *data, size_t length)
{
    size_t index;

    for (index = 0u; index < length; index++) {
        ld2412_input_byte(device, data[index]);
    }
}

static void reset_captures(void)
{
    memset(tx_buffer, 0, sizeof(tx_buffer));
    memset(&last_ack, 0, sizeof(last_ack));
    memset(&last_report, 0, sizeof(last_report));
    tx_length = 0u;
    ack_count = 0u;
    report_count = 0u;
}

static void test_enter_config_frame(void)
{
    static const uint8_t expected[] = {
        0xFDu, 0xFCu, 0xFBu, 0xFAu,
        0x04u, 0x00u,
        0xFFu, 0x00u,
        0x01u, 0x00u,
        0x04u, 0x03u, 0x02u, 0x01u
    };
    ld2412_t device;

    reset_captures();
    ld2412_init(&device, capture_tx);

    assert(ld2412_enter_config(&device));
    assert(tx_length == sizeof(expected));
    assert(memcmp(tx_buffer, expected, sizeof(expected)) == 0);
}

static void test_ack_parsing(void)
{
    static const uint8_t frame[] = {
        0xFDu, 0xFCu, 0xFBu, 0xFAu,
        0x06u, 0x00u,
        0xA0u, 0x01u,
        0x00u, 0x00u,
        0x12u, 0x34u,
        0x04u, 0x03u, 0x02u, 0x01u
    };
    ld2412_t device;

    reset_captures();
    ld2412_init(&device, capture_tx);
    ld2412_set_ack_callback(&device, capture_ack);
    feed_bytes(&device, frame, sizeof(frame));

    assert(ack_count == 1u);
    assert(last_ack.command == LD2412_CMD_GET_VERSION);
    assert(last_ack.status == 0u);
    assert(last_ack.payload_len == 2u);
    assert(last_ack.payload[0] == 0x12u);
    assert(last_ack.payload[1] == 0x34u);
}

static void test_basic_report_and_resync(void)
{
    static const uint8_t noise[] = {0x00u, 0xAAu, 0xFDu};
    static const uint8_t frame[] = {
        0xF4u, 0xF3u, 0xF2u, 0xF1u,
        0x0Du, 0x00u,
        0x02u, 0xAAu, 0x01u,
        0x7Bu, 0x00u, 0x50u,
        0xC8u, 0x01u, 0x40u,
        0x2Cu, 0x01u,
        0x55u, 0x00u,
        0xF8u, 0xF7u, 0xF6u, 0xF5u
    };
    ld2412_t device;

    reset_captures();
    ld2412_init(&device, capture_tx);
    ld2412_set_report_callback(&device, capture_report);
    feed_bytes(&device, noise, sizeof(noise));
    feed_bytes(&device, frame, sizeof(frame));

    assert(report_count == 1u);
    assert(last_report.state == LD2412_TARGET_MOVING);
    assert(last_report.moving_distance_cm == 123u);
    assert(last_report.moving_energy == 80u);
    assert(last_report.static_distance_cm == 456u);
    assert(last_report.static_energy == 64u);
    assert(!last_report.engineering_mode);
}

static void test_invalid_lengths_are_rejected(void)
{
    static const uint8_t malformed[] = {
        0xF4u, 0xF3u, 0xF2u, 0xF1u, 0xFFu, 0xFFu, 0x00u
    };
    static const uint8_t valid_ack[] = {
        0xFDu, 0xFCu, 0xFBu, 0xFAu,
        0x04u, 0x00u,
        0xFEu, 0x01u,
        0x00u, 0x00u,
        0x04u, 0x03u, 0x02u, 0x01u
    };
    uint8_t payload = 0u;
    ld2412_t device;

    reset_captures();
    ld2412_init(&device, capture_tx);
    ld2412_set_ack_callback(&device, capture_ack);

    feed_bytes(&device, malformed, sizeof(malformed));
    feed_bytes(&device, valid_ack, sizeof(valid_ack));
    assert(ack_count == 1u);
    assert(last_ack.command == LD2412_CMD_EXIT_CONFIG);

    tx_length = 0u;
    assert(!ld2412_send_command(&device, 0x1234u, &payload, UINT16_MAX));
    assert(tx_length == 0u);
}

int main(void)
{
    test_enter_config_frame();
    test_ack_parsing();
    test_basic_report_and_resync();
    test_invalid_lengths_are_rejected();

    puts("ld2412 tests: PASS");
    return 0;
}
