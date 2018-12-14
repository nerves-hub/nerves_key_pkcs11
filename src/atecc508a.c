/*
 * Copyright (c) 2018 Frank Hunleth
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "atecc508a.h"

#define DEBUG
#ifdef DEBUG
#include <stdio.h>
#define INFO(fmt, ...) \
        do { fprintf(stderr, "%s: " fmt "\r\n", "atecc508a", ##__VA_ARGS__); } while (0)
#else
#define INFO(fmt, ...)
#endif

#define ATECC508A_ADDR 0x60
#define ATECC508A_WAKE_DELAY_US 1500

// The ATECC508A/608A have different times for how long to wait for commands to complete.
// Unless I'm totally misreading the datasheet (Table 9-4), it really seems like those are too short.
// See https://github.com/MicrochipTech/cryptoauthlib/blob/master/lib/atca_execution.c#L98 for
// another opinion on execution times.
//
// I'm taking the "typical" time from the datasheet and the "max" time from the longest
// I see on the 508A/608A in atca_execution.c or the datasheet.
#define ATECC508A_CHECKMAC_TYPICAL_US     5000
#define ATECC508A_CHECKMAC_MAX_US        40000
#define ATECC508A_DERIVE_KEY_TYPICAL_US   2000
#define ATECC508A_DERIVE_KEY_MAX_US      50000
#define ATECC508A_ECDH_TYPICAL_US        38000
#define ATECC508A_ECDH_MAX_US           531000
#define ATECC508A_GENKEY_TYPICAL_US      11000
#define ATECC508A_GENKEY_MAX_US         653000
#define ATECC508A_NONCE_TYPICAL_US         100
#define ATECC508A_NONCE_MAX_US           29000
#define ATECC508A_READ_TYPICAL_US          100
#define ATECC508A_READ_MAX_US             5000
#define ATECC508A_SIGN_TYPICAL_US        42000
#define ATECC508A_SIGN_MAX_US           665000

static int microsleep(int microseconds)
{
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = microseconds * 1000;
    int rc;

    while ((rc = nanosleep(&ts, &ts)) < 0 && errno == EINTR);

    return rc;
}

static int i2c_read(int fd, uint8_t addr, uint8_t *to_read, size_t to_read_len)
{
    struct i2c_rdwr_ioctl_data data;
    struct i2c_msg msg;

    msg.addr = addr;
    msg.flags = I2C_M_RD;
    msg.len = (uint16_t) to_read_len;
    msg.buf = to_read;
    data.msgs = &msg;
    data.nmsgs = 1;

    return ioctl(fd, I2C_RDWR, &data);
}

static int i2c_write(int fd, uint8_t addr, const uint8_t *to_write, size_t to_write_len)
{
    struct i2c_rdwr_ioctl_data data;
    struct i2c_msg msg;

    msg.addr = addr;
    msg.flags = 0;
    msg.len = (uint16_t) to_write_len;
    msg.buf = (uint8_t *) to_write;
    data.msgs = &msg;
    data.nmsgs = 1;

    return ioctl(fd, I2C_RDWR, &data);
}

static int i2c_poll_read(int fd, uint8_t addr, uint8_t *to_read, size_t to_read_len, int min_us, int max_us)
{
    int amount_slept = min_us;
    int poll_interval = 1000;
    int rc;

    microsleep(min_us);

    do {
        rc = i2c_read(fd, addr, to_read, to_read_len);
        if (rc >= 0)
            break;

        microsleep(poll_interval);
        amount_slept += poll_interval;
    } while (amount_slept <= max_us);
    return rc;
}

int atecc508a_open(const char *filename)
{
    return open(filename, O_RDWR);
}

void atecc508a_close(int fd)
{
    close(fd);
}

int atecc508a_wakeup(int fd)
{
    // See ATECC508A 6.1 for the wakeup sequence.
    //
    // Write to address 0 to pull SDA down for the wakeup interval (60 uS).
    // Since only 8-bits get through, the I2C speed needs to be < 133 KHz for
    // this to work.
    uint8_t zero = 0;
    i2c_write(fd, 0, &zero, 1);

    // Wait for the device to wake up for real
    microsleep(ATECC508A_WAKE_DELAY_US);

    // Check that it's awake by reading its signature
    uint8_t buffer[4];
    if (i2c_read(fd, ATECC508A_ADDR, buffer, sizeof(buffer)) < 0)
        return -1;

    if (buffer[0] != 0x04 ||
            buffer[1] != 0x11 ||
            buffer[2] != 0x33 ||
            buffer[3] != 0x43)
        return -1;

    return 0;
}

int atecc508a_sleep(int fd)
{
    // See ATECC508A 6.2 for the sleep sequence.
    uint8_t sleep = 0x01;
    if (i2c_write(fd, ATECC508A_ADDR, &sleep, 1) < 0)
        return -1;

    return 0;
}

static int atecc508a_get_addr(uint8_t zone, uint16_t slot, uint8_t block, uint8_t offset, uint16_t *addr)
{
    switch (zone) {
    case ATECC508A_ZONE_CONFIG:
    case ATECC508A_ZONE_OTP:
        *addr = (uint16_t) (block << 3) + (offset & 7);
        return 0;

    case ATECC508A_ZONE_DATA:
        *addr = (uint16_t) ((block << 8) + (slot << 3) + (offset & 7));
        return 0;

    default:
        return -1;
    }
}

// See Atmel CryptoAuthentication Data Zone CRC Calculation application note
static void atecc508a_crc(uint8_t *data)
{
    const uint16_t polynom = 0x8005;
    uint16_t crc_register = 0;

    size_t length = data[0] - 2;

    for (size_t counter = 0; counter < length; counter++)
    {
        for (uint8_t shift_register = 0x01; shift_register > 0x00; shift_register <<= 1)
        {
            uint8_t data_bit = (data[counter] & shift_register) ? 1 : 0;
            uint8_t crc_bit = crc_register >> 15;
            crc_register <<= 1;
            if (data_bit != crc_bit)
                crc_register ^= polynom;
        }
    }

    uint8_t *crc_le = &data[length];
    crc_le[0] = (uint8_t)(crc_register & 0x00FF);
    crc_le[1] = (uint8_t)(crc_register >> 8);
}

/**
 * Read data out of a zone
 *
 * @param fd the fd opened by atecc508a_open
 * @param zone ATECC508A_ZONE_CONFIG, ATECC508A_ZONE_OTP, or ATECC508A_ZONE_DATA
 * @param slot the slot if this is a data zone
 * @param block which block
 * @param offset the offset in the block
 * @param data where to store the data
 * @param len how much to read (4 or 32 bytes)
 * @return 0 on success
 */
