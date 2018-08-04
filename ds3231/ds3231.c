/*********************************************************************
 * ds3231.c : DS3231 RTC Clock
 * Warren W. Gay VE3WWG
 *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <stdbool.h>
#include <linux/i2c-dev.h>

struct s_ds3231_regs {
	struct s_00 {			/* Seconds */
		uint8_t	secs_1s  : 4;	/* Ones digit: seconds */
		uint8_t	secs_10s : 3;	/* Tens digit: seconds */
		uint8_t	mbz_0    : 1;
	} s00;
	struct s_01 {			/* Minutes */
		uint8_t	mins_1s  : 4;	/* Ones digit: minutes */
		uint8_t	mins_10s : 3;	/* Tens digit: minutes */
		uint8_t	mbz_1    : 1;
	} s01;
	union u_02 {			/* Hours */
		struct	{
			uint8_t	hour_1s  : 4;	/* Ones digit: hours */
			uint8_t	hour_10s : 1;	/* Tens digit: hours (24hr mode) */
			uint8_t ampm	 : 1;	/* AM=0/PM=1 */
			uint8_t	mode_1224: 1;	/* Mode bit: 12=1/24=0 hour format */	
		} hr12;
		struct	{
			uint8_t	hour_1s  : 4;	/* Ones digit: hours */
			uint8_t	hour_10s : 3;	/* Tens digit: hours (24hr mode) */
			uint8_t	mode_1224: 1;	/* Mode bit: 12=1/24=0 hour format */	
		} hr24;
	} u02;
	struct s_03 {			/* Weekday */
		uint8_t	wkday    : 3;	/* Day of week (1-7) */
		uint8_t	mbz_2    : 5;
	} s03;
	struct s_04 {			/* Day of month */
		uint8_t	day_1s   : 4;	/* Ones digit: day of month (1-31) */
		uint8_t	day_10s  : 2;	/* Tens digit: day of month */
		uint8_t	mbz_3    : 2;
	} s04;
	struct s_05 {			/* Month */
		uint8_t	month_1s : 4;	/* Ones digit: month (1-12) */
		uint8_t	month_10s: 1;	/* Tens digit: month */
		uint8_t	mbz_4    : 2;
		uint8_t	century  : 1;	/* Century */
	} s05;
	struct s_06 {			/* Year */
		uint8_t	year_1s  : 4;	/* Ones digit: BCD year (00-99) */
		uint8_t	year_10s : 4;	/* Tens digit: BCD year */
	} s06;
	struct s_07 {			/* Alarm Seconds */
		uint8_t	alrms01	 : 4;	/* Alarm BCD 1s seconds */
		uint8_t	alrms10  : 3;	/* Alarm BCD 10s Seconds */
		uint8_t	AxM1     : 1;	/* Alarm Mask 1 */
	} s07;				/* Alarm Seconds */
	struct s_08 {			/* Alarm Minutes */
		uint8_t	alrmm01	 : 4;	/* Alarm BCD 1s Minutes */
		uint8_t	alrmm10  : 3;	/* Alarm BCD 10s Minutes */
		uint8_t	AxM2	 : 1;	/* Alarm Mask 2 */
	} s08;				/* Alarm Minutes */
	union u_09 {			/* Alarm Hours */
		struct 	{
			uint8_t	alr_hr10 : 1;	/* Alarm 10s Hours */
			uint8_t	alr_ampm : 1;	/* Alarm am=0/pm=1 */
			uint8_t	alr_1224 : 1;	/* Alarm 12=1 */
			uint8_t	AxM3	 : 1;	/* Alarm Mask 3 */
		} ampm;
		struct	{
			uint8_t alr_hr10 : 2;	/* Alarm 10s Hours */
			uint8_t	alr_1224 : 1;	/* Alarm 24=0 */
			uint8_t	AxM3	 : 1;	/* Alarm Mask 3 */
		} hrs24;
	} u09;				/* Alarm 1 Hours */
	union u_0A {			/* Alarm Date */
		struct 	{
			uint8_t	day10    : 2;	/* Alarm 10s date */
			uint8_t	dydt     : 1;	/* Alarm dy=1 */
			uint8_t	AxM4	 : 1;	/* Alarm Mask 4 */
		} dy;
		struct	{
			uint8_t	day10    : 3;	/* Alarm 10s date */
			uint8_t	dydt     : 1;	/* Alarm dt=0 */
			uint8_t	AxM4	 : 1;	/* Alarm Mask 4 */
		} dt;
	} u0A;
	struct s_08 s0B;		/* Alarm 2 Minutes */
	union u_09 u0C;			/* Alarm 2 Hours */
	union u_0A u0D;			/* Alarm 2 Date */
	struct s_0E {			/* Control */
		uint8_t A1IE	 : 1;	/* Alarm 1 Int enable */
		uint8_t A2IE	 : 1;	/* Alarm 2 Int enable */
		uint8_t INTCN	 : 1;	/* SQW signal when 1 */
		uint8_t RS1	 : 1;	/* Rate select 1 */
		uint8_t RS2	 : 1;	/* Rate select 2 */
		uint8_t CONV	 : 1;	/* Temp conversion */
		uint8_t BBSQW	 : 1;	/* Enable square wave */
		uint8_t	NEOSC	 : 1;	/* /EOSC: Enable */
	} s0E;
	struct s_0F {			/* Control/status */
		uint8_t A1F	 : 1;	/* Alarm 1 Flag */
		uint8_t A2F	 : 1;	/* Alarm 2 Flag */
		uint8_t bsy	 : 1;	/* Busy flag */
		uint8_t en32khz	 : 1;	/* Enable 32kHz out */
		uint8_t zeros	 : 3;
		uint8_t	OSF	 : 1;	/* Stop Osc when 1 */
	} s0F;
	struct s_10 {			/* Aging offset */
		int8_t data	 : 8;	/* Data */
	} s10;
	struct s_1112 {
		int16_t data	 : 10;	/* Signed temp */
		int16_t zeros	 : 6;	/* Zero bits */
	} s1112;
};
	
