/* Wii balanceboard weight measurement application        */
/* Renze Nicolai 2015                                     */
/* Version 1.0                      27-02-2015            */

/* Based on xwiishow, written 2010-2013 by David Herrmann */
/* Dedicated to the Public Domain                         */

#include <errno.h>
#include <poll.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "xwiimote.h"


// how many samples to collect for calculation, once you actually step on the board
#define MAX_SAMPLES 100

static struct xwii_iface *iface;

static int counter = 0;
static int countertoolow = 0;
static int totalval = 0;

/* error messages */

static void print_info(const char *format, ...)
{
	va_list list;
	char str[58 + 1];

	va_start(list, format);
	vsnprintf(str, sizeof(str), format, list);
	str[sizeof(str) - 1] = 0;
	va_end(list);

	printf( "%s", str);
}

static void print_error(const char *format, ...)
{
	va_list list;
	char str[58 + 80 + 1];

	va_start(list, format);
	vsnprintf(str, sizeof(str), format, list);
	str[sizeof(str) - 1] = 0;
	va_end(list);

	printf( "%s", str);
}

static void led_on(void)
{
	int ret;

	ret = xwii_iface_set_led(iface, XWII_LED(1), 1);
	if (ret) {
		print_error("Error: Cannot toggle LED 1: %d\n", ret);
	}
}

static void led_off(void)
{
	int ret;

	ret = xwii_iface_set_led(iface, XWII_LED(1), 0);
	if (ret) {
		print_error("Error: Cannot toggle LED 1: %d", ret);
	}
}
/*
void submit(int val) {
	char cmd[200];
	char vals[10];
	sprintf(vals, "%d", val);
	strcpy(cmd,"wget -O- 'http://<URL TO SUBMIT TO>/?<VAR>=");
	strcat(cmd,vals);
	strcat(cmd,"'");
	system(cmd);
} */

/* balance board */

static bool bboard_show_ext(const struct xwii_event *event, uint16_t *offset)
{
	uint16_t w, x, y, z;


	w = event->v.abs[0].x;
	x = event->v.abs[1].x;
	y = event->v.abs[2].x;
	z = event->v.abs[3].x;

//	printf( "W: %5d, X: %5d, Y: %5d, Z: %5d, T: %5d, CNT: %i (adjusted by %5d)\r ", w,x,y,z,w+x+y+z - *offset,counter, *offset);

//    if (w+x+y+z<3000) {
//        *offset = w+x+z+y;
//        printf("Adjusting weight offset : %5d\r",*offset);
//    }


    // assuming you are over 60kg, this is when the measurements are considered valid
	if (w+x+y+z > 6000) {
//	  led_off();
      counter++;
	  totalval += (w+x+y+z - *offset);  //adjusted for calibration, not sure if correct readings, though.
	  countertoolow = 0;
      printf("collecting measurement %i /%i\n",counter,MAX_SAMPLES);
	  if (counter > MAX_SAMPLES) {
            //led_on();
	    totalval = totalval / counter;
        printf("-----------------------------------------\n");
	    printf("Total value: %i, adjustment : %i\n",totalval, *offset);
	    //submit(totalval);
	    totalval = 0;
            counter = 0;
	    return false;
	  }
	} else {
	  //led_on();
	  countertoolow++;
          if (countertoolow > 1000) {
	    printf("No person (40KG) found. Shutting down.\n");
	    totalval = 0;
            counter = 0;
	    return false;
	  }
	}
	return true;
}

/*
static void bboard_clear(void)
{
	struct xwii_event ev;

	ev.v.abs[0].x = 0;
	ev.v.abs[1].x = 0;
	ev.v.abs[2].x = 0;
	ev.v.abs[3].x = 0;
	bboard_show_ext(&ev);
}
*/
/*
static void bboard_toggle(void)
{
	int ret;

	if (xwii_iface_opened(iface) & XWII_IFACE_BALANCE_BOARD) {
		xwii_iface_close(iface, XWII_IFACE_BALANCE_BOARD);
		bboard_clear();
		print_info("Info: Disable Balance Board");
	} else {
		ret = xwii_iface_open(iface, XWII_IFACE_BALANCE_BOARD);
		if (ret)
			print_error("Error: Cannot enable Balance Board: %d",
				    ret);
		else
			print_error("Info: Enable Balance Board");
	}
}
*/
/* LEDs */

