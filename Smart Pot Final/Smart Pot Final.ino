/** @Brief If you are using Arduino Uno*/
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
#include <CayenneMQTTEthernet.h>
#include <CayenneArduinoMQTTClient.h>
#endif

/** @Brief If you are using Arduino Yun*/
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega16U4__)
#include <CayenneMQTTYunClient.h>
#include <CayenneArduinoMQTTClient.h> 
#endif 

#pragma region Defines

/** @Brief Light Sensor's Pin and channel[5V]*/
#define LIGHT_SENSOR_PIN A4
#define LIGHT_SENSOR_CHANNEL 5

#define SOIL_HUMIDITY_SENSOR_PIN
#define SOIL_HUMIDITY_SENSOR_CHANNEL 

#define WATER_LEVEL_SENSOR_PIN 3
#define WATER_LEVEL_SENSOR_CHANNEL

#define DHT_SENSOR_PIN
#define DHT_SENSOR_CHANNEL

#define LAMP_PIN
#define LAMP_CHANNEL

#define PUMP_PIN 3
#define PUMP_CHANNEL 3

#pragma endregion


#pragma region Variables

/** @Brief Variable to store the Light sensor value */
uint16_t LightSensVal_g;
/** @Brief Variable to store the Humidity sensor's data*/
uint16_t SoilHumiditySensVal_g;
uint16_t WaterSensVal_g;
uint16_t DhtSensVal_g;

long int CurrentTime_g;
long int PreviousTime_g = 0;

const uint8_t rampTo_g = 245;
const uint8_t rampDelay_g = 5;
bool PumpState;

// Cayenne authentication info. This should be obtained from the Cayenne Dashboard.
char username[] = "";
char password[] = "";
char clientID[] = "";

#pragma endregion

/** @Brief this function will read a sensor and map it if some of the functions param are set to true
* @param uint8_t Pin,The pin that the sensor is connected to.
* @param bool mapPerc,Map the sensor value in percents[true/false].
* @param bool mapPWM,Map the sensor value in PWM range(Pulse Wide Modulation)[true/false].
* @param bool mappercB, Map the value in percents but in backward from 100% to 0%[true/false].
* @return uint16_t,Sensor Value
*/
uint16_t read_sens(uint8_t pin, bool mapperc = false, bool mappwm = false, bool mappercB = false);

void serial_info();



void setup()
{
	Cayenne.begin(username, password, clientID);
	Serial.begin(9600);

	/** @Brief Set the Light sensor pin to receive data*/
	pinMode(LIGHT_SENSOR_PIN, INPUT);

	pinMode(PUMP_PIN, OUTPUT);

}

void loop()
{
	serial_info();
	Cayenne.loop();

	if (SoilHumiditySensVal_g<10 && WaterSensVal_g>40)
	{
		for (uint8_t fadeValue = 0; fadeValue <= rampTo_g; fadeValue++)
		{
			analogWrite(PUMP_PIN, fadeValue);
			delay(rampDelay_g);
			if (fadeValue == rampTo_g) break;

		}
		PumpState = true;
	}
	else
	{
		for (uint8_t fadeValue = rampTo_g; fadeValue >= 0; fadeValue--)
		{
			analogWrite(PUMP_PIN, fadeValue);
			delay(rampDelay_g);
			if (fadeValue == 0)break;
		}
		PumpState = false;
	}
}


CAYENNE_OUT(LIGHT_SENSOR_CHANNEL)
{
	Cayenne.virtualWrite(LIGHT_SENSOR_CHANNEL, read_sens(LIGHT_SENSOR_PIN, true));
}

CAYENNE_OUT(PUMP_PIN)
{
	if (PumpState == true)
	{
		Cayenne.virtualWrite(PUMP_CHANNEL, "ON");
	}
	else Cayenne.virtualWrite(PUMP_CHANNEL, "OFF");

}

void serial_info()
{
	CurrentTime_g = millis();
	LightSensVal_g = read_sens(LIGHT_SENSOR_PIN, true);
	WaterSensVal_g = read_sens(WATER_LEVEL_SENSOR_PIN, true);
	SoilHumiditySensVal_g = read_sens(SOIL_HUMIDITY_SENSOR_PIN,false,false,true);

	if (CurrentTime_g - PreviousTime_g > 1000)
	{
		PreviousTime_g = CurrentTime_g;
		Serial.print("Light: ");
		Serial.println(LightSensVal_g);
	}
}

uint16_t read_sens(uint8_t pin, bool mapperc = false, bool mappwm = false, bool mappercB = false)
{
	uint16_t sensvall;
	uint16_t temp;

	sensvall = analogRead(pin);

	if (mapperc == true)
	{
		temp = map(sensvall, 0, 1023, 0, 100);
		return (uint8_t)temp;
	}

	if (mappercB == true)
	{
		temp = map(sensvall, 0, 1023, 100, 0);
		return (uint8_t)temp;
	}

	if (mappwm == true)
	{
		temp = map(sensvall, 0, 1023, 0, 255);
		return (uint8_t)temp;
	}

	if (mapperc == false && mappwm == false && mappercB == false)
	{
		return sensvall;
	}
}
