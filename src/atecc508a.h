#ifndef ATECC508A_H
#define ATECC508A_H

#include <stdint.h>

#define ATECC508A_ZONE_CONFIG 0
#define ATECC508A_ZONE_OTP    1
#define ATECC508A_ZONE_DATA   2

int atecc508a_open(const char *filename);
void atecc508a_close(int fd);
int atecc508a_wakeup(int fd);
int atecc508a_sleep(int fd);
int atecc508a_read_serial(int fd, uint8_t *serial_number);
int atecc508a_derive_public_key(int fd, uint8_t slot, uint8_t *key);
int atecc508a_sign(int fd, uint8_t slot, const uint8_t *data, uint8_t *signature);

#endif // ATECC508A_H
