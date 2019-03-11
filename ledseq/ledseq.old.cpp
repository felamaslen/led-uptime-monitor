#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <climits>

#define NUM_LEDS 30

// all times are in micro seconds
#define SCROLL_INTERVAL 50000
#define PONG_INTERVAL 20000
#define MEM_INTERVAL 1000000

// unix time stamp (2016-01-23 11:36:52 GMT)
#define INSTALLATION_TIME 1453549012

// sample /proc/stat every 200ms
#define CPU_USAGE_SAMPLE_TIME 200000

/** Sensors stuff */
#define SENSORS_ERR_WILDCARDS	1 /* Wildcard found in chip name */
#define SENSORS_ERR_NO_ENTRY	2 /* No such subfeature known */
#define SENSORS_ERR_ACCESS_R	3 /* Can't read */
#define SENSORS_ERR_KERNEL	4 /* Kernel interface error */
#define SENSORS_ERR_DIV_ZERO	5 /* Divide by zero */
#define SENSORS_ERR_CHIP_NAME	6 /* Can't parse chip name */
#define SENSORS_ERR_BUS_NAME	7 /* Can't parse bus name */
#define SENSORS_ERR_PARSE	8 /* General parse error */
#define SENSORS_ERR_ACCESS_W	9 /* Can't write */
#define SENSORS_ERR_IO		10 /* I/O error */
#define SENSORS_ERR_RECURSION	11 /* Evaluation recurses too deep */

typedef enum sensors_feature_type {
	SENSORS_FEATURE_IN		= 0x00,
	SENSORS_FEATURE_FAN		= 0x01,
	SENSORS_FEATURE_TEMP		= 0x02,
	SENSORS_FEATURE_POWER		= 0x03,
	SENSORS_FEATURE_ENERGY		= 0x04,
	SENSORS_FEATURE_CURR		= 0x05,
	SENSORS_FEATURE_HUMIDITY	= 0x06,
	SENSORS_FEATURE_MAX_MAIN,
	SENSORS_FEATURE_VID		= 0x10,
	SENSORS_FEATURE_INTRUSION	= 0x11,
	SENSORS_FEATURE_MAX_OTHER,
	SENSORS_FEATURE_BEEP_ENABLE	= 0x18,
	SENSORS_FEATURE_MAX,
	SENSORS_FEATURE_UNKNOWN		= INT_MAX,
} sensors_feature_type;

/* All the sensor types (in, fan, temp, vid) are a multiple of 0x100 apart,
   and sensor subfeatures which have no compute mapping have bit 7 set. */
