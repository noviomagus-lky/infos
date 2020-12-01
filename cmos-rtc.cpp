/*
 * CMOS Real-time Clock
 * SKELETON IMPLEMENTATION -- TO BE FILLED IN FOR TASK (3)
 */

/*
 * STUDENT NUMBER: s1891130
 */
#include <infos/drivers/timer/rtc.h>
#include <arch/x86/pio.h>
#include <infos/util/lock.h>

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
	void read_timepoint(RTCTimePoint& tp) override
	{
		
		uint8_t buffer[12] = {};
		uint8_t flag = 0;
		
		UniqueIRQLock l;

		//Wait for an update cycle to begin.
		while (1) {
			__outb(0x70, 0x0A);
			flag = __inb(0x71) & 0x80;

			if(flag != 0)
				break;
		}
		
		//Wait for the update cycle to finish.
		while (1) {
			__outb(0x70, 0x0A);
			flag = __inb(0x71) & 0x80;

			if (flag == 0)
				break;
		}
		
		//Read the data from RTC.
		for (int i = 0x00; i <= 0x0B; i++) {
			__outb(0x70, i);
			buffer[i] = __inb(0x71);
		}

		//Process data to get the time and date.If the format of data is DCB, 
		//first convert it to binary.
		if ((buffer[0x0B] & 0x04) == 0) {
			
			for (int i = 0; i <= 0x09; i++){
				
				buffer[i] = (buffer[i]>>4)*10 + (buffer[i] & 0x0f);
				
			}
		}

		//Store the data into the RTCTimePoint data structure.
		tp.seconds = buffer[0x00];
		tp.minutes = buffer[0x02];
		tp.hours = buffer[0x04];
		tp.day_of_month = buffer[0x07];
		tp.month = buffer[0x08];
		tp.year = buffer[0x09];		
	}
};

const DeviceClass CMOSRTC::CMOSRTCDeviceClass(RTC::RTCDeviceClass, "cmos-rtc");

RegisterDevice(CMOSRTC);
