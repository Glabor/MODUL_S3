#pragma once
#ifndef binData_H
#define binData_H
#include <vector>
#include <arduino.h>
#include "FS.h"
#include <RTClib.h>

enum dataType { UNSIGNED_BYTE, UNSIGNED_2BYTES_B, UNSIGNED_3BYTES_B, UNSIGNED_4BYTES_B, UNSIGNED_2BYTES_L, UNSIGNED_3BYTES_L, UNSIGNED_4BYTES_L, SIGNED_BYTE, SIGNED_2BYTES_B, SIGNED_3BYTES_B, SIGNED_4BYTES_B, SIGNED_2BYTES_L, SIGNED_3BYTES_L, SIGNED_4BYTES_L, FLOAT, STRING,DATETIME };
//static byte writeBuff[4]; 
union num {
    float f;
    int32_t i;
    uint32_t ui;
    char bytes[4];
};
static int getL(dataType code) {
    if (code == UNSIGNED_BYTE || code == SIGNED_BYTE) { return 1; }
    if (code == UNSIGNED_2BYTES_B || code == SIGNED_2BYTES_B || code == UNSIGNED_2BYTES_L || code == SIGNED_2BYTES_L) { return 2; }
    if (code == UNSIGNED_3BYTES_B || code == SIGNED_3BYTES_B || code == UNSIGNED_3BYTES_L || code == SIGNED_3BYTES_L) { return 3; }
    return 4;
}
static bool isByteorderBig(dataType code){
    if (code == UNSIGNED_2BYTES_B || code == SIGNED_2BYTES_B || code == UNSIGNED_3BYTES_B || code == SIGNED_3BYTES_B) { return true; }
    if (code == UNSIGNED_4BYTES_B || code == SIGNED_4BYTES_B) { return true; }
    return false;
}
static bool isSigned(dataType code) {
    if (code == SIGNED_BYTE || code == SIGNED_2BYTES_B || code == SIGNED_3BYTES_B || code == SIGNED_4BYTES_B) { return true; }
    if (code == SIGNED_BYTE || code == SIGNED_2BYTES_L || code == SIGNED_3BYTES_L || code == SIGNED_4BYTES_L) { return true; }
    return false;
}
static num numRead(dataType code,File inFile);
static void numWrite(num n,dataType code,File outFile);
class metaData{
    public:
    metaData(File inFile);
    metaData(String k,String ss);
    metaData(String k,DateTime  d);
    metaData(String k,float f);
    metaData(String k,int32_t i);
    metaData(String k,uint32_t ui);
    void print();
    void write(File outFile);
    private:
    int l=0;
    String key;
    dataType code;
    String s;
    num n;
};
static String strRead(File inFile);
static void strWrite(String s,File outFile);
class field {
public:
    field() {};
    field(String nm,String u, dataType code, float m);
    field(File inFile);
    int length=0;
    dataType typeCode;
    String unit;
    String name;
    float multiplier = 1;//positif multiplier, negatif diviseur
    void write(File outFile);
    void valueWrite(File outFile);
    uint8_t nameLength;
    uint8_t unitLength;
    void print() {
        Serial.println("name: " + name + ", unit: " + unit + ", multiplier: " + String(multiplier)+ ", size: "+ String(getL(typeCode))+ "bytes");
    };
    float* pf=nullptr;
    int32_t* pi=nullptr;
    uint32_t* pu=nullptr;
};
class measurement {
public:
    measurement() {};
    measurement(File inFile);
    void addField(field f) {
        Fields.push_back(f);
        nCol++;
        length += f.length;
    };
    void addField(String name, String unit, dataType code, int multiplier) {
        field fl(name, unit, code, multiplier);
        addField(fl);
    };
    uint8_t nCol = 0;
    uint16_t nRow=0;
    int length = 3;//nCol+nRow
    std::vector<field> Fields;
    void write(File outFile) {
        byte buff[1];
        buff[0]=nCol;
        outFile.write(buff[0]);
        buff[0]=lowByte(nRow);
        outFile.write(buff[0]);
        buff[0]=lowByte(nRow>>8);
        outFile.write(buff[0]);
        for (int i = 0; i < nCol; i++) {
            Fields[i].write(outFile);
        }
    };
    void print(int num) {
        Serial.println("measurement " + String(num) + ", " + String(nRow) + " records per cycle, " + String(nCol) + " fields:");
        for (int i = 0; i < nCol; i++) {
            Fields[i].print();
        }
    }
};
#endif