typedef enum sensors_subfeature_type {
	SENSORS_SUBFEATURE_IN_INPUT = SENSORS_FEATURE_IN << 8,
	SENSORS_SUBFEATURE_IN_MIN,
	SENSORS_SUBFEATURE_IN_MAX,
	SENSORS_SUBFEATURE_IN_LCRIT,
	SENSORS_SUBFEATURE_IN_CRIT,
	SENSORS_SUBFEATURE_IN_AVERAGE,
	SENSORS_SUBFEATURE_IN_LOWEST,
	SENSORS_SUBFEATURE_IN_HIGHEST,
	SENSORS_SUBFEATURE_IN_ALARM = (SENSORS_FEATURE_IN << 8) | 0x80,
	SENSORS_SUBFEATURE_IN_MIN_ALARM,
	SENSORS_SUBFEATURE_IN_MAX_ALARM,
	SENSORS_SUBFEATURE_IN_BEEP,
	SENSORS_SUBFEATURE_IN_LCRIT_ALARM,
	SENSORS_SUBFEATURE_IN_CRIT_ALARM,

	SENSORS_SUBFEATURE_FAN_INPUT = SENSORS_FEATURE_FAN << 8,
	SENSORS_SUBFEATURE_FAN_MIN,
	SENSORS_SUBFEATURE_FAN_MAX,
	SENSORS_SUBFEATURE_FAN_ALARM = (SENSORS_FEATURE_FAN << 8) | 0x80,
	SENSORS_SUBFEATURE_FAN_FAULT,
	SENSORS_SUBFEATURE_FAN_DIV,
	SENSORS_SUBFEATURE_FAN_BEEP,
	SENSORS_SUBFEATURE_FAN_PULSES,
	SENSORS_SUBFEATURE_FAN_MIN_ALARM,
	SENSORS_SUBFEATURE_FAN_MAX_ALARM,

	SENSORS_SUBFEATURE_TEMP_INPUT = SENSORS_FEATURE_TEMP << 8,
	SENSORS_SUBFEATURE_TEMP_MAX,
	SENSORS_SUBFEATURE_TEMP_MAX_HYST,
	SENSORS_SUBFEATURE_TEMP_MIN,
	SENSORS_SUBFEATURE_TEMP_CRIT,
	SENSORS_SUBFEATURE_TEMP_CRIT_HYST,
	SENSORS_SUBFEATURE_TEMP_LCRIT,
	SENSORS_SUBFEATURE_TEMP_EMERGENCY,
	SENSORS_SUBFEATURE_TEMP_EMERGENCY_HYST,
	SENSORS_SUBFEATURE_TEMP_LOWEST,
	SENSORS_SUBFEATURE_TEMP_HIGHEST,
	SENSORS_SUBFEATURE_TEMP_MIN_HYST,
	SENSORS_SUBFEATURE_TEMP_LCRIT_HYST,
	SENSORS_SUBFEATURE_TEMP_ALARM = (SENSORS_FEATURE_TEMP << 8) | 0x80,
	SENSORS_SUBFEATURE_TEMP_MAX_ALARM,
	SENSORS_SUBFEATURE_TEMP_MIN_ALARM,
	SENSORS_SUBFEATURE_TEMP_CRIT_ALARM,
	SENSORS_SUBFEATURE_TEMP_FAULT,
	SENSORS_SUBFEATURE_TEMP_TYPE,
	SENSORS_SUBFEATURE_TEMP_OFFSET,
	SENSORS_SUBFEATURE_TEMP_BEEP,
	SENSORS_SUBFEATURE_TEMP_EMERGENCY_ALARM,
	SENSORS_SUBFEATURE_TEMP_LCRIT_ALARM,

	SENSORS_SUBFEATURE_POWER_AVERAGE = SENSORS_FEATURE_POWER << 8,
	SENSORS_SUBFEATURE_POWER_AVERAGE_HIGHEST,
	SENSORS_SUBFEATURE_POWER_AVERAGE_LOWEST,
	SENSORS_SUBFEATURE_POWER_INPUT,
	SENSORS_SUBFEATURE_POWER_INPUT_HIGHEST,
	SENSORS_SUBFEATURE_POWER_INPUT_LOWEST,
	SENSORS_SUBFEATURE_POWER_CAP,
	SENSORS_SUBFEATURE_POWER_CAP_HYST,
	SENSORS_SUBFEATURE_POWER_MAX,
	SENSORS_SUBFEATURE_POWER_CRIT,
	SENSORS_SUBFEATURE_POWER_AVERAGE_INTERVAL = (SENSORS_FEATURE_POWER << 8) | 0x80,
	SENSORS_SUBFEATURE_POWER_ALARM,
	SENSORS_SUBFEATURE_POWER_CAP_ALARM,
	SENSORS_SUBFEATURE_POWER_MAX_ALARM,
	SENSORS_SUBFEATURE_POWER_CRIT_ALARM,

	SENSORS_SUBFEATURE_ENERGY_INPUT = SENSORS_FEATURE_ENERGY << 8,

	SENSORS_SUBFEATURE_CURR_INPUT = SENSORS_FEATURE_CURR << 8,
	SENSORS_SUBFEATURE_CURR_MIN,
	SENSORS_SUBFEATURE_CURR_MAX,
	SENSORS_SUBFEATURE_CURR_LCRIT,
	SENSORS_SUBFEATURE_CURR_CRIT,
	SENSORS_SUBFEATURE_CURR_AVERAGE,
	SENSORS_SUBFEATURE_CURR_LOWEST,
	SENSORS_SUBFEATURE_CURR_HIGHEST,
	SENSORS_SUBFEATURE_CURR_ALARM = (SENSORS_FEATURE_CURR << 8) | 0x80,
	SENSORS_SUBFEATURE_CURR_MIN_ALARM,
	SENSORS_SUBFEATURE_CURR_MAX_ALARM,
	SENSORS_SUBFEATURE_CURR_BEEP,
	SENSORS_SUBFEATURE_CURR_LCRIT_ALARM,
	SENSORS_SUBFEATURE_CURR_CRIT_ALARM,

	SENSORS_SUBFEATURE_HUMIDITY_INPUT = SENSORS_FEATURE_HUMIDITY << 8,

	SENSORS_SUBFEATURE_VID = SENSORS_FEATURE_VID << 8,

	SENSORS_SUBFEATURE_INTRUSION_ALARM = SENSORS_FEATURE_INTRUSION << 8,
	SENSORS_SUBFEATURE_INTRUSION_BEEP,

	SENSORS_SUBFEATURE_BEEP_ENABLE = SENSORS_FEATURE_BEEP_ENABLE << 8,

	SENSORS_SUBFEATURE_UNKNOWN = INT_MAX,
} sensors_subfeature_type;