static int atecc508a_read_zone_nowake(int fd, uint8_t zone, uint16_t slot, uint8_t block, uint8_t offset, uint8_t *data, uint8_t len)
{
    uint16_t addr;

    if (atecc508a_get_addr(zone, slot, block, offset, &addr) < 0)
        return -1;

    uint8_t msg[8];

    msg[0] = 3;    // "word address"
    msg[1] = 7;    // 7 byte message
    msg[2] = 0x02; // Read opcode
    msg[3] = (len == 32 ? (zone | 0x80) : zone);
    msg[4] = addr & 0xff;
    msg[5] = addr >> 8;

    atecc508a_crc(&msg[1]);

    if (i2c_write(fd, ATECC508A_ADDR, msg, sizeof(msg)) < 0)
        return -1;

    uint8_t response[32 + 3];
    if (i2c_poll_read(fd, ATECC508A_ADDR, response, len + 3, ATECC508A_READ_TYPICAL_US, ATECC508A_READ_MAX_US) < 0)
        return -1;

    // Check length
    if (response[0] != len + 3)
        return -1;

    // Check the CRC
    uint8_t got_crc[2];
    got_crc[0] = response[len + 1];
    got_crc[1] = response[len + 2];
    atecc508a_crc(response);
    if (got_crc[0] != response[len + 1] || got_crc[1] != response[len + 2])
        return -1;

    // Copy the data (bytes after the count field)
    memcpy(data, &response[1], len);

    return 0;
}

/**
 * Read the ATECC508A's serial number
 *
 * @param fd the fd opened by atecc508a_open
 * @param serial_number a 9-byte buffer for the serial number
 * @return 0 on success
 */
int atecc508a_read_serial(int fd, uint8_t *serial_number)
{
    if (atecc508a_wakeup(fd) < 0)
        return -1;

    // Read the config -> try 2 times just in case there's a hiccup on the I2C bus
    uint8_t buffer[32];
    if (atecc508a_read_zone_nowake(fd, ATECC508A_ZONE_CONFIG, 0, 0, 0, buffer, 32) < 0 &&
            atecc508a_read_zone_nowake(fd, ATECC508A_ZONE_CONFIG, 0, 0, 0, buffer, 32) < 0)
        return -1;

    // Copy out the serial number (see datasheet for offsets)
    memcpy(&serial_number[0], &buffer[0], 4);
    memcpy(&serial_number[4], &buffer[8], 5);

    atecc508a_sleep(fd);
    return 0;
}

/**
 * Derive a public key from the private key that's stored in the specified slot.
 *
 * @param fd the fd opened by atecc508a_open
 * @param slot which slot
 * @param key a 64-byte buffer for the key
 * @return 0 on success
 */
