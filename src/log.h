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

#ifndef LOG_H
#define LOG_H

#include <stdio.h>

#define PROGNAME "nerves_key_pkcs11"

#define DEBUG
#ifdef DEBUG
#define ENTER() do { fprintf(stderr, "%s: Entered %s().\r\n", PROGNAME, __func__); } while (0)

#define INFO(fmt, ...) \
        do { fprintf(stderr, "%s: " fmt "\r\n", PROGNAME, ##__VA_ARGS__); } while (0)
#else
#define ENTER()
#define INFO(fmt, ...)
#endif

// Always warn on unimplemented functions. Maybe someone will help make this more complete.
#define UNIMPLEMENTED() do { \
        fprintf(stderr, "%s: %s unimplemented.\r\n", PROGNAME, __func__); \
    } while (0)
#define ERROR(fmt, ...) \
        do { fprintf(stderr, "%s: " fmt "\r\n", PROGNAME, ##__VA_ARGS__); } while (0)

#endif // LOG_H