static int get_type_scaling(sensors_subfeature_type type) {
	/* Multipliers for subfeatures */
	switch (type & 0xFF80) {
	case SENSORS_SUBFEATURE_IN_INPUT:
	case SENSORS_SUBFEATURE_TEMP_INPUT:
	case SENSORS_SUBFEATURE_CURR_INPUT:
	case SENSORS_SUBFEATURE_HUMIDITY_INPUT:
		return 1000;
	case SENSORS_SUBFEATURE_FAN_INPUT:
		return 1;
	case SENSORS_SUBFEATURE_POWER_AVERAGE:
	case SENSORS_SUBFEATURE_ENERGY_INPUT:
		return 1000000;
	}

	/* Multipliers for second class subfeatures
	   that need their own multiplier */
	switch (type) {
	case SENSORS_SUBFEATURE_POWER_AVERAGE_INTERVAL:
	case SENSORS_SUBFEATURE_VID:
	case SENSORS_SUBFEATURE_TEMP_OFFSET:
		return 1000;
	default:
		return 1;
	}
}

/** Serial interface functions */
int set_interface_attribs(int fd, int speed, int parity) {
  struct termios tty;
  memset(&tty, 0, sizeof tty);
  int tcget_error = tcgetattr(fd, &tty);
  if (tcget_error != 0) {
    printf("error %d from tcgetattr", tcget_error);
    return -1;
  }

  cfsetospeed(&tty, speed);
  cfsetispeed(&tty, speed);

  tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;	// 8-bit chars
  // disable IGNBRK for mismatched speed tests; otherwise receive break
  // as \000 chars
  tty.c_iflag &= ~IGNBRK;	// disable break processing
  tty.c_lflag = 0;		// no signaling chars, no echo,
  // no canonical processing
  tty.c_oflag = 0;		// no remapping, no delays
  tty.c_cc[VMIN] = 0;		// read doesn't block
  tty.c_cc[VTIME] = 5;		// 0.5 seconds read timeout

  tty.c_iflag &= ~(IXON | IXOFF | IXANY);	// shut off xon/xoff ctrl

  tty.c_cflag |= (CLOCAL | CREAD);	// ignore modem controls,
  // enable reading
  tty.c_cflag &= ~(PARENB | PARODD);	// shut off parity
  tty.c_cflag |= parity;
  tty.c_cflag &= ~CSTOPB;
  tty.c_cflag &= ~CRTSCTS;

  int tcset_error = tcsetattr(fd, TCSANOW, &tty);
  if (tcset_error != 0) {
    printf("error %d from tcsetattr", tcset_error);
    return -1;
  }
  return 0;
}

void set_blocking(int fd, int should_block) {
  struct termios tty;
  memset(&tty, 0, sizeof tty);
  int tcget_error = tcgetattr(fd, &tty);
  if (tcget_error != 0) {
    printf("error %d from tggetattr", tcget_error);
    return;
  }

  tty.c_cc[VMIN] = should_block ? 1 : 0;
  tty.c_cc[VTIME] = 5;		// 0.5 seconds read timeout

  int tcset_error = tcsetattr(fd, TCSANOW, &tty);
  if (tcset_error != 0) {
    printf("error %d setting term attributes", tcset_error);
  }
}

/*
 * Simple method to add one string to (part of) another
 */
void stradd(char *dst, char *src, int offset, int length) {
  for (int i = offset, j = 0; j < length; i++, j++) {
    dst[i] = src[j];
  }
}

void set_display(int fd, char *pattern) {
  char data[11];

  data[0] = 'c'; // send begin command to arduino

  stradd(data, pattern, 1, 10);

  write(fd, data, 11);

  usleep((5 + 25) * 100);

  return;
}

/*
 * This is a method used by other functions to tell the arduino
 * which LEDs to light up
 */
