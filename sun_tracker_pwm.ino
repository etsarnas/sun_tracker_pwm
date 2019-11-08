/*
	Name:       sun_tracker_pwm.ino
	Created:	4/12/2019 8:26:46 PM
	Author:     Manny
*/


#include "spa.h"
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include "RTClib.h"
#include <Timer.h>                    //Timer Library
#include <Servo.h>


int PAN_SERVO = 15;
int PAN_SERVO_PIN = 23;

int TILT_SERVO = 14;
int TILT_SERVO_PIN = 23;

Timer t;
Timer t1;
spa_data spa;

RTC_DS3231 rtc;

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(); // default address 0x40

int day;
int month;
int year;
int hour;
int minute;
int sec;
int doy;
int dow;
int dst;
float latitude = 41.78528;    // 164 Hoyt St Brooklyn NY
float longitude = -73.98861;
float elevation;
float azimuth;
int timezone = 5;
double sunrise;
double sunset;

int dc, tHigh, tLow;
volatile int theta, thetaP;
int unitsFC = 360;                          // Units in a full circle
int dutyScale = 1000;                       // Scale duty cycle to 1/1000ths
int dcMin = 29;                             // Minimum duty cycle
int dcMax = 971;                            // Maximum duty cycle
volatile int angle, targetAngle;              // Global shared variables
volatile int Kp = 1;

volatile bool atZero = false;
volatile bool direction = false; //1 cw 0 ccw


void setup()
{
	SerialUSB.begin(115200);
	rtc.begin();

	// rtc.adjust(DateTime(__DATE__,__TIME__));
	 //getRTC();
	 //printdata();



	//if (rtc.lostPower()) {
	//	SerialUSB.println("RTC lost power, lets set the time!");
	//	// following line sets the RTC to the date & time this sketch was compiled
	//	rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
	//	// This line sets the RTC with an explicit date & time, for example to set
	//	// January 21, 2014 at 3am you would call:
	//rtc.adjust(DateTime(2019, 11, 6, 07, 11, 0));
	//}


	pinMode(TILT_SERVO_PIN, INPUT);
	pinMode(PAN_SERVO_PIN, INPUT);
	pwm.begin();
	pwm.setPWMFreq(60);
	//setServoPulse(TILT_SERVO, 1200);
	////Serial.println(map(1200, 1200, 2100, 0, 90));
	////Serial.println(map(1210, 1200, 2100, 0, 90));//1 = 1200 + 10
	////Serial.println(map(1220, 1200, 2100, 0, 90));
	////Serial.println(map(1650, 1200, 2100, 0, 90)); //45 = 1200 + (45 * 10)
	////Serial.println(map(2090, 1200, 2100, 0, 90));//89
	////Serial.println(map(2100, 1200, 2100, 0, 90)); //90
	//delay(1000);
	//setServoPulse(TILT_SERVO, 2100);


	setServoPulse(PAN_SERVO, 1700);
	delay(400);
	setServoPulse(PAN_SERVO, 1450);
	SetZero();

	t.every(120000, calculate_sun_position, (void*)0); //1 min
	t1.every(60, read_pin, (void*)0);
}


void loop()
{
	t.update();
	t1.update();


}

void read_pin(void *context) {

	int val = analogRead(PAN_SERVO_PIN); // Calculate angle

	theta = map(val, 0, 1023, 0, 360);

	if (targetAngle == 0) {

		if (val < 9 || val == 1023) {
			setServoPulse(PAN_SERVO, 1450);
		}
	}
	else {
		if (direction) {
			if (theta >= targetAngle) {
				setServoPulse(PAN_SERVO, 1450);
			}
		}
		else {

			if (targetAngle >= theta) {
				setServoPulse(PAN_SERVO, 1450);
			}
		}
	}
}

void MoveClockwise(int angle) {
	direction = 1;
	targetAngle = angle;
	setServoPulse(PAN_SERVO, 1800);
}


void MoveCounterclockwise(int angle) {
	direction = 0;
	targetAngle = angle;
	setServoPulse(PAN_SERVO, 1370);
}

/*
Clockwise slow to fast
1520
2000

Counterclockwise slow to fast
1000
1480

*/

void SetZero() {

	atZero = true;
	targetAngle = 0;
	setServoPulse(PAN_SERVO, 1360);
	setServoPulse(TILT_SERVO, 1200);
}

// you can use this function if you'd like to set the pulse length in seconds
// e.g. setServoPulse(0, 0.001) is a ~1 millisecond pulse width. its not precise!
void setServoPulse(uint8_t n, double pulse) {
	double pulselength;

	pulselength = 1000000;   // 1,000,000 us per second
	pulselength /= 60;   // 60 Hz
	pulselength /= 4096;  // 12 bits of resolution
   // pulse *= 1000000;  // convert to us
	pulse /= pulselength;
	pwm.setPWM(n, 0, pulse);
	delay(20);
}

