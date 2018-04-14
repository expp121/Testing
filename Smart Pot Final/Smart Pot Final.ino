#pragma region Libraries

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

#include <DHT.h>

#pragma endregion

#pragma region Defines

/** @Brief Used mainly for debugging(Serial information)
*/
#define Debug

/** @Brief The type of the DHT sensor
*/
#define DHTTYPE DHT11 //DHT21, DHT22 

/** @Brief Light Sensor's Pin and channel[5V]
* @ Used for mesuaring the luminosity 
*/
#define LIGHT_SENSOR_PIN A4
#define LIGHT_SENSOR_CHANNEL 5

/** @Brief Soil Moisture Humidity Sensor's Pin and channel [5V] 
* @ Used for mesuaring the soil's humidity	
*/
#define SOIL_HUMIDITY_SENSOR_PIN A2
#define SOIL_HUMIDITY_SENSOR_CHANNEL 2 

/** @Brief Water Level Sensor's Pin and channel [5V]
* @ Used for mesuaring the water level in a tank
*/
#define WATER_LEVEL_SENSOR_PIN A0	
#define WATER_LEVEL_SENSOR_CHANNEL 1

/** @Brief DHT sensor's Pin and channel [5V]  
* @ Used for gathering information(air temperature,air humidity)
* @ You will want to place a 10 Kohm resistor between VCC and the data pin
* @ How to do it : https://elementztechblog.files.wordpress.com/2014/06/elementz_dht11_bb.jpg
*/
#define DHT_SENSOR_PIN A1
#define DHT_SENSOR_CHANNEL 0

/** @Brief Lamp's  Pin and channel [12V]
* @ Used for Lighting the plant when it's dark
*/
#define LAMP_PIN 5
#define LAMP_CHANNEL 6

/** @Brief Pump's Pin and channel [12V]
* @ Used for Watering the plant
*/
#define PUMP_PIN 3
#define PUMP_CHANNEL 3

#pragma endregion

#pragma region Variables

/**  @Brief Creating the DHT object
*/
DHT dht(DHT_SENSOR_PIN, DHTTYPE);

/** @Brief Variable to store the Light sensor value
*/
uint16_t LightSensVal_g;

/** @Brief Variable to store the Humidity sensor's value
*/
uint16_t SoilHumiditySensVal_g;

/** @Brief Variable to store the Water level sensor's value
*/
uint16_t WaterSensVal_g;

/** @Brief Variable to store the DHT sensor's Temperature value 
*/
uint16_t DhtSensTemp_g;

/** @Brief Variable to store the DHT sensor's Humidity value
*/
uint16_t DhtSensHum_g;

long int CurrentTime_g;

#ifdef Debug

	/** @Brief Timer for printing Serial info */
	long int PreviousSerialTime_g = 0;
	short int IntervalSerial_g = 1;//[sec]
#endif 

// TODO: Correctly impelemnt time because i don't want to drown the plant and slowly turn on/off the lamp
/*
long int PreviousWatered_g=0;
short int  WateringInterval_g = 60;

long int PreviousLightAdjustment_g=0;
short int  LightAdjustingInterval_g = 30;
*/


/** @Brief These are the max percents soil humidity that allowed to start the pump[%]
*/
uint8_t SoilHumidityMaxVal_g = 20;

/** @Brief These are the min percent fullness of the tank that allowed to start the pump[%]
*/
uint8_t TankMinfullnessVal_g = 30;

/** @Brief Max Pwm Value for the pump to ramp to
*/
const uint8_t  PumpRampTo_g = 245;
/** @Brief Delay between ramp up's [miliseconds]
*/
const uint8_t PumpRampDelay_g = 5;

/** @Brief Flag for the state of the pump[True/false]
*/
bool PumpState;

/** @Brief Cayenne authentication info. This should be obtained from the Cayenne Dashboard.\
*/
char username[] = "";
char password[] = "";
char clientID[] = "";

#pragma endregion

#pragma region Function Prototypes

/** @Brief this function will read and send more stable data from a sensors and map it if some of the functions params are set to true
* @param uint8_t Pin,The pin that the sensor is connected to.
* @param uint8_t Delay, Time to wait between reads[ms].
* @param uint8_t tests,how many time to read from the sensor(the higher the number the more stable data you will get).
* @param bool mapPerc,Map the sensor value in percents[true/false].
* @param bool mapPWM,Map the sensor value in PWM range(Pulse Wide Modulation)[true/false].
* @param bool mapPWMB,Map the sensor value in PWM range(Pulse Wide Modulation) but backwards from 255 to 0[true/false].
* @param bool mappercB, Map the value in percents but in backward from 100% to 0%[true/false].
* @return uint16_t,Sensor Value
*/
uint16_t read_sens(uint8_t pin, uint8_t Delay = 1, uint8_t tests = 10, bool mapperc = false, bool mappwm = false, bool mappwnB = false, bool mappercB = false);

/** @Brief Read from a light level sensor and set the brightness level for a lamp.
* @return Void.
*/
void adjust_lights();

/**@Brief This function controls the interval that other functions will execute.
* @return Void.
*/
void proc_handler();