void set_pattern(int fd, char *pattern) {
  char data[NUM_LEDS + 2];

  data[0] = 'b';              // send begin command to arduino
  data[NUM_LEDS + 1] = 'e';   // send end command to arduino

  // as far as the arduino is concerned, each character comes on its own.
  // There is no need for the string to be null-terminated like a string in C
  // data[NUM_LEDS + 2] = '\0';  // terminate with null character

  stradd(data, pattern, 1, NUM_LEDS);
  
  write(fd, data, NUM_LEDS + 2);

  usleep((NUM_LEDS + 2 + 25) * 100);

  return;
}

/* Backend functions */

/**
 * Get each CPU's temperature, and return it as one int
 * e.g. if CPU0 is 35C, CPU1 30C, then this function would return 3035
 */
int get_temps(char *temps) {
  int value;

  int TEMP_IDX_MAX = 2;
  FILE *fp;
  const char* n[] = {
    "/sys/class/hwmon/hwmon1/temp2_input",
    "/sys/class/hwmon/hwmon1/temp3_input"
  };

  for (int i = 0, o = 0; i < TEMP_IDX_MAX; i++) {
    if (fp = fopen(n[i], "r")) {
      char buf[256];
      size_t bytes_read;

      bytes_read = fread(buf, 1, sizeof(buf), fp);

      fclose(fp);

      if (bytes_read == 0 || bytes_read == sizeof(buf)) {
        printf("Error reading sensor values!\n");
        return -1;
      }

      buf[bytes_read] = '\0';

      value = (int)strtol(buf, NULL, 10);

      value /= get_type_scaling(SENSORS_SUBFEATURE_TEMP_INPUT);

      // this shouldn't be necessary, unless
      // (a) someone put liquid nitrogen on the motherboard, or
      // (b) the sensor is calibrated incorrectly
      value = abs(value);

      if (value > 99) {
        // hopefully this would never happen!!!
        value = 99;
      }

      int unit  = value % 10;
      int ten   = floor(value / 10);

      char tmp_string[4];
      sprintf(tmp_string, "0%d0%d", ten, unit);

      stradd(temps, tmp_string, o, 4);

      o += 4;

      if (i == 0) {
        // add parameter to print colon
        char colon[3] = "10";

        stradd(temps, colon, o, 2);
        o += 2;
      }
    }
    else {
      return -SENSORS_ERR_KERNEL;
    }
  }

  return 0;
}

/*
 * Get current memory usage values from /proc/meminfo
 */
struct meminfo {
  long unsigned int mem_total;
  long unsigned int mem_free;
  long unsigned int mem_available;
  long unsigned int mem_buffers;
  long unsigned int mem_cached;
};

/*
 * Get the current memory usage of the machine
 */
int get_mem_usage(double *fraction) {
  meminfo mem_usage;

  FILE *fstat = fopen("/proc/meminfo", "r");
  
  if (!fstat) {
    printf("Error reading from /proc/meminfo!\n");
    return -1;
  }

  long unsigned int usage[5];
  
  // bzero(times, sizeof(times));
  if (fscanf(fstat, "%*s %lu %*s %*s %lu %*s %*s %lu %*s %*s %lu %*s %*s %lu %*s",
        &usage[0], &usage[1], &usage[2], &usage[3], &usage[4]
      ) == EOF) {

    fclose(fstat);
    return -1;
  }

  fclose(fstat);

  mem_usage.mem_total     = usage[0];
  mem_usage.mem_free      = usage[1];
  mem_usage.mem_available = usage[2];
  mem_usage.mem_buffers   = usage[3];
  mem_usage.mem_cached    = usage[4];

  *fraction = ((double)(mem_usage.mem_total - (
    mem_usage.mem_free + mem_usage.mem_buffers + mem_usage.mem_cached
  )) / (double)(mem_usage.mem_total));

  return 0;
}

/*
 * Get current CPU time values from /proc/stat
 */
struct cpu_time {
  long unsigned int time_total;
  long unsigned int time_user;
  long unsigned int time_nice;
  long unsigned int time_system;
  long unsigned int time_idle;
  long unsigned int time_iowait;
  long unsigned int time_irq;
  long unsigned int time_softirq;
};

