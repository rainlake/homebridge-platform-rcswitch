#include <stdio.h>
#include <wiringPi.h>

#include "rcswitch.h"

static void rcswitch_handle_interrupt();
static int rcswitch_receive_protocol1(unsigned int changeCount);
static int rcswitch_receive_protocol2(unsigned int changeCount);
static void rcswitch_send0(int protocol, int pin, int pulse);
static void rcswitch_send1(int protocol, int pin, int pulse);

static char* rcswitch_dec2bin_wzerofill(unsigned long dec, unsigned int bit_length);
static void rcswitch_transmit(int pin, int pulse, int nHighPulses, int nLowPulses);
static void rcswitch_send_sync(int protocol, int pin, int pulse);

static int nReceiveTolerance;
static unsigned int timings[RCSWITCH_MAX_CHANGES];

static unsigned int duration;
static unsigned int changeCount;
static unsigned long lastTime;
static unsigned int repeatCount;
static data_ready_t data_ready_cb;
static void *user_data;
void rcswitch_init() {
	rcswitch_set_receive_tolerance(60);
}
void rcswitch_set_data_ready_cb(data_ready_t cb, void *data) {
	data_ready_cb = cb;
	user_data = data;
}
void rcswitch_set_receive_tolerance(int percent) {
	nReceiveTolerance = percent;
}
int rcswitch_enable_receive(int interrupt) {
	if(wiringPiSetup() == -1) {
		return 1;
	}
	wiringPiISR(interrupt, INT_EDGE_BOTH, &rcswitch_handle_interrupt);
	return 0;
}
static void rcswitch_handle_interrupt() {
	long time = micros();
	duration = time - lastTime;

	if (duration > 5000 && duration > timings[0] - 200 && duration < timings[0] + 200) {    
		repeatCount++;
		changeCount--;

		if (repeatCount == 2) {
			if (!rcswitch_receive_protocol1(changeCount) && !rcswitch_receive_protocol2(changeCount)){
				// failed
			}
			repeatCount = 0;
		}
		changeCount = 0;
	} else if (duration > 5000) {
		changeCount = 0;
	}

	if (changeCount >= RCSWITCH_MAX_CHANGES) {
		changeCount = 0;
		repeatCount = 0;
	}
	timings[changeCount++] = duration;
	lastTime = time;
}

static int rcswitch_receive_protocol1(unsigned int changeCount){
	unsigned long code = 0;
	unsigned long delay = timings[0] / 31;
	unsigned long delayTolerance = delay * nReceiveTolerance * 0.01;   
	int i =0;
	for (i = 1; i< changeCount; i += 2) {
		if (timings[i] > delay - delayTolerance && timings[i] < delay + delayTolerance
			&& timings[i+1] > delay*3-delayTolerance && timings[i+1] < delay*3+delayTolerance) {
			code = code << 1;
		} else if (timings[i] > delay*3-delayTolerance && timings[i] < delay*3+delayTolerance
			&& timings[i+1] > delay-delayTolerance && timings[i+1] < delay+delayTolerance) {
			code += 1;
			code = code << 1;
		} else {
			// Failed
			code = 0;
			break;
		}
	}      
	code = code >> 1;
	if (changeCount > 6 && code && data_ready_cb) {    // ignore < 4bit values as there are no devices sending 4bit values => noise
		data_ready_cb(code, delay, 1, changeCount / 2, user_data);
		data_ready_cb = NULL;
	}
	return code;
}

