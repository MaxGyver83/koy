/*
 * Copyright 2018 Thomas Bocek
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 */
 
 /*
  * Changes by Max Schillinger (2018):
  * - replaced function "qwerty2dvorak" by "qwertz2koy"
  * - replaced "dvorak" by "koy" in some comments
  */
 
 /*
  * Why is this tool useful?
  * ========================
  * 
  * Since I type with the "Dvorak" keyboard layout, the shortcuts such
  * as ctrl-c, ctrl-x, or ctrl-v are not comfortable anymore and one of them
  * require two hands to press.
  * 
  * Furthermore, applications such as Intellij and Eclipse have their
  * shortcuts, which I'm used to. So for these shortcuts I prefer "Querty".
  * Since there is no way to configure this, I had to intercept the
  * keys and remap the keys from "Dvorak" to "Querty" once CTRL, ALT, 
  * WIN or any of those combinations are pressed.
  * 
  * With X.org I was reling on the wonderful tool from Kenton Varda,
  * which I modified a bit, to make it work when Numlock is active. Other
  * than that, it worked as expected.
  * 
  * And then came Wayland. XGrabKey() works partially with some application
  * but not with others (e.g., gedit is not working). Since XGrabKey() is
  * an X.org function with some support in Wayland, I was looking for a more
  * stable solution. After a quick look to the repo https://github.com/kentonv/dvorak-qwerty
  * I saw that Kenton added a systemtap script to implement the mapping. This
  * scared me a bit to follow that path, so I implemented an other solution
  * based on /dev/uinput. The idea is to read /dev/input, grab keys with
  * EVIOCGRAB, create a virtual device that can emit the keys and pass 
  * the keys from /dev/input to /dev/uinput. If CTRL/ALT/WIN is
  * pressed it will map the keys back to "Qwerty".
  * 
  * Installation
  * ===========
  * 
  * make koy
  * //make sure your user belongs to the group "input" -> ls -la /dev/input
  * //this also applies for /dev/uinput -> https://github.com/tuomasjjrasanen/python-uinput/blob/master/udev-rules/40-uinput.rules
  * //start it in startup applications
  * 
  * Related Links
  * =============
  * I used the following sites for inspiration:
  * https://www.kernel.org/doc/html/v4.12/input/uinput.html
  * https://www.linuxquestions.org/questions/programming-9/uinput-any-complete-example-4175524044/
  * https://stackoverflow.com/questions/20943322/accessing-keys-from-linux-input-device
  * https://gist.github.com/toinsson/7e9fdd3c908b3c3d3cd635321d19d44d
  * 
  */
#define _GNU_SOURCE
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <string.h>
#include <stdio.h>

static const char *const evval[3] = {
    "RELEASED",
    "PRESSED",
    "REPEATED"
};

int emit(int fd, int type, int code, int val)
{
   struct input_event ie;
   ie.type = type;
   ie.code = code;
   ie.value = val;
   /* timestamp values below are ignored */
   ie.time.tv_sec = 0;
   ie.time.tv_usec = 0;

   return write(fd, &ie, sizeof(ie));
}

//from: https://github.com/kentonv/dvorak-qwerty/tree/master/unix
static int modifier_bit(int key) {
  switch (key) {
    case 29: return 1;     // l-ctrl
    case 97: return 2;     // r-ctrl
    case 56: return 4;     // l-alt
    case 125: return 8;    // win
  }
  return 0;
}