typedef struct s_ds3231_regs ds3231_regs_t;

static const char *node = "/dev/i2c-1";
static int i2c_fd = -1;			/* Device node: /dev/i2c-1 */
static unsigned long i2c_funcs = 0;	/* Support flags */

/*
 * Open I2C bus and check capabilities :
 */
static void
i2c_init(const char *node) {
	int rc;

	i2c_fd = open(node,O_RDWR);	/* Open driver /dev/i2s-1 */
	if ( i2c_fd < 0 ) {
		perror("Opening /dev/i2s-1");
		exit(1);
	}

	/*
	 * Make sure the driver supports plain I2C I/O:
	 */
	rc = ioctl(i2c_fd,I2C_FUNCS,&i2c_funcs);
	assert(rc >= 0);
	assert(i2c_funcs & I2C_FUNC_I2C);
}

/*
 * Close the I2C driver :
 */
static void
i2c_close(void) {
	close(i2c_fd);
	i2c_fd = -1;
}

/*
 * Read: [S] 0xB1 <regaddr> <rtcbuf[0]> ... <rtcbuf[n-1]> [P]
 */
static bool
i2c_rd_rtc(ds3231_regs_t *rtc) {
	struct i2c_rdwr_ioctl_data msgset;
	struct i2c_msg iomsgs[2];
	char zero = 0x00;		/* Register 0x00 */

	iomsgs[0].addr = 0x68;		/* DS3231 */
	iomsgs[0].flags = 0;		/* Write */
	iomsgs[0].buf = &zero;		/* Register 0x00 */
	iomsgs[0].len = 1;

	iomsgs[1].addr = 0x68;		/* DS3231 */
	iomsgs[1].flags = I2C_M_RD;	/* Read */
	iomsgs[1].buf = (char *)rtc;
	iomsgs[1].len = sizeof *rtc;

	msgset.msgs = iomsgs;
	msgset.nmsgs = 2;

	return ioctl(i2c_fd,I2C_RDWR,&msgset) == 2;
}

/*
 * Write [S] 0xB0 <regaddr> <rtcbuf[0]> ... <rtcbuf[n-1]> [P]
 */
static bool
i2c_wr_rtc(ds3231_regs_t *rtc) {
	struct i2c_rdwr_ioctl_data msgset;
	struct i2c_msg iomsgs[1];
	char buf[sizeof *rtc + 1];	/* Work buffer */

	buf[0] = 0x00;			/* Register 0x00 */
	memcpy(buf+1,rtc,sizeof *rtc);	/* Copy RTC info */

	iomsgs[0].addr = 0x68;		/* DS3231 Address */
	iomsgs[0].flags = 0;		/* Write */
	iomsgs[0].buf = buf;		/* Register + data */
	iomsgs[0].len = sizeof *rtc + 1; /* Total msg len */

	msgset.msgs = &iomsgs[0];
	msgset.nmsgs = 1;

	return ioctl(i2c_fd,I2C_RDWR,&msgset) == 1;
}

