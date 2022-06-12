/**
 * @file HC12.cpp
 * @author Giel Willemsen
 * @brief Implementation of the wrapper around the HC12 433mhz packet radio.
 * @version 0.1 2022-06-11 Initial implementation with support for changing baud, FU mode, transmission power and channel.
 * @version 0.2 2022-06-12 Renamed PowerMode to OperationalMode and SendPower to TransmitPower.
 * @version 0.3 2022-06-12 Refactor the code to keep track of changes into small helper class.
 * @version 0.4 2022-06-12 Small bug fix where updating the FU mode doesn't always return the baudrate update. Only if it changed.
 * @date 2022-06-12
 *
 * @copyright Copyright (c) 2022
 *
 */
#include <Arduino.h>
#include "HC12.h"

// Middle way between everything as a flash string vs having it constantly in memory.
// #define STR_F(str) String(F(str))
#define STR_F(str) F(str))
#ifdef HC12_DEBUG_MODE
#define LOG(x)              \
    Serial.print("(L");     \
    Serial.print(__LINE__); \
    Serial.print(") ");     \
    Serial.println(x)
#else
#define LOG(x)
#endif

HC12::HC12(Stream &serial, unsigned int setPin, Baudrates baud, OperationalMode mode, unsigned int channel, TransmitPower power) : serial(serial), setPin(setPin),
                                                                                                                                   baudrate((int)baud),
                                                                                                                                   operationalMode(OperationalMode::FU3),
                                                                                                                                   channel(1),
                                                                                                                                   transmitPower(TransmitPower::mW_100_0)
{
    pinMode(setPin, OUTPUT_OPEN_DRAIN);
}

bool HC12::begin()
{
    CommandMode cmd(this->setPin);
    bool result = SendCommandAndGetOK("AT");
    return result;
}

void HC12::PrepareBaudrate(HC12::Baudrates baudrate)
{
    if (IsBaudrate(baudrate))
    {
        this->baudrate.New() = (int)baudrate;
    }
}

void HC12::PrepareOperationalMode(HC12::OperationalMode mode)
{
    this->operationalMode.New() = mode;
}

void HC12::PrepareChannel(int channel)
{
    this->channel.New() = channel;
}

void HC12::PrepareTransmitPower(HC12::TransmitPower power)
{
    this->transmitPower.New() = power;
}

bool HC12::UpdateParams()
{
    CommandMode cmd(this->setPin);
    bool success = true;
    if (this->transmitPower.HasChanged() && !this->UpdateBaudrate())
    {
        LOG("Baudrate update failure 1.");
        success = false;
    }

    if (this->channel.HasChanged() && !this->UpdateChannel())
    {
        LOG("Channel update failure 1.");
        success = false;
    }
    if (!this->channel.HasChanged() && !this->RequestChannel())
    {
        LOG("Channel update failure 2.");
        success = false;
    }
    if (this->transmitPower.HasChanged() && !this->UpdateTransmitPower())
    {
        LOG("Transmit power update failure 1.");
        success = false;
    }
    if (!this->transmitPower.HasChanged() && !this->RequestTransmitPower())
    {
        LOG("Request transmit power update failure 2.");
        success = false;
    }

    // Update this one after updating the baud rate because not all modes support all baudrates and setting a mode
    // will forcefully change the baudrate to a supported one.
    if (this->operationalMode.HasChanged() && !this->UpdateOperationalMode())
    {
        LOG("Operational mode update failure 1.");
        success = false;
    }
    if (!this->operationalMode.HasChanged() && !this->RequestOperationalMode())
    {
        LOG("Operational mode update failure 2.");
        success = false;
    }

    if (!this->baudrate.HasChanged() && !this->RequestBaudrate())
    {
        LOG("Baudrate update failure 2.");
        success = false;
    }
    return success;
}

unsigned int HC12::GetBaudrate()
{
    return this->baudrate.Current();
}

HC12::OperationalMode HC12::GetOperationalMode()
{
    return this->operationalMode.Current();
}

unsigned int HC12::GetChannel()
{
    return this->channel.Current();
}