int get_cpu_time(cpu_time* result) {
  FILE *fstat = fopen("/proc/stat", "r");
  
  if (!fstat) {
    printf("Error reading from /proc/stat!\n");
    return -1;
  }

  long unsigned int times[10];
  
  // bzero(times, sizeof(times));
  if (fscanf(fstat, "%*s %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
        &times[0], &times[1], &times[2], &times[3],
        &times[4], &times[5], &times[6], &times[7],
        &times[8], &times[9]
      ) == EOF) {

    fclose(fstat);
    return -1;
  }

  fclose(fstat);

  result->time_user = times[0];
  result->time_nice = times[1];
  result->time_system = times[2];
  result->time_idle = times[3];
  result->time_iowait = times[4];
  result->time_irq = times[5];
  result->time_softirq = times[6];
  
  result->time_total =
    result->time_user +
    result->time_nice +
    result->time_system +
    result->time_iowait +
    result->time_idle
  ;

  return 0;
}

/*
 * Get the current CPU usage of the machine
 */
int get_cpu_usage(double *fraction) {
  cpu_time last, current;

  if (get_cpu_time(&last) == -1) {
    printf("error\n");
  }

  usleep(CPU_USAGE_SAMPLE_TIME);

  if (get_cpu_time(&current) == -1) {
    printf("error\n");
  }

  const long unsigned int diff_idle   = current.time_idle  - last.time_idle;
  const long unsigned int diff_total  = current.time_total - last.time_total;

  *fraction = ((double)(diff_total - diff_idle) / (double)diff_total);
}

/*
 * Get the number of seconds since INSTALLATION_TIME
 */
void get_time_seconds(double *seconds) {
  double current_time = (double)time(NULL);

  *seconds = current_time - INSTALLATION_TIME;
}

/*
 * Get the number of seconds the machine has been on without
 * a reboot
 */
int get_uptime_seconds(double *seconds) {
  const char *error_text = "** Error reading from uptime file!";

  FILE *fp = fopen("/proc/uptime", "r");
  
  if (!fp) {
    printf("%s\n", error_text);
    return -1;
  }

  char buf[256];
  size_t bytes_read;

  bytes_read = fread(buf, 1, sizeof(buf), fp);

  fclose(fp);

  if (bytes_read == 0 || bytes_read == sizeof(buf)) {
    printf("%s\n", error_text);
    return -1;
  }

  buf[bytes_read] = '\0';

  char *uptime = strtok(buf, " ");

  *seconds = atof(uptime);

  return 0;
}

/** Pattern generating functions */

/**
 * output the CPU temperature to the LED display
 */
void do_temps(int fd) {
  char temps[11] = "0000000000";
  get_temps(temps);

  set_display(fd, temps);
}

/**
 * output a word to the LED display
 */
int do_word(int fd, char *word) {
  char display_string[11];

  if (!strcmp(word, "beef")) {
    sprintf(display_string, "B000000000");
  }
  else if (!strcmp(word, "babe")) {
    sprintf(display_string, "C000000000");
  }
  else {
    printf("%s is not a recognised word. Sorry!\n", word);

    return 1;
  }

  set_display(fd, display_string);

  return 0;
}

/*
 * Calculate which LEDs should be on or off, so as to
 * represent the uptime in seconds, as a binary number,
 * where an LED being on represents "1", and off "0".
 *
 * E.g. an uptime of 186 seconds would be represented by the binary
 * number 0b10111010.
 */
void set_seconds_pattern(int fd, long *seconds) {
  char digits[NUM_LEDS + 1];

  long num = *seconds;

  int digit = 0;
  while (digit < NUM_LEDS) {
    if (num > 0) {
      digits[NUM_LEDS - 1 - digit] = (num % 2) + '0';
      
      num = floor(num / 2);
    }
    else {
      digits[NUM_LEDS - 1 - digit] = '0';
    }

    digit++;
  }

  digits[NUM_LEDS] = '\0';

  set_pattern(fd, digits);
}

/*
 * A string, e.g. 1110101011, is given as a parameter to the
 * program, and this function makes the LEDs "scroll" that pattern
 * as if it was text on a notice board.
 */
