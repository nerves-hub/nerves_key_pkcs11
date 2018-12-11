/*
   Copyright 2018 Frank Hunleth

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

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

static int microsleep(int microseconds)
{
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = microseconds * 1000;
    return nanosleep(&ts, NULL);
}

static int i2c_transfer(int fd,
                        uint8_t addr,
                        const uint8_t *to_write, size_t to_write_len,
                        uint8_t *to_read, size_t to_read_len)
{
    struct i2c_rdwr_ioctl_data data;
    struct i2c_msg msgs[2];

    msgs[0].addr = addr;
    msgs[0].flags = 0;
    msgs[0].len = (uint16_t) to_write_len;
    msgs[0].buf = (uint8_t *) to_write;

    msgs[1].addr = addr;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = (uint16_t) to_read_len;
    msgs[1].buf = to_read;

    if (to_write_len != 0)
        data.msgs = &msgs[0];
    else
        data.msgs = &msgs[1];

    data.nmsgs = (to_write_len != 0 && to_read_len != 0) ? 2 : 1;

    return ioctl(fd, I2C_RDWR, &data);
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
    i2c_transfer(fd, 0, &zero, 1, NULL, 0);

    // Wait for the device to wake up for real
    microsleep(ATECC508A_WAKE_DELAY_US);

    // Check that it's awake by reading its signature
    uint8_t buffer[4];
    if (i2c_transfer(fd, ATECC508A_ADDR, NULL, 0, buffer, sizeof(buffer)) < 0)
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
    if (i2c_transfer(fd, ATECC508A_ADDR, &sleep, 1, NULL, 0) < 0)
        return -1;

    return 0;
}

static int atecc508a_get_addr(uint8_t zone, uint16_t slot, uint8_t block, uint8_t offset, uint16_t *addr)
{
    switch (zone) {
    case ATECC508A_ZONE_CONFIG:
    case ATECC508A_ZONE_OTP:
        *addr = (block << 3) + (offset & 7);
        return 0;

    case ATECC508A_ZONE_DATA:
        *addr = (block << 8) + (slot << 3) + (offset & 7);
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
int atecc508a_read_zone(int fd, uint8_t zone, uint16_t slot, uint8_t block, uint8_t offset, uint8_t *data, uint8_t len)
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

    if (i2c_transfer(fd, ATECC508A_ADDR, msg, sizeof(msg), NULL, 0) < 0)
        return -1;

    // Wait for read to be available
    microsleep(5000);

    uint8_t response[32 + 3];
    if (i2c_transfer(fd, ATECC508A_ADDR, NULL, 0, response, len + 3) < 0)
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
    // Read the config -> try 2 times just in case there's a hiccup on the I2C bus
    uint8_t buffer[32];
    if (atecc508a_read_zone(fd, ATECC508A_ZONE_CONFIG, 0, 0, 0, buffer, 32) < 0 &&
            atecc508a_read_zone(fd, ATECC508A_ZONE_CONFIG, 0, 0, 0, buffer, 32) < 0)
        return -1;

    // Copy out the serial number (see datasheet for offsets)
    memcpy(&serial_number[0], &buffer[0], 4);
    memcpy(&serial_number[4], &buffer[8], 5);
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

    if (i2c_transfer(fd, ATECC508A_ADDR, msg, sizeof(msg), NULL, 0) < 0)
        return -1;

    // Wait for the public key to be available
    microsleep(115000);

    uint8_t response[64 + 3];
    if (i2c_transfer(fd, ATECC508A_ADDR, NULL, 0, response, sizeof(response)) < 0)
        return -1;

    // Check length
    if (response[0] != 64 + 3)
        return -1;

    // Check the CRC
    uint8_t got_crc[2];
    got_crc[0] = response[64 + 1];
    got_crc[1] = response[64 + 2];
    atecc508a_crc(response);
    if (got_crc[0] != response[64 + 1] || got_crc[1] != response[64 + 2])
        return -1;

    // Copy the data (bytes after the count field)
    memcpy(key, &response[1], 64);

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

    msg[0] = 3;    // "word address"
    msg[1] = 39;   // Length
    msg[2] = 0x16; // Nonce opcode
    msg[3] = 0x3;  // Mode - Write NumIn to TempKey
    msg[4] = 0;    // Zero LSB
    msg[5] = 0;    // Zero MSB
    memcpy(&msg[6], data, 32); // NumIn

    atecc508a_crc(&msg[1]);

    if (i2c_transfer(fd, ATECC508A_ADDR, msg, msg[1] + 1, NULL, 0) < 0)
        return -1;

    // Wait for the data to be stored
    microsleep(7000);

    uint8_t response[64 + 3];
    if (i2c_transfer(fd, ATECC508A_ADDR, NULL, 0, response, 4) < 0) {
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

    if (i2c_transfer(fd, ATECC508A_ADDR, msg, msg[1] + 1, NULL, 0) < 0) {
        INFO("Error signing TempKey");
        return -1;
    }

    // Wait for the signature
    microsleep(100000);

    if (i2c_transfer(fd, ATECC508A_ADDR, NULL, 0, response, 64 + 3) < 0) {
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

    return 0;
}