int atecc508a_derive_public_key(int fd, uint8_t slot, uint8_t *key)
{
    // Send a GenKey command to derive the public key from a previously stored private key
    uint8_t msg[11];

    if (atecc508a_wakeup(fd) < 0)
        return -1;

    msg[0] = 3;    // "word address"
    msg[1] = 10;   // 10 byte message
    msg[2] = 0x40; // GenKey opcode
    msg[3] = 0;    // Mode
    msg[4] = slot;
    msg[5] = 0;
    msg[6] = 0;
    msg[7] = 0;
    msg[8] = 0;

    atecc508a_crc(&msg[1]);

    if (i2c_write(fd, ATECC508A_ADDR, msg, sizeof(msg)) < 0) {
        INFO("Error from i2c_write for GenKey %d", slot);
        return -1;
    }

    uint8_t response[64 + 3];
    if (i2c_poll_read(fd, ATECC508A_ADDR, response, sizeof(response), ATECC508A_GENKEY_TYPICAL_US, ATECC508A_GENKEY_MAX_US) < 0) {
        INFO("Read failed for genkey response");
        return -1;
    }

    // Check length
    if (response[0] != 64 + 3) {
        INFO("Unexpected response length 0x%02x", response[0]);
        return -1;
    }

    // Check the CRC
    uint8_t got_crc[2];
    got_crc[0] = response[64 + 1];
    got_crc[1] = response[64 + 2];
    atecc508a_crc(response);
    if (got_crc[0] != response[64 + 1] || got_crc[1] != response[64 + 2]) {
        INFO("CRC error on genkey");
        return -1;
    }

    // Copy the data (bytes after the count field)
    memcpy(key, &response[1], 64);

    atecc508a_sleep(fd);

    return 0;
}

/**
 * Derive a public key from the private key that's stored in the specified slot.
 *
 * @param fd the fd openned by atecc508a_open
 * @param slot which slot
 * @param data a 32-byte input buffer to sign
 * @param signature a 64-byte buffer for the signature
 * @return 0 on success
 */
int atecc508a_sign(int fd, uint8_t slot, const uint8_t *data, uint8_t *signature)
{
    // Send a Nonce command to load the data into TempKey
    uint8_t msg[40];

    if (atecc508a_wakeup(fd) < 0)
        return -1;

    msg[0] = 3;    // "word address"
    msg[1] = 39;   // Length
    msg[2] = 0x16; // Nonce opcode
    msg[3] = 0x3;  // Mode - Write NumIn to TempKey
    msg[4] = 0;    // Zero LSB
    msg[5] = 0;    // Zero MSB
    memcpy(&msg[6], data, 32); // NumIn

    atecc508a_crc(&msg[1]);

    if (i2c_write(fd, ATECC508A_ADDR, msg, msg[1] + 1) < 0)
        return -1;

    uint8_t response[64 + 3];
    if (i2c_poll_read(fd, ATECC508A_ADDR, response, 4, ATECC508A_NONCE_TYPICAL_US, ATECC508A_NONCE_MAX_US) < 0) {
        INFO("Didn't receive response to Nonce cmd");
        return -1;
    }

    if (response[0] != 4 || response[1] != 0) {
        INFO("Unexpected Nonce response %02x %02x %02x %02x", response[0], response[1], response[2], response[3]);
        return -1;
    }

    // Sign the value in TempKey
    msg[0] = 3;     // "word address"
    msg[1] = 7;     // Length
    msg[2] = 0x41;  // Sign opcode
    msg[3] = 0x80;  // Mode - the data to be signed is in TempKey
    msg[4] = slot;  // KeyID LSB
    msg[5] = 0;     // KeyID MSB
    atecc508a_crc(&msg[1]);

    if (i2c_write(fd, ATECC508A_ADDR, msg, msg[1] + 1) < 0) {
        INFO("Error signing TempKey");
        return -1;
    }

    if (i2c_poll_read(fd, ATECC508A_ADDR, response, 64 + 3, ATECC508A_SIGN_TYPICAL_US, ATECC508A_SIGN_MAX_US) < 0) {
        INFO("Didn't receive response from sign cmd");
        return -1;
    }

    // Check length
    if (response[0] != 64 + 3) {
        INFO("Sign response not expected: %02x %02x %02x %02x", response[0], response[1], response[2], response[3]);
        return -1;
    }

    // Check the CRC
    uint8_t got_crc[2];
    got_crc[0] = response[64 + 1];
    got_crc[1] = response[64 + 2];
    atecc508a_crc(response);
    if (got_crc[0] != response[64 + 1] || got_crc[1] != response[64 + 2]) {
        INFO("Bad CRC on sign response");
        return -1;
    }

    // Copy the data (bytes after the count field)
    memcpy(signature, &response[1], 64);

    atecc508a_sleep(fd);

    return 0;
}