// same pattern as function "qwerty2dvorak" from
// https://github.com/kentonv/dvorak-qwerty/tree/master/unix
static int qwertz2koy(int key) {
  switch (key) {
    case 12: return 25; // ß
    case 13: return 13; // ´
    case 16: return 45; // Q
    case 17: return 51; // W
    case 18: return 32; // E
    case 19: return 37; // R
    case 20: return 36; // T
    case 21: return 26; // Z
    case 22: return 34; // U
    case 23: return 33; // I
    case 24: return 18; // O
    case 25: return 50; // P
    case 26: return 47; // Ü
    case 27: return 27; // + (no "+" key in KOY layout)
    case 30: return 31; // A
    case 31: return 39; // S
    case 32: return 35; // D
    case 33: return 40; // F
    case 34: return 22; // G
    case 35: return 30; // H
    case 36: return 53; // J
    case 37: return 16; // K
    case 38: return 24; // L
    case 39: return 48; // Ö
    case 40: return 46; // Ä
    case 44: return 20; // Y
    case 45: return 44; // X
    case 46: return 23; // C
    case 47: return 21; // V
    case 48: return 49; // B
    case 49: return 38; // N
    case 50: return 52; // M
    case 51: return 19; // ,
    case 52: return 17; // .
    case 53: return 12; // -
  }
  return key;
}

