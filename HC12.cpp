/**
 * @file HC12.cpp
 * @author Giel Willemsen
 * @brief Implementation of the wrapper around the HC12 433mhz packet radio.
 * @version 0.1 2022-06-11 Initial implementation with support for changing baud, FU mode, transmission power and channel.
 * @version 0.2 2022-06-12 Renamed PowerMode to OperationalMode and SendPower to TransmitPower.
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
#ifndef HC12_DEBUG_MODE
#define LOG(x) Serial.println(x)
#else
#define LOG(x)
#endif

HC12::HC12(Stream &serial, unsigned int setPin, Baudrates baud, OperationalMode mode, unsigned int channel, TransmitPower power) : serial(serial), setPin(setPin),
                                                                                                                         newBaudrate((int)baud),
                                                                                                                         baudChanged(false),
                                                                                                                         newPowerMode(mode),
                                                                                                                         powerModeChanged(false),
                                                                                                                         newChannel(channel),
                                                                                                                         channelChanged(false),
                                                                                                                         newSendPower(power),
                                                                                                                         sendPowerChanged(false)
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
    this->baudChanged = (int)baudrate != this->baudrate;
    this->newBaudrate = (int)baudrate;
}

void HC12::PrepareOperationalMode(HC12::OperationalMode mode)
{
    this->powerModeChanged = mode != this->powerMode;
    this->newPowerMode = mode;
}

void HC12::PrepareChannel(int channel)
{
    this->channelChanged = channel != this->channel;
    this->newChannel = channel;
}

void HC12::PrepareTransmitPower(HC12::TransmitPower power)
{
    this->sendPowerChanged = power != this->sendPower;
    this->newSendPower = power;
}

bool HC12::UpdateParams()
{
    CommandMode cmd(this->setPin);
    bool success = true;
    if (this->baudChanged && !this->UpdateBaudrate())
    {
        LOG("Baudrate update failure 1.");
        success = false;
    }

    if (this->channelChanged && !this->UpdateChannel())
    {
        LOG("Channel update failure 1.");
        success = false;
    }
    if (!this->channelChanged && !this->RequestChannel())
    {
        LOG("Channel update failure 2.");
        success = false;
    }
    if (this->sendPowerChanged && !this->UpdateTransmitPower())
    {
        LOG("Transmit power update failure 1.");
        success = false;
    }
    if (!this->sendPowerChanged && !this->RequestTransmitPower())
    {
        LOG("Request transmit power update failure 2.");
        success = false;
    }

    // Update this one after updating the baud rate because not all modes support all baudrates and setting a mode
    // will forcefully change the baudrate to a supported one.
    if (this->powerModeChanged && !this->UpdateOperationalMode())
    {
        LOG("Operational mode update failure 1.");
        success = false;
    }
    if (!this->powerModeChanged && !this->RequestOperationalMode())
    {
        LOG("Operational mode update failure 2.");
        success = false;
    }

    if (!this->baudChanged && !this->RequestBaudrate())
    {
        LOG("Baudrate update failure 2.");
        success = false;
    }
    return success;
}

unsigned int HC12::GetBaudrate()
{
    return this->baudrate;
}

HC12::OperationalMode HC12::GetOperationalMode()
{
    return this->powerMode;
}

unsigned int HC12::GetChannel()
{
    return this->channel;
}

HC12::TransmitPower HC12::GetTransmitPower()
{
    return this->sendPower;
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
        this->baudrate = 9600;
        this->channel = 1;
        this->sendPower = TransmitPower::mW_100_0;
        this->powerMode = OperationalMode::FU3;
        LOG("Reset was successfull.");
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
    const String kBaudrateString = String(this->newBaudrate);
    String result = this->SendCommandAndGetResult("AT+B" + kBaudrateString);
    bool success = (result == ("OK+B" + kBaudrateString));
    if (success)
    {
        this->baudrate = this->newBaudrate;
        this->baudChanged = false;
    }
    else
    {
        LOG("Received baudrate confirmation in baudrate update wasn't a known supported value.");
    }
    return success;
}

bool HC12::RequestBaudrate()
{
    String result = this->SendCommandAndGetResult("AT+RB");
    if (result.startsWith("OK+B") == false)
    {
        return false;
    }
    result.remove(0, 4);
    int baud = result.toInt();
    bool success = IsBaudrate((Baudrates)baud);
    if (success)
    {
        this->baudrate = baud;
        this->baudChanged = this->baudrate != this->newBaudrate;
    }
    else
    {
        LOG("Received baudrate wasn't a known supported value.");
    }

    return success;
}

bool HC12::UpdateOperationalMode()
{
    const String kPowerModeString = String((int)this->newPowerMode);
    String result = this->SendCommandAndGetResult("AT+FU" + kPowerModeString);
    if (result.startsWith("AT+FU") == false)
    {
        LOG("FU update command reply didn't start with AT+FU.");
        return false;
    }
    result.remove(0, 5);
    OperationalMode power = (OperationalMode)result.substring(0, 1).toInt();
    bool power_success = IsOperationalMode(power);
    if (power_success)
    {
        this->powerMode = power;
        this->powerModeChanged = this->powerMode != this->newPowerMode;
    }
    else
    {
        LOG("Received FU mode wasn't a supported value.");
    }
    return power_success;
}

bool HC12::RequestOperationalMode()
{
    String result = this->SendCommandAndGetResult("AT+RF");
    if (result.startsWith("OK+FU") == false)
    {
        LOG("FU value request reply didn't start  AT+FU.");
        return false;
    }
    result.remove(0, 5);
    OperationalMode power = (OperationalMode)result.substring(0, 1).toInt();
    bool power_success = IsOperationalMode(power);
    if (power_success)
    {
        this->powerMode = power;
        this->powerModeChanged = this->powerMode != this->newPowerMode;
    }
    else
    {
        LOG("Received FU mode wasn't a valid mode.");
    }

    Baudrates baud = (Baudrates)result.substring(3, result.length()).toInt();
    bool baud_success = IsBaudrate(baud);
    if (baud_success)
    {
        this->baudrate = (int)baud;
    }
    else
    {
        LOG("Received baudrate in FU mode wasn't a known supported value.");
    }
    return power_success;
}

bool HC12::UpdateChannel()
{
    String channelString = String(this->newChannel);
    while (channelString.length() != 3)
    {
        channelString = "0" + channelString;
    }
    String result = this->SendCommandAndGetResult("AT+C" + channelString);
    bool success = (result == ("OK+C" + channelString));
    if (success)
    {
        this->channel = this->newChannel;
        this->channelChanged = false;
    }
    else
    {
        LOG("Received channel wasn't the same as what was send.");
    }
    return success;
}

bool HC12::RequestChannel()
{
    String result = this->SendCommandAndGetResult("AT+RC");
    if (result.startsWith("OK+RC") == false)
    {
        LOG("Channel value request reply didn't start with AT+RC.");
        return false;
    }
    result.remove(0, 5);
    int channel = result.toInt();
    bool success = channel > 0 && channel < 127;
    if (success)
    {
        this->channel = channel;
        this->channelChanged = this->channel != this->newChannel;
    }
    else
    {
        LOG("Received channel wasn't between 0 and 128 (non-inclusive).");
    }
    return success;
}

bool HC12::UpdateTransmitPower()
{
    const String kChannelString = String((int)this->newSendPower);
    String result = this->SendCommandAndGetResult("AT+P" + kChannelString);
    bool success = (result == ("OK+P" + kChannelString));
    if (success)
    {
        this->sendPower = this->newSendPower;
        this->sendPowerChanged = false;
    }
    else
    {
        LOG("Received channel didn't match what was requested.");
    }
    return success;
}

bool HC12::RequestTransmitPower()
{
    String result = this->SendCommandAndGetResult("AT+RP");
    if (result.startsWith("OK+RP:") == false)
    {
        LOG("Transmission power value request reply didn't start with AT+RP.");
        return false;
    }
    if (result.endsWith("dBm") == false)
    {
        LOG("Transmission power value request reply didn't end with dBm.");
        return false;
    }
    result.remove(0, 6);
    result.remove(result.length() - 3, 3);
    TransmitPower power = TransmitPower::mW_0_8;
    bool success = DbmToTransmitPower(result.toInt(), power);
    if (success)
    {
        this->sendPower = power;
        this->sendPowerChanged = this->sendPower != this->newSendPower;
    }
    else
    {
        LOG("Received dBm value wasn't a valid TransmitPower value.");
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