static int rcswitch_receive_protocol2(unsigned int changeCount){
	unsigned long code = 0;
	unsigned long delay = timings[0] / 10;
	unsigned long delayTolerance = delay * nReceiveTolerance * 0.01;
	int i =0;

	for (i = 1; i<changeCount ; i += 2) {
		if (timings[i] > delay-delayTolerance && timings[i] < delay+delayTolerance
			&& timings[i+1] > delay*2-delayTolerance && timings[i+1] < delay*2+delayTolerance) {
			code = code << 1;
		} else if (timings[i] > delay*2-delayTolerance && timings[i] < delay*2+delayTolerance
			&& timings[i+1] > delay-delayTolerance && timings[i+1] < delay+delayTolerance) {
			code+=1;
			code = code << 1;
		} else {
            // Failed
			i = changeCount;
			code = 0;
		}
	}      
	code = code >> 1;
	if (changeCount > 6 && code && data_ready_cb) {    // ignore < 4bit values as there are no devices sending 4bit values => noise
		data_ready_cb(code, delay, 2, changeCount / 2, user_data);
		data_ready_cb = NULL;
    }
	return code;
}

void rcswitch_send(int protocol, int pin, int repeat, int pulse, unsigned long code, unsigned int length) {
	rcswitch_send_scode_word(protocol, pin, repeat, pulse, rcswitch_dec2bin_wzerofill(code, length) );
}

void rcswitch_send_scode_word(int protocol, int pin, int repeat, int pulse, char* sCodeWord) {
	if (wiringPiSetup () == -1) {
        fprintf(stderr,"wiringPiSetup failed %d\n");
    }
	pinMode(pin, OUTPUT);
	int nRepeat = 0, i = 0;
	for (nRepeat=0; nRepeat< repeat; nRepeat++) {
		i = 0;
		while (sCodeWord[i] != '\0') {
			switch(sCodeWord[i]) {
				case '0':
					rcswitch_send0(protocol, pin, pulse);
					break;
				case '1':
					rcswitch_send1(protocol, pin, pulse);
					break;
			}
			i++;
		}
		rcswitch_send_sync(protocol, pin, pulse);
	}
}
/**
 * Sends a "0" Bit
 *                       _    
 * Waveform Protocol 1: | |___
 *                       _  
 * Waveform Protocol 2: | |__
 */
static void rcswitch_send0(int protocol, int pin, int pulse) {
	if (protocol == 1){
		rcswitch_transmit(pin, pulse, 1,3);
	}
	else if (protocol == 2) {
		rcswitch_transmit(pin, pulse, 1,2);
	}
}
/**
 * Sends a "1" Bit
 *                       ___  
 * Waveform Protocol 1: |   |_
 *                       __  
 * Waveform Protocol 2: |  |_
 */
static void rcswitch_send1(int protocol, int pin, int pulse) {
  	if (protocol == 1){
		rcswitch_transmit(pin, pulse, 3, 1);
	}
	else if (protocol == 2) {
		rcswitch_transmit(pin, pulse, 2, 1);
	}
}
/**
 * Sends a "Sync" Bit
 *                       _
 * Waveform Protocol 1: | |_______________________________
 *                       _
 * Waveform Protocol 2: | |__________
 */
static void rcswitch_send_sync(int protocol, int pin, int pulse) {

    if (protocol == 1){
		rcswitch_transmit(pin, pulse, 1, 31);
	}
	else if (protocol == 2) {
		rcswitch_transmit(pin, pulse, 1, 10);
	}
}

static void rcswitch_transmit(int pin, int pulse, int nHighPulses, int nLowPulses) {
	if(!pulse) {
		pulse = 350;
	}
	digitalWrite(pin, HIGH);
	delayMicroseconds(pulse * nHighPulses);
	digitalWrite(pin, LOW);
	delayMicroseconds(pulse * nLowPulses);
}

static char* rcswitch_dec2bin_wzerofill(unsigned long dec, unsigned int bit_length){
	static char bin[64];
	unsigned int i=0, j = 0;

	while (dec > 0) {
		bin[32+i++] = ((dec & 1) > 0) ? '1' : '0';
		dec = dec >> 1;
	}

	for (j = 0; j< bit_length; j++) {
		if (j >= bit_length - i) {
			bin[j] = bin[ 31 + i - (j - (bit_length - i)) ];
		} else {
			bin[j] = '0';
		}
	}
	bin[bit_length] = '\0';
	return bin;
}


