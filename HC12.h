/**
 * @file HC12.h
 * @author Giel Willemsen
 * @brief Wrapper around the HC12 433mhz packet radio.
 * @version 0.1 2022-06-11 Initial version that can change the settings and acts as a Stream. And has helper function to find the currently used baudrate.
 * @date 2022-06-11
 *
 * @copyright Copyright (c) 2022
 *
 */
#pragma once
#ifndef INCLUDE_ARDUINO_HC12_H
#define INCLUDE_ARDUINO_HC12_H

#include "Arduino.h"

class HC12 : public Stream
{
public:
    static constexpr unsigned long kMaxCommandResponseTime = 150UL;

    enum class PowerMode
    {
        FU1 = 1,
        FU2 = 2,
        FU3 = 3,
        FU4 = 4
    };

    enum class SendPower
    {
        mW_0_8 = 1,
        mW_1_6 = 2,
        mW_3_2 = 3,
        mW_6_3 = 4,
        mW_12_0 = 5,
        mW_25_0 = 6,
        mW_50_0 = 7,
        mW_100_0 = 8,
    };

    enum class Baudrates
    {
        BPS_1200 = 1200,
        BPS_2400 = 2400,
        BPS_4800 = 4800,
        BPS_9600 = 9600,
        BPS_19200 = 19200,
        BPS_138400 = 138400,
        BPS_57600 = 57600,
        BPS_115200 = 115200
    };

private:
    class CommandMode
    {
    private:
        int pin;

    public:
        CommandMode(int pin) : pin(pin)
        {
            digitalWrite(pin, LOW);
            delay(40);
        }

        ~CommandMode()
        {
            digitalWrite(pin, HIGH);
            delay(80);
        }
    };

private:
    Stream &serial;
    int setPin;

    unsigned int newBaudrate;
    unsigned int baudrate;
    bool baudChanged;
    PowerMode newPowerMode;
    PowerMode powerMode;
    bool powerModeChanged;
    int newChannel;
    int channel;
    bool channelChanged;
    SendPower newSendPower;
    SendPower sendPower;
    bool sendPowerChanged;

public:
    HC12(Stream &serial, unsigned int setPin, Baudrates baud = Baudrates::BPS_9600, PowerMode mode = PowerMode::FU2, unsigned int channel = 1, SendPower power = SendPower::mW_100_0);

    bool begin();

    void PrepareBaudrate(Baudrates baudrate);
    void PreparePowerMode(PowerMode mode);
    void PrepareChannel(int channel);
    void PrepareSendPower(SendPower power);

    bool UpdateParams();

    unsigned int GetBaudrate();
    PowerMode GetPowerMode();
    unsigned int GetChannel();
    SendPower GetSendPower();
    bool Sleep();
    bool Reset();

    virtual int available() override;
    virtual int read() override;
    virtual int peek() override;
    virtual size_t write(uint8_t data) override;

    template <typename T = HardwareSerial>
    static unsigned int FindBaudrateForModule(T &ser, int cmdPin)
    {
        CommandMode cmd(cmdPin);
        // Put these baudrates up front since they are much more common.
        ser.updateBaudRate(9600);
        Serial.println(F("Looking at baud: 9600."));
        if (SendCommandAndGetOK(ser, "AT"))
            return 9600;
        ser.updateBaudRate(115200);
        Serial.println(F("Looking at baud: 115200."));
        if (SendCommandAndGetOK(ser, "AT"))
            return 115200;
        ser.updateBaudRate(19200);
        Serial.println(F("Looking at baud: 19200."));
        if (SendCommandAndGetOK(ser, "AT"))
            return 19200;

        // Other 'regular' baudrates.
        ser.updateBaudRate(1200);
        Serial.println(F("Looking at baud: 1200."));
        if (SendCommandAndGetOK(ser, "AT"))
            return 1200;
        ser.updateBaudRate(2400);
        Serial.println(F("Looking at baud: 2400."));
        if (SendCommandAndGetOK(ser, "AT"))
            return 2400;
        ser.updateBaudRate(4800);
        Serial.println(F("Looking at baud: 4800."));
        if (SendCommandAndGetOK(ser, "AT"))
            return 4800;
        ser.updateBaudRate(138400);
        Serial.println(F("Looking at baud: 138400."));
        if (SendCommandAndGetOK(ser, "AT"))
            return 138400;
        ser.updateBaudRate(57600);
        Serial.println(F("Looking at baud: 57600."));
        if (SendCommandAndGetOK(ser, "AT"))
            return 57600;
        return 0;
    }

    static constexpr bool IsPowerMode(PowerMode mode)
    {
        return (mode == PowerMode::FU1 ||
                mode == PowerMode::FU2 ||
                mode == PowerMode::FU3 ||
                mode == PowerMode::FU4);
    }

    static constexpr bool IsSendPower(SendPower power)
    {
        return (
            power == SendPower::mW_0_8 ||
            power == SendPower::mW_1_6 ||
            power == SendPower::mW_3_2 ||
            power == SendPower::mW_6_3 ||
            power == SendPower::mW_12_0 ||
            power == SendPower::mW_25_0 ||
            power == SendPower::mW_50_0 ||
            power == SendPower::mW_100_0);
    }

    static constexpr bool IsBaudrate(Baudrates baud)
    {
        return (
            baud == Baudrates::BPS_1200 ||
            baud == Baudrates::BPS_2400 ||
            baud == Baudrates::BPS_4800 ||
            baud == Baudrates::BPS_9600 ||
            baud == Baudrates::BPS_19200 ||
            baud == Baudrates::BPS_138400 ||
            baud == Baudrates::BPS_57600 ||
            baud == Baudrates::BPS_115200);
    }

private:
    static void SendCommand(Stream &serial, const String &command);
    static bool SendCommandAndGetOK(Stream &serial, const String &command);
    static String SendCommandAndGetResult(Stream &serial, const String &command);

    void SendCommand(const String &command)
    {
        this->SendCommand(this->serial, command);
    }

    bool SendCommandAndGetOK(const String &command)
    {
        return this->SendCommandAndGetOK(this->serial, command);
    }

    String SendCommandAndGetResult(const String &command)
    {
        return this->SendCommandAndGetResult(this->serial, command);
    }

    bool UpdateBaudrate();
    bool RequestBaudrate();
    bool UpdatePowerMode();
    bool RequestPowerMode();
    bool UpdateChannel();
    bool RequestChannel();
    bool UpdateSendPower();
    bool RequestSendPower();

    bool DbmToSendPower(int dbm, SendPower &power);
};

#endif // INCLUDE_ARDUINO_HC12_H
