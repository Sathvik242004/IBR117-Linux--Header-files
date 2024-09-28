#ifndef I2C_H
#define I2C_H

#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

// I2C device file open function
static inline int i2c_open(const char *device) {
    int file = open("/dev/i2c-2", O_RDWR);
    if (file < 0) {
        perror("Failed to open I2C device");
        return -1;
    }
    return file;
}

// I2C set slave address function
static inline int i2c_set_slave(int i2c_file, uint8_t slave_addr) {
    if (ioctl(i2c_file, I2C_SLAVE, slave_addr) < 0) {
        perror("Failed to set I2C slave address");
        return -1;
    }
    return 0;
}

// I2C write function
static inline int i2c_write(int i2c_file, uint8_t reg, uint8_t *data, size_t length) {
    uint8_t *buffer = (uint8_t *)malloc(length + 1);
    if (buffer == NULL) {
        perror("Failed to allocate memory");
        return -1;
    }
    buffer[0] = reg;
    for (size_t i = 0; i < length; i++) {
        buffer[i + 1] = data[i];
    }

    ssize_t result = write(i2c_file, buffer, length + 1);
    free(buffer);
    if (result != static_cast<ssize_t>(length + 1)) {
        return -1;
    }

    return 0;
}

// I2C read function
static inline int i2c_read(int i2c_file, uint8_t reg, uint8_t *buffer, size_t length) {
    if (write(i2c_file, &reg, 1) != 1) {
        perror("Failed to write register address to I2C");
        return -1;
    }

    if (read(i2c_file, buffer, length) != static_cast<ssize_t>(length)) {
        return -1;
    }

    return 0;
}

// I2C write a single byte
static inline int i2c_write_byte(int i2c_file, uint8_t reg, uint8_t value) {
    uint8_t data[1] = {value};
    return i2c_write(i2c_file, reg, data, 1);
}

// I2C read a single byte
static inline int i2c_read_byte(int i2c_file, uint8_t reg, uint8_t *value) {
    return i2c_read(i2c_file, reg, value, 1);
}

// I2C close function
static inline void i2c_close(int i2c_file) {
    close(i2c_file);
}

// I2C device initialization helper
static inline int i2c_init(const char *device, uint8_t slave_addr) {
    int i2c_file = i2c_open(device);
    if (i2c_file < 0) {
        return -1;
    }

    if (i2c_set_slave(i2c_file, slave_addr) < 0) {
        i2c_close(i2c_file);
        return -1;
    }
    return i2c_file;
}

// I2C delay function (optional)
static inline void i2c_delay_ms(unsigned int milliseconds) {
    usleep(milliseconds * 1000);  // Delay in milliseconds
}

#endif // I2C_H

