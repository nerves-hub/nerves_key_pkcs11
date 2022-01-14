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

#ifndef ATECC508A_H
#define ATECC508A_H

#include <stdint.h>

#define ATECC508A_ZONE_CONFIG 0
#define ATECC508A_ZONE_OTP    1
#define ATECC508A_ZONE_DATA   2

int atecc508a_open(const char *filename);
void atecc508a_close(int fd);
int atecc508a_wakeup(int fd, uint8_t addr);
int atecc508a_sleep(int fd, uint8_t addr);
int atecc508a_read_serial(int fd, uint8_t addr, uint8_t *serial_number);
int atecc508a_derive_public_key(int fd, uint8_t addr, uint8_t slot, uint8_t *key);
int atecc508a_sign(int fd, uint8_t addr, uint8_t slot, const uint8_t *data, uint8_t *signature);
int atecc508a_read_zone_nowake(int fd, uint8_t addr, uint8_t zone, uint16_t slot, uint8_t block, uint8_t offset, uint8_t *data, uint8_t len);

#endif // ATECC508A_H