static bool led1_state;

static void led1_show(bool on)
{
	if (on) {
        	printf("LED IS ON\n");
	} else {
        	printf("LED IS OFF\n");
	}
}
/*
static void led1_toggle(void)
{
	int ret;

	led1_state = !led1_state;
	ret = xwii_iface_set_led(iface, XWII_LED(1), led1_state);
	if (ret) {
		print_error("Error: Cannot toggle LED 1: %d", ret);
		led1_state = !led1_state;
	}
	led1_show(led1_state);
}
*/

static void led1_refresh(void)
{
	int ret;

	ret = xwii_iface_get_led(iface, XWII_LED(1), &led1_state);
	if (ret)
		print_error("Error: Cannot read LED state");
	else
		led1_show(led1_state);
}

/* battery status */

static void battery_show(uint8_t capacity)
{
	printf( "Battery: %3u%%\n", capacity);
}

static void battery_refresh(void)
{
	int ret;
	uint8_t capacity;

	ret = xwii_iface_get_battery(iface, &capacity);
	if (ret)
		print_error("Error: Cannot read battery capacity");
	else
		battery_show(capacity);
}

/* device type */

static void devtype_refresh(void)
{
	int ret;
	char *name;

	ret = xwii_iface_get_devtype(iface, &name);
	if (ret) {
		print_error("Error: Cannot read device type");
	} else {
		printf( "Devicetype: %s\n", name);
		free(name);
	}
}

static bool devtype_balanceboard(void)
{
	int ret;
	bool res = false;
	char *name;

	ret = xwii_iface_get_devtype(iface, &name);
	if (ret) {
		print_error("Error: Cannot read device type");
	} else {
		if ((name[0]=='b')&&(name[1]=='a')&&(name[2]=='l')) {
			res = true;
		}
		free(name);
	}
	return res;
}

/* basic window setup */

static void refresh_all(void)
{
	battery_refresh();
	led1_refresh();
	devtype_refresh();

	if (geteuid() != 0)
		printf("Warning: Please run as root! (sysfs+evdev access needed)");
}

/* device watch events */

static void handle_watch(void)
{
	static unsigned int num;
	int ret;

	print_info("Info: Watch Event #%u", ++num);

	ret = xwii_iface_open(iface, xwii_iface_available(iface) |
				     XWII_IFACE_WRITABLE);
	if (ret)
		print_error("Error: Cannot open interface: %d", ret);

	refresh_all();
}

static int run_iface(struct xwii_iface *iface)
{
	struct xwii_event event;
	int ret = 0;
	struct pollfd fds[2];

    uint16_t offset = 0;

	memset(fds, 0, sizeof(fds));
	fds[0].fd = 0;
	fds[0].events = POLLIN;
	fds[1].fd = xwii_iface_get_fd(iface);
	fds[1].events = POLLIN;

	ret = xwii_iface_watch(iface, true);
	if (ret)
		print_error("Error: Cannot initialize hotplug watch descriptor");

	//Bij het opstarten
	battery_refresh();
	bool isrunning = true;
	while (isrunning) {
		ret = poll(fds, 2, -1);
		if (ret < 0) {
			if (errno != EINTR) {
				ret = -errno;
				print_error("Error: Cannot poll fds: %d", ret);
				break;
			}
		}

		ret = xwii_iface_dispatch(iface, &event, sizeof(event));
		if (ret) {
			if (ret != -EAGAIN) {
				print_error("Error: Read failed with err:%d", ret);
				break;
			}
		} else {
			switch (event.type) {
			case XWII_EVENT_WATCH:
				handle_watch();
				break;
			case XWII_EVENT_BALANCE_BOARD:
				  isrunning = bboard_show_ext(&event,&offset);
				  //led1_toggle();
				break;
			}
		}
	}

	return ret;
}

static char *get_dev(int num)
{
	struct xwii_monitor *mon;
	char *ent;
	int i = 0;

	mon = xwii_monitor_new(false, false);
	if (!mon) {
		printf("Cannot create monitor\n");
		return NULL;
	}

	while ((ent = xwii_monitor_poll(mon))) {
		if (++i == num)
			break;
		free(ent);
	}

	xwii_monitor_unref(mon);

	if (!ent)
		printf("Cannot find device with number #%d\n", num);

	return ent;
}

