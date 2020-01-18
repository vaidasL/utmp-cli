#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "platform.h"
#include "usbtemp.h"

enum {
  HELP,
  ACQUIRE_TEMP,
  READ_ROM,
  SET
};

int main(int argc, char **argv)
{
  const char *hex_upper = "%02X", *hex_lower = "%02x";
  signed char c;
  char units = 'C';
  HANDLE fd;
  char *serial_port = NULL;
  int verbose = 1;
  int rv = 0;
  int i;
  int action = ACQUIRE_TEMP;
  int precision = 12;
  float temperature;
  unsigned char rom[DS18X20_ROM_SIZE];
  char timebuf[40];
  time_t now;
  struct tm *timeptr;
  const char *hex_format = NULL;

  while ((c = getopt(argc, argv, "fhp:qRrs:")) != -1) {
    switch (c) {
      case 'f':
        units = 'F';
        break;
      case 'h':
        action = HELP;
        break;
      case 'p':
        action = SET;
        precision = strtol(optarg, NULL, 10);
        break;
      case 'q':
        verbose = 0;
        break;
      case 'R':
        hex_format = hex_upper;
        break;
      case 'r':
        hex_format = hex_lower;
        break;
      case 's':
        serial_port = optarg;
        break;
      case '?':
        return -1;
    }
  }

  if (units == 'F') {
    action = ACQUIRE_TEMP;
  }

  if (hex_format != NULL)
  {
    action = READ_ROM;
  }

  if (action == HELP) {
    verbose = 1;
  }

  if (verbose) {
    printf("USB Thermometer CLI v1.06 Copyright 2019 usbtemp.com Licensed under MIT licence.\n");
  }

  if (action == HELP) {
    printf("\t-f\tDisplay temperature using the Fahrenheit scale\n");
    printf("\t-p\tSet probe precision {9,10,11,12}\n");
    printf("\t-q\tQuiet mode\n");
    printf("\t-r\tGet probe serial number (ROM) in hexadecimal, or -R uppercase\n");
    printf("\t-s\tSet serial port\n");
    return 0;
  }

  if (!serial_port) {
    serial_port = DEFAULT_SERIAL_PORT;
  }
  if (verbose) {
    printf("Using serial port: %s\n", serial_port);
  }

  fd = DS18B20_open(serial_port);
  if (!is_fd_valid(fd)) {
    fprintf(stderr, "%s\n", DS18B20_errmsg());
    return 1;
  }

  switch (action) {

    case ACQUIRE_TEMP:
      if (DS18B20_measure(fd) < 0) {
        fprintf(stderr, "%s\n", DS18B20_errmsg());
        rv = 1;
        break;
      }
      if (verbose) {
        printf("Waiting for response ...\n");
      }
      wait_1s();
      if (DS18B20_acquire(fd, &temperature) < 0) {
        fprintf(stderr, "%s\n", DS18B20_errmsg());
        rv = 1;
        break;
      }
      time(&now);
      timeptr = localtime(&now);
      strftime(timebuf, sizeof(timebuf), "%b %d %H:%M:%S", timeptr);

      if (units == 'F') {
        temperature = (9 * temperature) / 5 + 32;
      }

      if (verbose) {
        printf("%s Sensor %c: %.2f\n", timebuf, units, temperature);
      } else {
        printf("%.2f", temperature);
      }
      break;

    case READ_ROM:
      if (DS18B20_rom(fd, rom) < 0) {
        fprintf(stderr, "%s\n", DS18B20_errmsg());
        rv = 1;
        break;
      }
      printf("ROM: ");
      for (i = 0; i < DS18X20_ROM_SIZE; i++) {
        printf(hex_format, rom[i]);
      }
      printf("\n");
      break;

    case SET:
      if (precision < 9 || 12 < precision) {
        if (verbose == 1) {
          fprintf(stderr, "Probe precision out of range!\n");
        }
        rv = 1;
        break;
      }
      rv = DS18B20_setprecision(fd, precision);
      break;

  }

  DS18B20_close(fd);
  return rv;
}