HC12::TransmitPower HC12::GetTransmitPower()
{
    return this->transmitPower.Current();
}

bool HC12::Sleep()
{
    CommandMode cmd(this->setPin);
    return this->SendCommandAndGetResult("AT+SLEEP") == "AT+SLEEP";
}

bool HC12::Reset()
{
    CommandMode cmd(this->setPin);
    String result = this->SendCommandAndGetResult("AT+DEFAULT");
    bool success = (result == "OK+DEFAULT");
    if (success)
    {
        this->baudrate = Updatable<unsigned int>(9600);
        this->channel = Updatable<int>(1);
        this->transmitPower = Updatable<TransmitPower>(TransmitPower::mW_100_0);
        this->operationalMode = Updatable<OperationalMode>(OperationalMode::FU3);
        LOG("Reset was successful.");
    }
    return success;
}

int HC12::available()
{
    return this->serial.available();
}

int HC12::read()
{
    return this->serial.read();
}

int HC12::peek()
{
    return this->serial.peek();
}

size_t HC12::write(uint8_t data)
{
    return this->serial.write(data);
}

bool HC12::SendCommandAndGetOK(Stream &serial, const String &command)
{
    String response = SendCommandAndGetResult(serial, command);
    return response == "OK";
}

String HC12::SendCommandAndGetResult(Stream &serial, const String &command)
{
    SendCommand(serial, command);
    auto oldTimeout = serial.getTimeout();
    serial.setTimeout(kMaxCommandResponseTime);
    String response = serial.readStringUntil('\n');
    response.trim();
    serial.setTimeout(oldTimeout);
    return response;
}

void HC12::SendCommand(Stream &serial, const String &command)
{
    // Drop old data since in command mode this can't be valid userdata anymore.
    while (serial.available())
    {
        serial.read();
    }
    serial.write(command.c_str());
    serial.write('\r');
    serial.write('\n');
}

bool HC12::UpdateBaudrate()
{
    const String kBaudrateString = String(this->baudrate.New());
    String result = this->SendCommandAndGetResult("AT+B" + kBaudrateString);
    bool success = (result == ("OK+B" + kBaudrateString));
    if (success)
    {
        this->baudrate.MarkUpdated();
    }
    else
    {
        LOG("Received baudrate confirmation in baudrate update wasn't a known supported value. Response was: " + result + ".");
    }
    return success;
}

bool HC12::RequestBaudrate()
{
    String result = this->SendCommandAndGetResult("AT+RB");
    if (result.startsWith("OK+B") == false)
    {
        LOG("Baudrate receive request didn't start with OK+B. Response was: " + result + ".");
        return false;
    }
    result.remove(0, 4);
    int baud = result.toInt();
    bool success = IsBaudrate((Baudrates)baud);
    if (success)
    {
        this->baudrate.ForceUpdateCurrent(baud);
    }
    else
    {
        LOG("Received baudrate wasn't a known supported value. Value was: " + String(baud) + ".");
    }

    return success;
}

bool HC12::UpdateOperationalMode()
{
    const String kPowerModeString = String((int)this->operationalMode.New());
    String result = this->SendCommandAndGetResult("AT+FU" + kPowerModeString);
    if (result.startsWith("OK+FU") == false)
    {
        LOG("FU update command reply didn't start with AT+FU but was: " + result + ".");
        return false;
    }
    result.remove(0, 5);
    OperationalMode power = (OperationalMode)result.substring(0, 1).toInt();
    bool power_success = IsOperationalMode(power);
    if (power_success)
    {
        this->operationalMode.MarkUpdated();
    }
    else
    {
        LOG("Received FU mode wasn't a supported value. Value was: " + String((int)power) + ".");
    }
    return power_success;
}