void do_scrolltext(int fd, char *seq) {
  int size = strlen(seq);
  
  int offset = 0;

  while (1) {
    char str[NUM_LEDS + 1];    

    if (size <= NUM_LEDS) {
      // we can fit the entire string on the board
      
      // check if we have to split seq into two parts
      if (offset + size <= NUM_LEDS) {
        // the string + offset still fits on the board
        char str1[offset];
        memset(str1, '0', offset);

        stradd(str, str1, 0, offset);

        stradd(str, seq, offset, size);
        
        char str3[NUM_LEDS - size - offset];
        memset(str3, '0', NUM_LEDS - size - offset);

        stradd(str, str3, offset + size, NUM_LEDS - size - offset);
      }
      else {
        // have to split the string into two
        int num_chars_cut = offset + size - NUM_LEDS;

        char str1[num_chars_cut];
        memcpy(str1, &seq[size - num_chars_cut], num_chars_cut);
     
        stradd(str, str1, 0, num_chars_cut);

        char str2[NUM_LEDS - size];
        memset(str2, '0', NUM_LEDS - size);

        stradd(str, str2, num_chars_cut, NUM_LEDS - size);

        char str3[size - num_chars_cut];
        memcpy(str3, &seq[0], size - num_chars_cut);
     
        stradd(str, str3, num_chars_cut + NUM_LEDS - size, size - num_chars_cut);
      }

      offset = (offset + 1) % NUM_LEDS;

    }
    else {
      int num_zeroes_before = fmax(0, NUM_LEDS - offset);

      char zeroes_before[num_zeroes_before];
      memset(zeroes_before, '0', num_zeroes_before);
      stradd(str, zeroes_before, 0, num_zeroes_before);

      // number of characters of the string to render
      int num_chars;

      if (offset < NUM_LEDS) {
        num_chars = offset;
      }
      else {
        num_chars = fmin(NUM_LEDS, size - (offset - NUM_LEDS));
      }

      // cut the string at this point
      int cut_position = fmax(0, offset - NUM_LEDS);

      char cut[num_chars];
      memcpy(cut, &seq[cut_position], num_chars);
      stradd(str, cut, num_zeroes_before, num_chars);

      if (NUM_LEDS > num_zeroes_before + num_chars) {
        int num_zeroes_after = NUM_LEDS - (num_zeroes_before + num_chars);

        char zeroes_after[num_zeroes_after];
        memset(zeroes_after, '0', num_zeroes_after);
        stradd(str, zeroes_after, num_zeroes_before + num_chars, num_zeroes_after);
      }

      offset = (offset + 1) % (size + NUM_LEDS);
    }

    str[NUM_LEDS] = '\0';

    set_pattern(fd, str);

    usleep(SCROLL_INTERVAL);
  }
}

/*
 * Ping-pong ball effect
 */
void do_pong(int fd) {
  int offset = 0;
  int direction = 1;
  int speed_factor = 2;

  while (1) {
    char str[NUM_LEDS];

    int num_zeroes_before = offset;
    int num_zeroes_after = NUM_LEDS - offset - 1;

    char zeroes_before[num_zeroes_before];
    char zeroes_after[num_zeroes_after];

    memset(zeroes_before, '0', num_zeroes_before);
    memset(zeroes_after, '0', num_zeroes_after);

    stradd(str, zeroes_before, 0, num_zeroes_before);
    str[offset] = '1';
    stradd(str, zeroes_after, num_zeroes_before + 1, num_zeroes_after);

    set_pattern(fd, str);

    offset = (offset + direction * speed_factor) % NUM_LEDS;

    if (offset == NUM_LEDS - speed_factor) {
      direction = -1;
    }
    else if (offset == 0) {
      direction = 1;
    }

    usleep(PONG_INTERVAL);
  }
}

void seconds_to_days(double seconds, char *pattern) {
  // max 9999 days = 27 years
  double days_raw = fmin(seconds / 86400, 9999);
  
  int days = floor(days_raw);

  // decimal point precision
  int precision = 0;

  if (days < 10) {
    precision = 3;
  }
  else if (days < 100) {
    precision = 2;
  }
  else if (days < 1000) {
    precision = 1;
  }

  int last_digits = round((days_raw - days) * pow(10, precision));

  days = days * pow(10, precision) + last_digits;

  int u1 = days % 10;
  int u2 = (int)floor(days / 10) % 10;
  int u3 = (int)floor(days / 100) % 10;
  int u4 = (int)floor(days / 1000) % 10;

  int dp1 = precision == 1;
  int dp2 = precision == 2;
  int dp3 = precision == 3;

  sprintf(pattern, "%d%d%d%d00%d%d0%d", dp3, u4, dp2, u3, dp1, u2, u1);
}

