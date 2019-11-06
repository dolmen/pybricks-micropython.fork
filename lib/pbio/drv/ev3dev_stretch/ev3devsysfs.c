// // SPDX-License-Identifier: MIT
// // Copyright (c) 2019 Laurens Valk

#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <pbdrv/ev3sensor.h>
#include <pbio/ev3device.h>

#include <pbio/port.h>
#include <pbio/iodev.h>

#define MAX_PATH_LENGTH 60
#define MAX_READ_LENGTH "60"

// Get the ev3dev sensor number for a given port
pbio_error_t sysfs_get_number(pbio_port_t port, const char *rdir, int *sysfs_number) {
    // Open lego-sensor directory in sysfs
    DIR *d_sensor;
    struct dirent *entry;
    d_sensor = opendir(rdir);
    if (!d_sensor) {
        return PBIO_ERROR_NO_DEV;
    }
    // Find sensor number for given port
    while ((entry = readdir(d_sensor))) {
        // Ignore the . and .. folders
        if (entry->d_name[0] != '.') {
            // Open the address file to get the port number
            char p_address[MAX_PATH_LENGTH];
#pragma GCC diagnostic push
#if (__GNUC__ > 7) || (__GNUC__ == 7 && __GNUC_MINOR__ >= 1)
#pragma GCC diagnostic ignored "-Wformat-truncation"
#endif
            snprintf(p_address, MAX_PATH_LENGTH, "%s/%s/address", rdir, entry->d_name);
#pragma GCC diagnostic pop
            FILE *f_address = fopen(p_address, "r");
            if (f_address == NULL) {
                return PBIO_ERROR_IO;
            }

            // Get the port from the address file
            fseek(f_address, 12, SEEK_SET);
            pbio_port_t port_found = fgetc(f_address);
            fclose(f_address);

            // If the port matches the requested port, get where it was found.
            if (port_found == port) {
                sscanf(entry->d_name, "%*[a-z]%d",  sysfs_number);
                closedir(d_sensor);
                return PBIO_SUCCESS;
            }
        }
    }
    // No sensor was found at the requested port
    closedir(d_sensor);
    return PBIO_ERROR_NO_DEV;
}

// Open a sysfs attribute
pbio_error_t sysfs_open(FILE **file, int n, const char *attribute, const char *rw) {
    char path[MAX_PATH_LENGTH];
    snprintf(path, MAX_PATH_LENGTH, "/sys/class/lego-sensor/sensor%d/%s", n, attribute);
    *file = fopen(path, rw);
    return file == NULL? PBIO_ERROR_IO : PBIO_SUCCESS;
}

// Read a string from a previously opened sysfs attribute
pbio_error_t sysfs_read_str(FILE *file, char *dest) {
    if (fseek(file, 0, SEEK_SET) == -1) {
        return PBIO_ERROR_IO;
    }

    if (fscanf(file, "%" MAX_READ_LENGTH "s", dest) < 1) {
        return PBIO_ERROR_IO;
    }

    return PBIO_SUCCESS;
}

// Read an int from a previously opened sysfs attribute
pbio_error_t sysfs_read_int(FILE *file, int *dest) {
    if (fseek(file, 0, SEEK_SET) == -1) {
        return PBIO_ERROR_IO;
    }

    if (fscanf(file, "%d", dest) < 1) {
        return PBIO_ERROR_IO;
    }

    if (fflush(file) != 0) {
        return PBIO_ERROR_IO;
    }

    return PBIO_SUCCESS;
}