static void
usage(const char *argv0) {
	const char *cmd = strrchr(argv0,'/');

	if ( !cmd )
		cmd = argv0;

	printf(
		"Usage:\t%s [-s] [-f format] [-h]\n"
		"where:\n"
		"\t-s\tSet RTC clock based upon system date\n"
		"\t-f fmt\tSet date format\n"
		"\t-h\tThis help\n",
		cmd);
}

/*
 * Main program :
 */
int
main(int argc,char **argv) {
	static char options[] = "hsf:";
	ds3231_regs_t rtc;		/* DS3231 Registers */
	bool opt_s = false;
	struct tm t0, t1;		/* Unix date/time values */
	char *date_format = "%Y-%m-%d %H:%M:%S (%A)";
	char dtbuf[256];		/* Formatted date/time */
	int oc;

	while ( (oc = getopt(argc,argv,options)) != -1 ) {
		switch ( oc ) {
		case 'f':
			date_format = optarg;
			break;
		case 's':
			opt_s = true;
			break;
		case 'h':
			usage(argv[0]);
			exit(0);
		default:
			usage(argv[0]);
			exit(1);
		}		
	}
	
	/*
	 * Initialize I2C and clear rtc and t1 structures:
	 */
	i2c_init(node);				/* Initialize for I2C */
	memset(&rtc,0,sizeof rtc);
	memset(&t1,0,sizeof t1);

	if ( !i2c_rd_rtc(&rtc) ) {
		perror("Reading DS3231 RTC clock.");
		exit(1);
	}

	if ( opt_s ) {
		time_t now;
		struct tm t;
		int yr;

		time(&now);
		localtime_r(&now,&t);

		rtc.s05.century = (yr = t.tm_year) >= 100 ? 1 : 0;
		if ( rtc.s05.century ) 
			yr -= 100;

		rtc.s06.year_10s = yr / 10;
		rtc.s06.year_1s = yr % 10;
		rtc.s05.month_10s = (t.tm_mon + 1) / 10;
		rtc.s05.month_1s = (t.tm_mon + 1) % 10;
		rtc.s04.day_10s = t.tm_mday / 10;
		rtc.s04.day_1s = t.tm_mday % 10;
		rtc.s03.wkday = t.tm_wday + 1;
		rtc.u02.hr24.hour_10s = t.tm_hour / 10;
		rtc.u02.hr24.hour_1s = t.tm_hour % 10;
		rtc.s01.mins_10s = t.tm_min / 10;
		rtc.s01.mins_1s = t.tm_min % 10;

		if ( !i2c_wr_rtc(&rtc) ) {
			perror("Reading DS3231 RTC clock.");
			exit(1);
		}
		if ( !i2c_rd_rtc(&rtc) ) {
			perror("Reading DS3231 RTC clock.");
			exit(1);
		}
	}

	/*
	 * Check the date returned by the RTC:
	 */
	memset(&t1,0,sizeof t1);

	t1.tm_year = rtc.s06.year_10s * 10 + rtc.s06.year_1s;
	if ( rtc.s05.century ) 
		t1.tm_year += 100;

	t1.tm_mon  = rtc.s05.month_10s * 10 + rtc.s05.month_1s - 1;
	t1.tm_mday = rtc.s04.day_10s   * 10 + rtc.s04.day_1s;
	t1.tm_hour = rtc.u02.hr24.hour_10s * 10 + rtc.u02.hr24.hour_1s;
	t1.tm_min  = rtc.s01.mins_10s  * 10 + rtc.s01.mins_1s;
	t1.tm_sec  = rtc.s00.secs_10s  * 10 + rtc.s00.secs_1s;
	t1.tm_isdst = 0;	

	t0 = t1;
	if ( mktime(&t1) == 1L 			/* t1 is modified */
	  || t1.tm_year   != t0.tm_year || t1.tm_mon    != t0.tm_mon
	  || t1.tm_mday   != t0.tm_mday || t1.tm_hour   != t0.tm_hour
	  || t1.tm_min    != t0.tm_min  || t1.tm_sec    != t0.tm_sec ) {
		strftime(dtbuf,sizeof dtbuf,date_format,&t0);
		fprintf(stderr,"Read RTC date is not valid: %s\n",dtbuf);
		exit(2);
	}

	if ( t1.tm_wday != rtc.s03.wkday-1 ) {
		fprintf(stderr,
			"Warning: RTC weekday is incorrect %d but should be %d\n",
			rtc.s03.wkday,t1.tm_wday);
	}

	strftime(dtbuf,sizeof dtbuf,date_format,&t1);
	puts(dtbuf);

	i2c_close();
	return 0;
}