bool HC12::RequestOperationalMode()
{
    String result = this->SendCommandAndGetResult("AT+RF");
    if (result.startsWith("OK+FU") == false)
    {
        LOG("FU value request reply didn't start AT+FU but was: " + result + ".");
        return false;
    }
    result.remove(0, 5);
    OperationalMode power = (OperationalMode)result.substring(0, 1).toInt();
    bool power_success = IsOperationalMode(power);
    if (power_success)
    {
        this->operationalMode.ForceUpdateCurrent(power);
    }
    else
    {
        LOG("Received FU mode wasn't a valid mode. Value was: " + String((int)power) + ". Full response: " + result + ".");
    }

    if (result.length() > 1)
    {
        Baudrates baud = (Baudrates)result.substring(3, result.length()).toInt();
        bool baud_success = IsBaudrate(baud);
        if (baud_success)
        {
            this->baudrate.ForceUpdateCurrent((int)baud);
        }
        else
        {
            LOG("Received baudrate in FU mode wasn't a known supported value. Value was: " + String((int)baud) + ". Full response: " + result + ".");
        }
        power_success = power_success && baud_success;
    }
    return power_success;
}

bool HC12::UpdateChannel()
{
    String channelString = String(this->channel.New());
    while (channelString.length() != 3)
    {
        channelString = "0" + channelString;
    }
    String result = this->SendCommandAndGetResult("AT+C" + channelString);
    bool success = (result == ("OK+C" + channelString));
    if (success)
    {
        this->channel.MarkUpdated();
    }
    else
    {
        LOG("Received channel wasn't the same as what was send. Response was: " + result + ".");
    }
    return success;
}

bool HC12::RequestChannel()
{
    String result = this->SendCommandAndGetResult("AT+RC");
    if (result.startsWith("OK+RC") == false)
    {
        LOG("Channel value request reply didn't start with AT+RC but was: " + result + ".");
        return false;
    }
    result.remove(0, 5);
    int channel = result.toInt();
    bool success = channel > 0 && channel < 127;
    if (success)
    {
        this->channel.ForceUpdateCurrent(channel);
    }
    else
    {
        LOG("Received channel wasn't between 0 and 128 (non-inclusive). Response was: " + String(channel) + ".");
    }
    return success;
}

bool HC12::UpdateTransmitPower()
{
    const String kChannelString = String((int)this->transmitPower.New());
    String result = this->SendCommandAndGetResult("AT+P" + kChannelString);
    bool success = (result == ("OK+P" + kChannelString));
    if (success)
    {
        this->transmitPower.MarkUpdated();
    }
    else
    {
        LOG("Received channel didn't match what was requested. Response was: " + result + ".");
    }
    return success;
}

bool HC12::RequestTransmitPower()
{
    String result = this->SendCommandAndGetResult("AT+RP");
    if (result.startsWith("OK+RP:") == false)
    {
        LOG("Transmission power value request reply didn't start with AT+RP. Response was: " + result + ".");
        return false;
    }
    if (result.endsWith("dBm") == false)
    {
        LOG("Transmission power value request reply didn't end with dBm. Response was: " + result + ".");
        return false;
    }
    result.remove(0, 6);
    result.remove(result.length() - 3, 3);
    TransmitPower power = TransmitPower::mW_0_8;
    bool success = DbmToTransmitPower(result.toInt(), power);
    if (success)
    {
        this->transmitPower.ForceUpdateCurrent(power);
    }
    else
    {
        LOG("Received dBm value wasn't a valid TransmitPower value. Value was: " + result + ".");
    }
    return success;
}

bool HC12::DbmToTransmitPower(int dbm, TransmitPower &power)
{
    bool success = true;
    switch (dbm)
    {
    case -1:
        power = TransmitPower::mW_0_8;
        break;
    case 2:
        power = TransmitPower::mW_1_6;
        break;
    case 5:
        power = TransmitPower::mW_3_2;
        break;
    case 8:
        power = TransmitPower::mW_6_3;
        break;
    case 11:
        power = TransmitPower::mW_12_0;
        break;
    case 14:
        power = TransmitPower::mW_25_0;
        break;
    case 17:
        power = TransmitPower::mW_50_0;
        break;
    case 20:
        power = TransmitPower::mW_100_0;
        break;
    default:
        success = false;
        break;
    }
    return success;
}