/**@Brief If the humidity in the soil is below certain percentageand the water in the reservoir is below certain percentage slowly start a pump,else slowly turn off the pump.
* @return Void.
*/
void watering_the_plant();

#ifdef Debug 
void serial_info();
#endif

#pragma endregion

void setup()
{
	Cayenne.begin(username, password, clientID);
	Serial.begin(9600);
	dht.begin();

	/** @Brief Set the Light sensor pin to receive data
	*/
	pinMode(LIGHT_SENSOR_PIN, INPUT);

	/** @Brief Set the Soil Humidity sensor pin to receive data
	*/
	pinMode(SOIL_HUMIDITY_SENSOR_PIN,INPUT);

	/** @Brief Set the Water Level sensor pin to receive data
	*/
	pinMode(WATER_LEVEL_SENSOR_PIN,INPUT);

	/** @Brief Set the Lamp pin to send data
	*/
	pinMode(LAMP_PIN,OUTPUT);

	/** @Brief Set the Pump pin to send data
	*/
	pinMode(PUMP_PIN, OUTPUT);
}

void loop()
{
#ifdef Debug

	serial_info();

#endif

	Cayenne.loop();

	proc_handler();
}

#pragma region Functions

CAYENNE_OUT(LIGHT_SENSOR_CHANNEL)
{
	Cayenne.virtualWrite(LIGHT_SENSOR_CHANNEL, read_sens(LIGHT_SENSOR_PIN, 1, 10, true));
}

CAYENNE_OUT(PUMP_PIN)
{
	if (PumpState == true)
	{
		Cayenne.virtualWrite(PUMP_CHANNEL, "ON");
	}
	else Cayenne.virtualWrite(PUMP_CHANNEL, "OFF");

}

#ifdef Debug

void serial_info()
{
	CurrentTime_g = millis();
	LightSensVal_g = read_sens(LIGHT_SENSOR_PIN, 1, 10, true);
	WaterSensVal_g = read_sens(WATER_LEVEL_SENSOR_PIN, 1, 10, true);
	SoilHumiditySensVal_g = read_sens(SOIL_HUMIDITY_SENSOR_PIN, 1, 10, false, false, true);

	if (CurrentTime_g - PreviousSerialTime_g > IntervalSerial_g*1000)
	{
		PreviousSerialTime_g = CurrentTime_g;
		Serial.print("Light: ");
		Serial.print(LightSensVal_g);
		Serial.print("%");
		Serial.print("	Water Level: ");
		Serial.print(WaterSensVal_g);
		Serial.println("%");
		Serial.print("Air Temperature: ");
		Serial.print(dht.readTemperature());
		Serial.print("C* ");
		Serial.print("	Air Humidity: ");
		Serial.print(dht.readHumidity());
		Serial.println("% ");
		if (read_sens(WATER_LEVEL_SENSOR_PIN, 1, 10, true) < TankMinfullnessVal_g) Serial.println("Fill the water tank");
	}
}

#endif

uint16_t read_sens(uint8_t pin, uint8_t Delay = 1, uint8_t tests = 10, bool mapperc = false, bool mappwm = false, bool mappwnB = false, bool mappercB = false)
{
	uint16_t sensvall;
	uint16_t temp;

	for (uint8_t sample = 0; sample < tests; sample++)
	{
		sensvall += analogRead(pin);
		delay(Delay);
	}
	sensvall = sensvall / tests;

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

	if (mappwnB == true)
	{
		temp = map(sensvall, 0, 1023, 255, 0);
		return (uint8_t)temp;
	}

	if (mapperc == false && mappwm == false && mappercB == false && mappwnB == false)
	{
		return sensvall;
	}
}


void proc_handler()
{
	watering_the_plant();
	adjust_lights();
}

void watering_the_plant()
{
	SoilHumiditySensVal_g = read_sens(SOIL_HUMIDITY_SENSOR_PIN, 5, 10, false, false, false, true);
	WaterSensVal_g = read_sens(WATER_LEVEL_SENSOR_PIN, 5, 10, true);
	if (SoilHumiditySensVal_g<SoilHumidityMaxVal_g && WaterSensVal_g>TankMinfullnessVal_g)
	{
		for (uint8_t fadeValue = 0; fadeValue <= PumpRampTo_g; fadeValue++)
		{
			analogWrite(PUMP_PIN, fadeValue);
			delay(PumpRampDelay_g);
			if (fadeValue == PumpRampTo_g) break;
		}
		PumpState = true;
	}
	else
	{
		for (uint8_t fadeValue = PumpRampTo_g; fadeValue >= 0; fadeValue--)
		{
			analogWrite(PUMP_PIN, fadeValue);
			delay(PumpRampDelay_g);
			if (fadeValue == 0)break;
		}
		PumpState = false;
	}
}

void adjust_lights()
{
	//Read the sensor.
	uint16_t sensor_valueL = read_sens(LIGHT_SENSOR_PIN, 1, 10, false, false, true);
	
	//Set the mapped value for the brightness of the lamp.
	analogWrite(LAMP_PIN, sensor_valueL);
}

#pragma endregion