void do_time(int fd, bool temps) {
  // makes the LEDs display the time since system installation
  double seconds;
  long seconds_int;

  while (1) {
    get_time_seconds(&seconds);

    seconds_int = (int)(seconds);

    set_seconds_pattern(fd, &seconds_int);
  
    if (temps) {
      do_temps(fd);
    }
    else {
      // display the time-since-install on the LED display
      char pattern[10];
      seconds_to_days(seconds, pattern);

      set_display(fd, pattern);
    }

    usleep(1000000);
  }
}

void do_uptime(int fd, bool temps) {
  // makes the LEDs display the system uptime
  double uptime_seconds_float;

  while (1) {
    get_uptime_seconds(&uptime_seconds_float);

    long uptime_seconds = uptime_seconds_float >= 0
      ? (long)(uptime_seconds_float + 0.5)
      : (long)(uptime_seconds_float - 0.5);

    set_seconds_pattern(fd, &uptime_seconds);
  
    if (temps) {
      do_temps(fd);
    }
    else {
      // display the uptime on the LED display
      char pattern[10];
      seconds_to_days(uptime_seconds_float, pattern);

      set_display(fd, pattern);
    }

    usleep(1000000);
  }
}

void do_cpu_monitor(int fd) {
  double cpu_usage;

  char leds[NUM_LEDS];

  while (1) {
    get_cpu_usage(&cpu_usage);

    const int num_leds_lit = (int)(round(cpu_usage * NUM_LEDS));

    memset(leds, '1', num_leds_lit);
    
    char zeroes[NUM_LEDS - num_leds_lit];
    memset(zeroes, '0', NUM_LEDS - num_leds_lit);
    stradd(leds, zeroes, num_leds_lit, NUM_LEDS - num_leds_lit);

    set_pattern(fd, leds);

    do_temps(fd);
  }
}

void do_mem_monitor(int fd) {
  double mem_usage;

  char leds[NUM_LEDS];

  while (1) {
    get_mem_usage(&mem_usage);

    const int num_leds_lit = (int)(round(mem_usage * NUM_LEDS));

    memset(leds, '1', num_leds_lit);
    
    char zeroes[NUM_LEDS - num_leds_lit];
    memset(zeroes, '0', NUM_LEDS - num_leds_lit);
    stradd(leds, zeroes, num_leds_lit, NUM_LEDS - num_leds_lit);

    set_pattern(fd, leds);

    usleep(MEM_INTERVAL);
  }
}

void do_quiet(int fd) {
  // turns all the LEDs off

  char str[NUM_LEDS + 1];
  
  memset(str, '0', NUM_LEDS);
  str[NUM_LEDS] = '\0';

  set_pattern(fd, str);

  return;
}

int main(int argc, char *argv[]) {
  /* set up serial device */
  const char *portname = "/dev/ttyACM0";
  int fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);

  if (fd < 0) {
    printf("error %d opening %s: %s", fd, portname, strerror(fd));
    return 1;
  }

  set_interface_attribs(fd, B9600, 0); // 9600 baud, no parity
  set_blocking(fd, 0);		             // set no blocking

  /* handle command line arguments */
  if (argc > 1) {
    char *task = argv[1]; 

    bool temps = argc < 3 || strcmp(argv[2], "no_temps");

    if (!strcmp(task, "uptime")) {
      printf("** Enabling uptime clock\n");
      do_uptime(fd, temps);
    }
    else if (!strcmp(task, "time")) {
      printf("** Enabling clock\n");
      do_time(fd, temps);
    }
    else if (!strcmp(task, "quiet")) {
      printf("** Turning everything off\n"); 
      do_quiet(fd);
    }
    else if (!strcmp(task, "scrolltext")) {
      if (argc < 3) {
        printf("You need to supply some text to scroll with!\n");
        return 2;
      }
      
      printf("** Doing scrolling text\n"); 

      char *text = argv[2];
      do_scrolltext(fd, text);
    }
    else if (!strcmp(task, "pong")) {
      printf("** Doing pong thingy\n"); 
      do_pong(fd);
    }
    else if (!strcmp(task, "cpu")) {
      printf("** Running CPU monitor\n");
      do_cpu_monitor(fd);
    }
    else if (!strcmp(task, "mem")) {
      printf("** Running Memory monitor\n");
      do_mem_monitor(fd);
    }
    else if (!strcmp(task, "word")) {
      if (argc < 3) {
        printf("You need to supply a word!\n");
        return 2;
      }

      char *text = argv[2];
      
      printf("** Displaying word %s\n", text); 

      do_word(fd, text);
    }
  }

  return 0;
}
