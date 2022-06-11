# arduino-hc12
A simple Arduino (Arduino IDE & PlatformIO) library for the HC-12 433MHz packet radio module.
It can be used as a normal stream but also has possiblities to enter the command mode and change the parameters of the radio.

# Basic usage
In the most basic version you begin the serial port you want to use and you construct a HC12 with the serial as a refence.
And you are good to go then.

```cpp
#include "HC12.h"
#define HC12_SET_PIN 5
HC12 hc12(Serial1, HC12_SET_PIN);
void setup()
{
    Serial.begin(9600);
    Serial1.begin(9600);
    if (hc12.begin() == false)
    {
        Serial.println("Failed to connect to the HC12 module. Check wiring.");
        while(1){}
    }
}
void loop()
{
    if (hc12.available())
    {
        Serial.write(hc12.read());
    }
    if (Serial.available())
    {
        hc12.write(Serial.read());
    }
}
```

# Find baudrate for modem
The library also includes a method to search the known supported bitrates and see if it replies to the AT check command.

```cpp
#include "HC12.h"
#define HC12_SET_PIN 5
void setup()
{
    Serial.begin(9600);
    Serial1.begin(9600);
    int hc12_baud = HC12::FindBaudrateForModule(Serial1, HC12_SET_PIN);
	if (hc12_baud == 0)
	{
		Serial.println("Failed to detect HC12 modem module. Check wiring.");
		while (1)
		{
		}
	}
    Serial1.begin(hc12_baud);
    Serial.println(String("Detected HC12 at ") + String(hc12_baud) + " baud");
}
void loop()
{
}
```


# Change the transmission power
Besides being able to just data the library also provides options to change the parameters of the radio.
It can change the transmission power, operational mode, communication baudrate and the radio channel.
You first call 'PrepareXXXX'.
And when you call 'UpdateParams' it will syncronize the parameters.
It will update the modem with the values from PrepareXXX.
And if a value was changed by something else (or because it has never synced before) it will also retrieve those values.

NOTE:
If you change the operation power mode it is good to check that the baudrate changed to what you expected.
Not all power modes support the same baudrates so the module might change the baudrate if you change the mode.


```cpp
#include "HC12.h"
#define HC12_SET_PIN 5
HC12 hc12(Serial1, HC12_SET_PIN);
void setup()
{
    Serial.begin(9600);
    Serial1.begin(9600);
    if (hc12.begin() == false)
    {
        Serial.println("Failed to connect to the HC12 module. Check wiring.");
        while(1){}
    }
	hc12.PrepareSendPower(HC12::SendPower::mW_6_3);
    if (hc12.UpdateParams() == false)
    {
        Serial.println("Failed to syncronise settings with the HC12 module. Aborting program.");
        while(1){}
    }
    Serial.println("HC-12 transmission power changed to 6,3 mW");
}

void loop()
{
    if (hc12.available())
    {
        Serial.write(hc12.read());
    }
    if (Serial.available())
    {
        hc12.write(Serial.read());
    }
}
```
