#include "hw.h"

hw::hw(pinout *p) {
    pins = p;
    adc1 = pins->HMCX_CS;
    adc2 = pins->HMCY_CS;
    adc3 = pins->HMCZ_CS;
    set = pins->SR;
}

void hw::Write(byte thisRegister, byte thisValue) {
    /* SPI write function */
    digitalWrite(adc1, LOW);
    digitalWrite(adc2, LOW);
    digitalWrite(adc3, LOW);
    SPI.transfer(thisRegister);
    SPI.transfer(thisValue);
    digitalWrite(adc1, HIGH);
    digitalWrite(adc2, HIGH);
    digitalWrite(adc3, HIGH);
}

void hw::ADCsetup() {
    pins->all_CS_high();
    Write(0x00, 0x00); // PD - register normal mode
    Write(0x03, 0x00); // CAL_START - PGA gain calibration
    Write(0x08, 0x38); // FILTER - set to SINC4 highest data rate SINGLE CONV 240Hz
    // Write(0x08, 0x36); // FILTER - set to SINC4 highest data rate CONT CONV 240Hz
    Write(0x09, 0x46); // CTRL
    Write(0x0A, 0x00); // SOURCE
    Write(0x0B, 0x23); // MUX_CTRL0
    Write(0x0C, 0xFF); // MUX_CTRL1
    Write(0x0D, 0x00); // MUX_CTRL2
    Write(0x0E, 0x20); // PGA
    // Write(0x01, 0x71); // writes to CONV_START - result stored in DATA7
    // Serial.println("setup of ADCs complete");
}

unsigned char hw::CheckStatus_ADC(byte thisRegister, int adc) {
    /* checks status of ADC 1*/
    // SRI why global variables here since they are not used anywhere else ?
    digitalWrite(adc, LOW); // reading
    SPI.transfer(thisRegister | READ);
    long ADC_status16 = SPI.transfer(0x00);
    long ADC_status8 = SPI.transfer(0x00);
    long ADC_status0 = SPI.transfer(0x00);
    long ADC_status24 = (ADC_status16 << 16) | (ADC_status8 << 8) | ADC_status0;
    digitalWrite(adc, HIGH);
    ADC_status0 = ADC_status0 >> 4;
    return (ADC_status0);
}

void hw::ReadResult_ADC(byte thisRegister, int adc) {
    long ADC_read1, ADC_read2;
    long ADC_result;
    /* reads result of ADC */
    digitalWrite(adc, LOW); // reading
    SPI.transfer(thisRegister | READ);
    long ADC_hi = SPI.transfer(0x00);
    long ADC_mid = SPI.transfer(0x00);
    long ADC_lo = SPI.transfer(0x00);
    digitalWrite(adc, HIGH);

    ADC_result = (ADC_hi << 16) | (ADC_mid << 8) | ADC_lo; // read 2 after reset pulse
    dataMessage += String(ADC_result) + ",";

    for (size_t i = 0; i < 3; i++) {
        dataBuffer[r] = lowByte(ADC_result >> 8 * (2 - i));
        r++;
    }

    if (adc == adc1) {
        sum_x = sum_x + ADC_result;
        count_x++;
        if (ADC_result > max_x) {
            max_x = ADC_result;
        }
        if (ADC_result < min_x) {
            min_x = ADC_result;
        }
    } else if (adc == adc2) {
        sum_y = sum_y + ADC_result;
        count_y++;
        if (ADC_result > max_y) {
            max_y = ADC_result;
        }
        if (ADC_result < min_y) {
            min_y = ADC_result;
        }
    } else if (adc == adc3) {
        Serial.println(dataMessage);
        // if (file) {
        //     // Serial.println("file is open to print");
        //     file.write(dataBuffer, r);
        //     r = 0;
        // }
        // file.write('\n');
        // file.write('\r');
        // file.write('\t');

        dataMessage = "";

        sum_z = sum_z + ADC_result;
        count_z++;
        if (ADC_result > max_z) {
            max_z = ADC_result;
        }
        if (ADC_result < min_z) {
            min_z = ADC_result;
        }
    }
}

byte *hw::ADC_conv() {
    /* during conversion, gets the accelero data */
    /* checks for drdy pin and reads the result */
    drdy1 = CheckStatus_ADC(0x38, adc1); // function to check status register
    drdy2 = CheckStatus_ADC(0x38, adc2); // function to check status register
    drdy3 = CheckStatus_ADC(0x38, adc3); // function to check status register

    int time3 = millis();
    bool timeOut = false;

    while ((drdy1 != 1 || drdy2 != 1 || drdy3 != 1) & (millis() - time3 < 20)) {
        drdy1 = CheckStatus_ADC(0x38, adc1); // function to check status register
        drdy2 = CheckStatus_ADC(0x38, adc2); // function to check status register
        drdy3 = CheckStatus_ADC(0x38, adc3); // function to check status register
    }
    // Serial.printf("%d,%d,%d\n", drdy1, drdy2, drdy3);

    ReadResult_ADC(result, adc1);
    ReadResult_ADC(result, adc2);
    ReadResult_ADC(result, adc3);
    return dataBuffer;
}

void hw::SR_pwm() {
    /* set and reset pulses */
    /* each pulse calls for a conversion by the ADC */
    digitalWrite(set, HIGH);
    delayMicroseconds(50);
    s = false;         // if i = false , then ADC conversion during SET
    Write(0x01, 0x70); // writes to CONV_START - result stored in DATA7

    if (file) {
        // Serial.println("file is open to print");
        file.write(dataBuffer, r);
        r = 0;
    }

    // dataMessage += String(tMeas) + ",";
    for (size_t i = 0; i < 4; i++) { // time in dataBuffer
        dataBuffer[r] = lowByte(tMeas >> 8 * (3 - i));
        r++;
    }

    ADC_conv();
    digitalWrite(set, LOW);
    delayMicroseconds(50);
    s = true;          // if i = true , then ADC conversion during RESET
    Write(0x01, 0x70); // writes to CONV_START - result stored in DATA7
    ADC_conv();
}

bool hw::SR_pwm(File inFile,bool flush) {
    bool res=r>0;
    uint32_t time;
    digitalWrite(set, HIGH);
    delayMicroseconds(50);
    s = false;         
    Write(0x01, 0x70); 
    if (inFile) {
        inFile.write(dataBuffer, r);
        r = 0;
    }
    r=4;
    ADC_conv();
    time=micros();//time_set
    for (size_t i = 0; i < 4; i++) { // time in dataBuffer
        dataBuffer[i] = lowByte(time >> 8 * (3 - i));
    }
    digitalWrite(set, LOW);
    delayMicroseconds(50);
    s = true;          // if i = true , then ADC conversion during RESET
    Write(0x01, 0x70); // writes to CONV_START - result stored in DATA7
    r=17;
    ADC_conv();
    time=micros();//time_reset
    for (size_t i = 0; i < 4; i++) { // time in dataBuffer
        dataBuffer[i+13] = lowByte(time >> 8 * (3 - i));
    }
    //Serial.print(r);
    if (flush){
        if (inFile) {
            inFile.write(dataBuffer, r);
            r = 0;
        }
    }
    return res;
}

void hw::measureHMR(int measTime, String sensName) {
    ADCsetup();
    file = SD_MMC.open(sensName, FILE_WRITE);
    if (file) {
        long m0 = millis();
        while (millis() < m0 + measTime * 1000) {
            tMeas = millis();
            SR_pwm();
        }
    }
    file.close();
}