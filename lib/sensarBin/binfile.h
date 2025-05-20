#pragma once
#ifndef binFile_H
#define binFile_H
#include "binData.h"
#include "SD_MMC.h"
//using metaType = std::variant<int, float, long, unsigned int, unsigned long, std::string>;

class binHeader {
public:
    void addMeasurement(measurement m) {
        measData.push_back(m);
        nbMeas++;
        headerLength += m.length;
        if (m.nRow == 0) { async = true; }
    };
    void addMetaData(String key, String s) {
        nbMeta++;
        metaDatas.push_back(metaData(key,s));
        headerLength += key.length()+s.length()+2;
    };
    void addMetaData(String key, float f) {
        nbMeta++;
        metaDatas.push_back(metaData(key,f));
        headerLength += key.length() + 6;
    };
    void addMetaData(String key, int32_t f) {
        nbMeta++;
        metaDatas.push_back(metaData(key,f));
        headerLength += key.length() + 6;
    };
    void addMetaData(String key, uint32_t f) {
        nbMeta++;
        metaDatas.push_back(metaData(key,f));
        headerLength += key.length() + 6;
    };
    void addMetaData(String key, DateTime d) {
        //Serial.print("date metadata");
        nbMeta++;
        metaDatas.push_back(metaData(key,d));
        headerLength += key.length()+6;
    };
    void read(File inFile) {
        inFile.readBytes((char*)&version, 1);
        inFile.readBytes((char*)&headerLength, 2);
        inFile.readBytes((char*)&nbMeta, 1);
        for (int i = 0; i < nbMeta; i++) {
            metaDatas.push_back(metaData(inFile));
        }
        inFile.readBytes((char*)&nbMeas, 1);
        for (int i = 0; i < nbMeas; i++) {
            measData.push_back(measurement(inFile));
            if (measData[i].nRow == 0) { async = true; }
        }
    };
    void write(File outFile) {
        byte buff[1];
        buff[0]=version;
        outFile.write(buff[0]);
        buff[0]=lowByte(headerLength);
        outFile.write(buff[0]);
        buff[0]=highByte(headerLength);
        outFile.write(buff[0]);
        buff[0]=nbMeta;
        outFile.write(buff[0]);
        for(unsigned int i = 0; i < metaDatas.size(); ++i) {
            metaDatas[i].write(outFile);
        }
        buff[0]=nbMeas;
        outFile.write(buff[0]);
        for (int i = 0; i < nbMeas; i++) {
            measData[i].write(outFile);
        }
    };
    std::vector<metaData> metaDatas;
    std::vector<measurement> measData;
    uint16_t headerLength =5 ;//version+length+nbMeta+nmMeas
    uint8_t nbMeas = 0;
    uint8_t nbMeta = 0;
    uint8_t version = 1;
    bool async=false;
    void print() {
        Serial.println("binary file header");
        Serial.println("header length: " + String(headerLength) + " bytes");
        Serial.println(String(nbMeta)+" METADATA:");
        delay(100);
        for (int i = 0; i < nbMeta; i++) {
            metaDatas[i].print();
        }
        Serial.println(String(nbMeta)+" MEASUREMENTS:");
        for (int i = 0; i < nbMeas; i++) {
            measData[i].print(i);
        }
    }
};
class binFile {
public:
    binFile(String path) {
        name = path;
    }
    binHeader header;
    void bind(String key,float* value){
        int found=0;
        for (int i = 0; i < header.nbMeas; i++) {
            for (int j = 0; j < header.measData[i].nCol; j++) {
                if (key == header.measData[i].Fields[j].name) {
                    header.measData[i].Fields[j].pf=value;
                    found++;
                }
            }
        }   
        Serial.println(key+": "+String(found)+"fields found");
    }
    void bind(String key,int32_t* value){
        int found=0;
        for (int i = 0; i < header.nbMeas; i++) {
            for (int j = 0; j < header.measData[i].nCol; j++) {
                if (key == header.measData[i].Fields[j].name) {
                    header.measData[i].Fields[j].pi=value;
                    found++;
                }
            }
        }  
        Serial.println(key+": "+String(found)+"fields found");
    }
    void bind(String key,uint32_t* value){
        int found=0;
        for (int i = 0; i < header.nbMeas; i++) {
            for (int j = 0; j < header.measData[i].nCol; j++) {
                if (key == header.measData[i].Fields[j].name) {
                    header.measData[i].Fields[j].pu=value;
                    found++;
                }
            }
        }  
        Serial.println(key+": "+String(found)+"fields found");
    }
    void readHeader() {
        inFile = SD_MMC.open(name, FILE_READ);
        header.read(inFile);
    };
    void writeHeader() {
        outFile = SD_MMC.open(name, FILE_WRITE);
        header.write(outFile);
    };
    void writeMeasurement(uint8_t indM) {
        byte buff[1];
        if (header.async) {
            buff[0]=indM;
            outFile.write(buff[0]);
        }
        for (int i = 0; i < header.measData[indM].nCol; i++) {
            header.measData[indM].Fields[i].valueWrite(outFile);
        }
    };
    void writeMeasurement() {
        if (!header.async) {
            //Serial.println("write measurement "+String(indMeas));
            writeMeasurement(indMeas);
            updateInd();
        }
    };

    /*void readMeasurement(int indM) {
        for (int i = 0; i < header.measData[indM].nCol;i++) {
            String key = header.measData[indM].Fields[i].name;
            dataType code = header.measData[indM].Fields[i].typeCode;
            if (values.count(key) > 0) {
                valueRead(values[key], code, inFile);
            }
            else {
                long fill = 0;
                outFile.write((char*)&fill, getL(code));//remplissage
            }
        }
    };
    uint8_t readMeasurement() {
        uint8_t indM = 0;
        if (header.async) {
            inFile.read((char*)&indM, 1);
        }
        else {
            indM = indMeas;
            updateInd();
        }
        readMeasurement(indM);
        return indM;
    };*/
    void close() {
        outFile.flush();
        outFile.close();
    }
    void updateInd() {
        indRow++;
        if (indRow >= header.measData[indMeas].nRow) {
            indMeas = (indMeas + 1) % header.nbMeas;
        }
    };
    File inFile;
    File outFile;
    int nbMeas = 0;
    String name;

private:
    int indMeas = 0;;
    int indCol = 0;
    int indRow = 0;
};
#endif