int main(int argc, char* argv[]) {
	
	setuid(0);
	
	if (argc < 2) {
		fprintf(stderr, "error: specify input device, e.g., found in "
		 "/dev/input/by-id/.\n");
        return EXIT_FAILURE;
	}
	
    struct input_event ev;
    ssize_t n;
    int fdi, fdo, i, mod_state, mod_current, array_counter, code, name_ret;
    struct uinput_user_dev uidev;
    const char MAX_LENGTH = 32;
    int array[MAX_LENGTH];
    char keyboard_name[256] = "Unknown";

    //the name and ids of the virtual keyboard, we need to define this now, as we need to ignore this to prevent
    //mapping the virtual keyboard

    memset(&uidev, 0, sizeof(uidev));
    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "Virtual KOY Keyboard");
    uidev.id.bustype = BUS_USB;
    uidev.id.vendor  = 0x1234;
    uidev.id.product = 0x5678;
    uidev.id.version = 0;

    //init states
    mod_state = 0;
    array_counter = 0;
    for (i=0;i<MAX_LENGTH;i++) {
    	array[i] = 0;
    }

	//get first input
	fdi = open(argv[1], O_RDONLY);
	if (fdi == -1) {
		fprintf(stderr, "Cannot open any of the devices: %s.\n", strerror(errno));
		return EXIT_FAILURE;
	}
	//
    name_ret = ioctl(fdi, EVIOCGNAME(sizeof(keyboard_name)-1), keyboard_name);
    if (name_ret < 0) {
        fprintf(stderr, "Cannot get device name: %s.\n", strerror(errno));
        return EXIT_FAILURE;
    }

    if (strcasestr(keyboard_name, uidev.name) != NULL) {
        fprintf(stderr, "Cannot get map the virtual device: %s.\n", keyboard_name);
        return EXIT_FAILURE;
    }

    // match names, reuse name_ret
    name_ret = -1;
    for (i = 2; i < argc; i++) {
        if (strcasestr(keyboard_name, argv[i]) != NULL) {
            printf("found input: [%s]\n", keyboard_name);
            name_ret = 0;
            break;
        }
    }
    if (name_ret < 0) {
        fprintf(stderr, "Not a matching device: [%s]\n", keyboard_name);
        return EXIT_FAILURE;
    }

    
    fdo = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fdo == -1) {
		fprintf(stderr, "Cannot open /dev/uinput: %s.\n", strerror(errno));
        return EXIT_FAILURE;
	}
	
	//grab the key, from the input
	//https://unix.stackexchange.com/questions/126974/where-do-i-find-ioctl-eviocgrab-documented/126996
	
	//fix is implemented, will make it to ubuntu sometimes in 1.9.4
	//https://bugs.freedesktop.org/show_bug.cgi?id=101796
	//quick workaround, sleep for 200ms...
	usleep(200 * 1000);
	
	if (ioctl(fdi, EVIOCGRAB, 1) == -1) {
		fprintf(stderr, "Cannot grab key: %s.\n", strerror(errno));
        return EXIT_FAILURE;
	}
	
	// Keyboard
	if (ioctl(fdo, UI_SET_EVBIT, EV_KEY) == -1) {
		fprintf(stderr, "Cannot set ev bits, key: %s.\n", strerror(errno));
        return EXIT_FAILURE;
	}
	if (ioctl(fdo, UI_SET_EVBIT, EV_SYN) == -1) {
		fprintf(stderr, "Cannot set ev bits, syn: %s.\n", strerror(errno));
        return EXIT_FAILURE;
	}
	if (ioctl(fdo, UI_SET_EVBIT, EV_MSC) == -1) {
		fprintf(stderr, "Cannot set ev bits, msc: %s.\n", strerror(errno));
        return EXIT_FAILURE;
	}
    
	// All keys
    for (i = 0; i < KEY_MAX; i++) {
        if (ioctl(fdo, UI_SET_KEYBIT, i) == -1) {
            fprintf(stderr, "Cannot set ev bits: %s.\n", strerror(errno));
			return EXIT_FAILURE;
		}
	}
	


    if (write(fdo, &uidev, sizeof(uidev)) == -1) {
        fprintf(stderr, "Cannot set device data: %s.\n", strerror(errno));
		return EXIT_FAILURE;
	}

    if (ioctl(fdo, UI_DEV_CREATE) == -1) {
		fprintf(stderr, "Cannot create device: %s.\n", strerror(errno));
		return EXIT_FAILURE;
	}
	
	//TODO: clear array

	while (1) {
        n = read(fdi, &ev, sizeof ev);
        if (n == (ssize_t)-1) {
            if (errno == EINTR) {
                continue;
			} else {
                break;
			}
        } else
        if (n != sizeof ev) {
            errno = EIO;
            break;
        }
		if (ev.type == EV_KEY && ev.value >= 0 && ev.value <= 2) {
			//printf("%s 0x%04x (%d), arr:%d\n", evval[ev.value], (int)ev.code, (int)ev.code, array_counter);
			//map the keys
			mod_current = modifier_bit(ev.code);  // can be 0, 1, 2, 4, 8
			if (mod_current > 0) { // key is Ctrl (left/right), Alt or Win
				if (ev.value == 1) { // pressed
					mod_state |= mod_current;  // add modifier bit
				} else if (ev.value == 0) { // released
					mod_state &= ~mod_current; // remove modifier bit
				}
			}

			if (ev.code != qwertz2koy(ev.code) && (mod_state > 0 || array_counter > 0)) {
				code = ev.code;
				//printf("koy %d, %d\n", array_counter, mod_state);
				if (ev.value==1) { //pressed
					if (array_counter == MAX_LENGTH) {
						printf("warning, too many keys pressed: %d. %s 0x%04x (%d), arr:%d\n",
								MAX_LENGTH, evval[ev.value], (int)ev.code, (int)ev.code, array_counter);
						//skip koy mapping
					} else {
						array[array_counter] = ev.code + 1; //0 means not mapped
						array_counter++;
						code = qwertz2koy(ev.code); // koy mapping
					}
				} else if (ev.value==0) { //released
					//now we need to check if the code is in the array
					//if it is, then the pressed key was in koy mode and
					//we need to remove it from the array. The ctrl or alt
					//key does not need to be pressed, when a key is released.
					//A previous implementation only had a counter, which resulted
					//occasionally in stuck keys.
					for (i=0; i<array_counter; i++) {
						if (array[i] == ev.code + 1) {
							//found it, map it!
							array[i] = 0;
							code = qwertz2koy(ev.code); // koy mapping
						}
					}
					//cleanup array counter
					for (i=array_counter-1; i>=0; i--) {
						if (array[i] == 0) {
							array_counter--;
						} else {
							break;
						}
					}
				}
				emit(fdo, ev.type, code, ev.value);
			} else {
				//printf("non koy %d\n", array_counter);
				emit(fdo, ev.type, ev.code, ev.value);
			}
		} else {
			//printf("Not key: %d 0x%04x (%d)\n", ev.value, (int)ev.code, (int)ev.code);
			emit(fdo, ev.type, ev.code, ev.value);
		}
    }
    fflush(stdout);
    fprintf(stderr, "%s.\n", strerror(errno));
    return EXIT_FAILURE;
}
