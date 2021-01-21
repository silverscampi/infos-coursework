/*
 * CMOS Real-time Clock
 * SKELETON IMPLEMENTATION -- TO BE FILLED IN FOR TASK (1)
 */

/*
 * STUDENT NUMBER: s1610745
 */
#include <infos/drivers/timer/rtc.h>
#include <arch/x86/pio.h>
#include <util/lock.h>

using namespace infos::drivers;
using namespace infos::drivers::timer;
using namespace infos::util;
using namespace infos::arch::x86;

class CMOSRTC : public RTC {
public:
	static const DeviceClass CMOSRTCDeviceClass;

	const DeviceClass& device_class() const override
	{
		return CMOSRTCDeviceClass;
	}

	/**
	 * Interrogates the RTC to read the current date & time.
	 * @param tp Populates the tp structure with the current data & time, as
	 * given by the CMOS RTC device.
	 */

	int update_in_progress() {
		__outb(0x70, 0x0A);
		return __inb(0x71);
	}

	uint8_t read_cmos(int reg) {
		__outb(0x70, reg);
		return __inb(0x71);
	}

	enum TimeUnit {
		sec,	// second
		min,	// minute
		hor,	// hour
		dom,	// day of month
		mon,	// month
		yer		// year
	};

	void read_timepoint(RTCTimePoint& tp) override
	{
		// Disable interrupts
		UniqueIRQLock irqlock;

		// Because interrupts are disabled and thus RTC cannot interrupt on a completed update, must avoid bad RTC values during updates by reading twice in a row:
		
		uint8_t last_buf[6];
		uint8_t new_buf[6];

		// Wait while update is in progress
		while (update_in_progress()) {};

		// Read RTC registers into buffer
		new_buf[sec] = read_cmos(0x00);
		new_buf[min] = read_cmos(0x02);
		new_buf[hor] = read_cmos(0x04);
		new_buf[dom] = read_cmos(0x07);
		new_buf[mon] = read_cmos(0x08);
		new_buf[yer] = read_cmos(0x09);

		do {
			for (int i=0; i<6; i++) {
				last_buf[i] = new_buf[i];
			}

			// Wait while update is in progress
			while (update_in_progress()) {};

			// Read RTC registers again 
			new_buf[sec] = read_cmos(0x00);
			new_buf[min] = read_cmos(0x02);
			new_buf[hor] = read_cmos(0x04);
			new_buf[dom] = read_cmos(0x07);
			new_buf[mon] = read_cmos(0x08);
			new_buf[yer] = read_cmos(0x09);
		} while(   last_buf[sec] != new_buf[sec]
				|| last_buf[min] != new_buf[min]
				|| last_buf[hor] != new_buf[hor]
				|| last_buf[dom] != new_buf[dom]
				|| last_buf[mon] != new_buf[mon]
				|| last_buf[yer] != new_buf[yer] );

		// Check if values are BCD - convert to binary if so
		uint8_t statusB = read_cmos(0x0B);
		if (~statusB & 0x02) {
			new_buf[sec] = (new_buf[sec] & 0x0F) + ((new_buf[sec] / 16) * 10);
			new_buf[min] = (new_buf[min] & 0x0F) + ((new_buf[min] / 16) * 10);
			new_buf[hor] =   ( (new_buf[hor] & 0x0F) + (((new_buf[hor] & 0x70) / 16) * 10) )
						   | (new_buf[hor] & 0x80);
			new_buf[dom] = (new_buf[dom] & 0x0F) + ((new_buf[dom] / 16) * 10);
			new_buf[mon] = (new_buf[mon] & 0x0F) + ((new_buf[mon] / 16) * 10);
			new_buf[yer] = (new_buf[yer] & 0x0F) + ((new_buf[yer] / 16) * 10);
		}

		// Return RTC time
		tp.seconds = new_buf[sec];
		tp.minutes = new_buf[min];
		tp.hours   = new_buf[hor];
		tp.day_of_month = new_buf[dom];
		tp.month   = new_buf[mon];
		tp.year    = new_buf[yer];
	}
};

const DeviceClass CMOSRTC::CMOSRTCDeviceClass(RTC::RTCDeviceClass, "cmos-rtc");

RegisterDevice(CMOSRTC);
