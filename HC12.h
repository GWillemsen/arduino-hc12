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

/**
 * @brief Class that helps with communicating with the HC12 module.
 * 
 */
class HC12 : public Stream
{
public:
    /**
     * @brief Maximum time that is allowed for a AT command to return a response.
     * 
     */
    static constexpr unsigned long kMaxCommandResponseTime = 150UL;

    /**
     * @brief The different FU power modes the module can be in.
     * 
     */
    enum class PowerMode
    {
        FU1 = 1,
        FU2 = 2,
        FU3 = 3,
        FU4 = 4
    };

    /**
     * @brief The different transmit powers the module has. Both mapped in milli Watt and in dBm.
     * 
     */
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
        dBm_NEG_1 = mW_0_8,
        dBm_2 = mW_1_6,
        dBm_5 = mW_3_2,
        dBm_8 = mW_6_3,
        dBm_11 = mW_12_0,
        dBm_14 = mW_25_0,
        dBm_17 = mW_50_0,
        dBm_20 = mW_100_0
    };

    /**
     * @brief The supported baudrates by the HC-12 module.
     * 
     */
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
    /**
     * @brief Small helper class that on construction enters command mode and on destruction leaves command mode.
     * 
     */
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
    /**
     * @brief Construct a new HC12 module connection.
     * @details All the values here provided are the 'default' values. 
     * They will be overwritten the moment a 'UpdateParams' call is done
     * so the 'GetXXXX' calls return their true value.
     * 
     * @param serial The stream to use for communication with the module.
     * @param setPin The pin that is used to set the module in command mode (also know as SET or KEY pin).
     * @param baud The default baudrate that the module is currently set at.
     * @param mode The default operational mode the module is in.
     * @param channel The default channel for the module.
     * @param power The default transmit power of the module.
     */
    HC12(Stream &serial, unsigned int setPin, Baudrates baud = Baudrates::BPS_9600, PowerMode mode = PowerMode::FU2, unsigned int channel = 1, SendPower power = SendPower::mW_100_0);

    /**
     * @brief Setup and try to contact the HC12 module.
     * 
     * @return true if the module replied and is available.
     * @return false if the module never replied or only garbage was received.
     */
    bool begin();

    /**
     * @brief Configure the baudrate to be set on the next UpdateParams call.
     * 
     * @param baudrate The new baudrate to set then.
     */
    void PrepareBaudrate(Baudrates baudrate);
    
    /**
     * @brief Configure the new operation mode to be set on the next UpdateParams call.
     * 
     * @param mode The new operation mode to set then.
     */
    void PreparePowerMode(PowerMode mode);
    
    /**
     * @brief Configure the channel to be set on the next UpdateParams call.
     * 
     * @param channel The new channel to set then.
     */
    void PrepareChannel(int channel);
    
    /**
     * @brief Configure the transmit power to be set on the next UpdateParams call.
     * 
     * @param power The new transmit power to set then.
     */
    void PrepareSendPower(SendPower power);

    /**
     * @brief Update the module with new parameter settings and retrieve the ones that aren't updated.
     * 
     * @return true if the syncronization was a success.
     * @return false if the syncronization was a failure.
     */
    bool UpdateParams();

    /**
     * @brief Retrieve the currently set baudrate.
     * 
     * @return unsigned int The baudrate that the module currently uses.
     */
    unsigned int GetBaudrate();
    
    /**
     * @brief Retrieve the currently set operation mode.
     * 
     * @return PowerMode The operational mode that the module currently is in.
     */
    PowerMode GetPowerMode();

    /**
     * @brief Get the channel that the module is now working on.
     * 
     * @return unsigned int The channel for transmitting and receiving.
     */
    unsigned int GetChannel();
    
    /**
     * @brief Get the transmit power the module is configured to use.
     * 
     * @return SendPower The transmit power the module is using.
     */
    SendPower GetSendPower();

    /**
     * @brief Put the module into sleep until the next time the module enters the command mode (using like `UpdateParams()`).
     * 
     * @return true if the module was successfully put into sleep mode.
     * @return false if the module failed to go into sleep mode.
     */
    bool Sleep();

    /**
     * @brief Resets all parameters to their default values.
     * 
     * @return true if all the values were successfully reset.
     * @return false if the module and it's values failed to be reset.
     */
    bool Reset();

    virtual int available() override;
    virtual int read() override;
    virtual int peek() override;
    virtual size_t write(uint8_t data) override;

    /**
     * @brief Looks on each baudrate if the module replies to the status command.
     * 
     * @tparam T The serial to use (but MUST have `updateBaudRate(int baudrate)` and inherit from Stream).
     * @param ser The serial object to use.
     * @param cmdPin The pin that is used to set the module into command mode (also known as the SET or KEY pin)
     * @return unsigned int The baudrate the module is found at. 0 if the module could not be found.
     */
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

    /**
     * @brief Checks the given mode if it is a valid PowerMode.
     * 
     * @param mode The mode to check.
     * @return true If the mode is a valid PowerMode.
     * @return false If the mode is not a valid mode.
     */
    static constexpr bool IsPowerMode(PowerMode mode)
    {
        return (mode == PowerMode::FU1 ||
                mode == PowerMode::FU2 ||
                mode == PowerMode::FU3 ||
                mode == PowerMode::FU4);
    }

    /**
     * @brief Checks the given mode if it is a valid value for the transmit power.
     * 
     * @param power The power to check.
     * @return true If the power is a valid transmit power.
     * @return false If the power is not a valid power.
     */
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

    /**
     * @brief Checks the given mode if it is a valid baudrate.
     * 
     * @param baud The baudrate to check.
     * @return true If the baudrate is a valid value.
     * @return false If the baudrate is a not a supported baudrate.
     */
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
    static bool DbmToSendPower(int dbm, SendPower &power);

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
};

#endif // INCLUDE_ARDUINO_HC12_H