void run(int num)
{
	int ret = 0;
	char *path = NULL;
	path = get_dev(num);
	free(path);
	ret = xwii_iface_open(iface,
			      xwii_iface_available(iface) |
			      XWII_IFACE_WRITABLE);
	if (ret)
		print_error("Error: Cannot open interface: %d",
			    ret);
	ret = run_iface(iface);
	xwii_iface_unref(iface);
	if (ret) {
		print_error("Program failed; press any key to exit");
	}
}

bool dcheck(int num)
{
	int ret = 0;
	bool disc = false;
	char *path = NULL;
	path = get_dev(num);
	ret = xwii_iface_new(&iface, path);
	free(path);
	if (ret) {
		printf("Cannot create xwii_iface.\n");
	} else {
		if (devtype_balanceboard()) {
			printf("> A balanceboard!\n");
			run(num);
			disc = true;
		} else {
			printf("> Not a balanceboard.\n");
		}
		xwii_iface_unref(iface);
	}
	return disc;
}

bool isbb(int num)
{
        int ret = 0;
        bool disc = false;
        char *path = NULL;
        path = get_dev(num);
        ret = xwii_iface_new(&iface, path);
        free(path);
        if (ret) {
                printf("Cannot create xwii_iface.\n");
        } else {
                if (devtype_balanceboard()) {
                        disc = true;
                }
                xwii_iface_unref(iface);
        }
        return disc;
}



static int enumerate()
{
	struct xwii_monitor *mon;
	char *ent;
	int num = 0;
	while (1) {
	//printf("Program started, creating monitor...\n");
	num = 0;
	mon = xwii_monitor_new(false, false);
	if (!mon) {
		printf("Cannot create monitor\n");
		return -EINVAL;
	}
	//printf("Monitor created. Reading...\n");

	while ((ent = xwii_monitor_poll(mon))) {
		printf("%d\t%s\n", ++num, ent);
		char mac[80];
                char getmac[300];
                strcpy(getmac,"ls ");
                strcat(getmac,ent);
                strcat(getmac,"/power_supply | grep 'wiimote_battery_' | cut -c17-");
		FILE *fp;
                fp = popen(getmac, "r");
                fgets(mac, sizeof(mac)-1, fp);
		free(ent);
		bool dobreak = false;
		if (isbb(num)) {
			xwii_monitor_unref(mon);
			dobreak = true;
		}
		printf("> MAC: %s",mac);
//this appears obsolete
		bool disconnect = dcheck(num);
		if (disconnect) {
			char str[80];
			strcpy(str,"bt-input -d ");
			strcat(str,mac);
			printf("Cmd: %s\n",str);
			system(str);
		}
		if (dobreak) { break; }
	}
	sleep(3);
	}
	return 0;
}

int main(int argc, char **argv)
{
  int ret = 0;
  char *path = NULL;
  if (argc < 2 || !strcmp(argv[1], "-h")) {
    printf("%s: Wii Balanceboard measurement tool\n",argv[0]);
    printf("\t%s [-h]: Show help\n",argv[0]);
    printf("\t%s auto: Connect automatically to all balance boards\n",argv[0]);
    printf("\t%s list: List available devices\n",argv[0]);
    printf("\t%s <num>: Use device with number #num\n",argv[0]);
    printf("\t%s /sys/path/to/device: Show given device\n",argv[0]);
    ret = -1;
  } else if (!strcmp(argv[1], "auto")) {
    printf("Automatic connection mode\n");
    ret = enumerate();
  } else {
		if (argv[1][0] != '/')
			path = get_dev(atoi(argv[1]));

		ret = xwii_iface_new(&iface, path ? path : argv[1]);
		free(path);
		if (ret) {
			printf("Cannot create xwii_iface '%s' err:%d\n", argv[1], ret);
		} else {
			printf("Created xwii_iface '%s' err:%d\n", argv[1], ret);

			ret = xwii_iface_open(iface, xwii_iface_available(iface) | XWII_IFACE_WRITABLE);
			if (ret)
				print_error("Error: Cannot open interface: %d", ret);

			ret = run_iface(iface);
			xwii_iface_unref(iface);
			if (ret) {
				print_error("Program failed; press any key to exit");
			}
		}
	}
	ret = enumerate();
	return abs(ret);
}