int current_theta() {

	//// Measure feedback signal high/low times.
	//tLow = pulseIn(TILT_SERVO_PIN, 1);            // Measure low time 
	//tHigh = pulseIn(TILT_SERVO_PIN, 0);           // Measure high time

 // // Calcualte initial duty cycle and angle.
	//dc = (dutyScale * tHigh) / (tHigh + tLow);
	//return (unitsFC - 1) - ((dc - dcMin) * unitsFC) / (dcMax - dcMin + 1);

	int val = analogRead(PAN_SERVO_PIN);

	return map(val, 0, 1023, 0, 360);

}


void calculate_sun_position(void *context) {

	getRTC();

	spa.year = year;
	spa.month = month;
	spa.day = day;
	spa.hour = hour;
	spa.minute = minute;
	spa.second = sec;
	spa.delta_ut1 = -.01;
	spa.delta_t = 32.184 + 37.0 - .01;
	spa.timezone = dst - 5;
	spa.longitude = longitude;
	spa.latitude = latitude;
	spa.elevation = 10;
	spa.pressure = 1018;
	spa.temperature = 13;
	spa.slope = 0;
	spa.azm_rotation = 0;
	spa.atmos_refract = 0.5667;
	spa.function = SPA_ALL;

	spa_calculate(&spa);

	elevation = spa.e;
	azimuth = spa.azimuth;
	sunrise = spa.sunrise;
	sunset = spa.sunset;

	int cth = current_theta();
	setServoPulse(TILT_SERVO, 1200 + ((int)elevation * 10));

	if (atZero) {

		setServoPulse(PAN_SERVO, 1700);
		delay(400);
		cth = 0;
		atZero = false;
	}

	if (elevation > 0 && (int)azimuth > cth) {
		MoveClockwise((int)azimuth);
	}

	printdata();

}

void dstcalc() {
	//dst=0 -->Winter
	//dst=1 -->Summer

	int ct;
	int sundayrule; //march 2nd sunday, november 1st sunday
	dst = 0;

	DateTime tmp = rtc.now();
	//DateTime tmp = DateTime(2019, 3, 30, 6, 0, 0);

	int y = tmp.year();
	int m = tmp.month();
	//	int d = tmp.day();
	int h = tmp.hour();
	int mi = tmp.minute();
	int sec = tmp.second();

	if (m == 3 || m == 11) {
		ct = 0;
		sundayrule = (m == 3 ? 2 : 1); //dst changes 2 nd sunday in march or 1st sunday in november
		for (int k = 1; k <= (m == 3 ? 31 : 30); k++) {
			DateTime dt = DateTime(y, m, k, h, mi, sec);

			if (dt.dayOfTheWeek() == 0) {
				if (ct++ == sundayrule && tmp.day() >= k) dst = 1;
			}
		}
	}
	if (m > 3 && m < 11) {
		dst = 1;
	}
}

void getRTC() {

	DateTime now = rtc.now();

	sec = now.second();
	minute = now.minute();
	hour = now.hour();
	day = now.day();
	month = now.month();
	year = now.year();
	dow = now.dayOfTheWeek();

	dstcalc();

}

void printdata() {
	int dst_in_ram;

	Serial.flush();
	Serial.println();
	Serial.println("ACK");

	delay(10);

	switch (dow) {
	case 1:
		Serial.print("Mon");
		break;
	case 2:
		Serial.print("Tue");
		break;
	case 3:
		Serial.print("Wed");
		break;
	case 4:
		Serial.print("Thu");
		break;
	case 5:
		Serial.print("Fri");
		break;
	case 6:
		Serial.print("Sat");
		break;
	case 0:
		Serial.print("Sun");
		break;
	}
	Serial.print("   ");

	if (month < 10) {
		Serial.print("0");
	}
	Serial.print(month);
	Serial.print("/");

	if (day < 10) {
		Serial.print("0");
	}
	Serial.print(day);
	Serial.print("/");

	Serial.print(year);

	Serial.print("  ");

	if (hour < 10) {
		Serial.print("0");
	}
	Serial.print(hour);
	Serial.print(":");
	if (minute < 10) {
		Serial.print("0");
	}
	Serial.print(minute);
	Serial.print(":");
	if (sec < 10) {
		Serial.print("0");
	}

	Serial.print(sec);
	Serial.println();
	Serial.print("Time Zone = ");
	Serial.print((dst - timezone), DEC);
	Serial.println();
	Serial.print("DST = ");
	dst_in_ram = dst;// readRegister(REG_DST);
	switch (dst_in_ram) {
	case 1:Serial.print("Summer");
		break;
	case 0:Serial.print("Winter");
		break;
	}
	Serial.println();
	Serial.print("Coordinates = ");
	Serial.print(latitude, 6);
	Serial.print(" , ");
	Serial.println(longitude, 6);

	SerialUSB.print("dst: "); SerialUSB.println(dst);

	delay(10);

	Serial.print("Sunrise  ");  Serial.println(sunrise);
	Serial.print("Sunset  ");  Serial.println(sunset);
	Serial.print("Elevation  ");  Serial.println(elevation);
	delay(10);

	//Serial.println(zenith,0);
	//delay(10);

	Serial.print("Azimuth  ");  Serial.println(azimuth);

}
