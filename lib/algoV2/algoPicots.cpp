#include "algoPicots.h"
int read(File inFile) {
    int8_t highbyte;
    uint8_t lowbyte;
    inFile.readBytes((char *)&highbyte, sizeof(highbyte));
    inFile.readBytes((char *)&lowbyte, sizeof(lowbyte));
    return highbyte << 8 | lowbyte;
}
unsigned long uread(File inFile) {
    uint8_t highbyte;
    uint8_t lowbyte;
    inFile.readBytes((char *)&highbyte, sizeof(highbyte));
    inFile.readBytes((char *)&lowbyte, sizeof(lowbyte));
    return highbyte << 8 | lowbyte;
}
void skip(File inFile, int n) {
    uint8_t temp;
    for (int i = 0; i < n; i++) {
        inFile.readBytes((char *)&temp, sizeof(temp));
    }
}
unsigned long timestampRead(File inFile) {
    uint8_t t1;
    uint8_t t2;
    uint8_t t3;
    uint8_t t4;
    inFile.readBytes((char *)&t1, sizeof(t1));
    inFile.readBytes((char *)&t2, sizeof(t2));
    inFile.readBytes((char *)&t3, sizeof(t3));
    inFile.readBytes((char *)&t4, sizeof(t4));
    return t1 << 24 | t2 << 16 | t3 << 8 | t4;
}
bool algoPicots::runFromFile(float omega, int r, int R, String path) {
    if (omega == 0) { // get rotation speed from file
        omega = getW(path);
        Serial.println("rotation from file: " + String(omega));
    }
    File inFile = SD_MMC.open(path, FILE_READ);
    if (!inFile) {
        error = "cannot open file";
        return false;
    }
    int long t0 = millis();
    nmeas = inFile.size() / 2 / 308; // 4[time] + 3*2[acc] + 3*2[gyr] + 100*(4[time] + 2[sick]) = 616

    // inFile = SD_MMC.open(path, FILE_READ);
    float perf = 2 * M_PI * float(r) / float(R) / omega * 1000000 / float(nd); // picot period in float (us)
    fil = new filtreSick;
    fil->init(0.8, long(perf), 1200);
    if (fil->demiplage < 2) { // less than 2 samples in plage
        error = "period too short";
        return false;
    }
    if (fil->demiplage >= buffsize / 2 - 1) { // plage bigger than measure
        error = "period too long";
        return false;
    }
    fstack = new algoStack(fil->demiplage, 1200);
    usure = new usurePicots(fstack);
    comptage = new comptagePicots(fstack);
    patinage = new patinagePicots(fstack);
    float ax, ay, az, gx, gy, gz, v, anglef;
    unsigned long t;
    bool first = true;
    angle rot = angle(nullptr, "");
    for (int i = 0; i < nmeas; i++) {
        t = timestampRead(inFile);
        Serial.println(t);
        ax = float(read(inFile) / 100);
        ay = float(read(inFile) / 100);
        az = float(read(inFile) / 100);
        gx = float(read(inFile) / 100);
        gy = float(read(inFile) / 100);
        gz = float(read(inFile) / 100);
        if (first) {
            Serial.println("init");
            Serial.print("omega = ");
            Serial.println(omega);
            Serial.print("r = ");
            Serial.println(r);
            Serial.print("R = ");
            Serial.println(R);
            rot.initangle(ax, ay, az, gx, gy, gz, t);
            first = false;
        } else {
            anglef = rot.correctionangle(0.1, ax, ay, az, gx, gy, gz, t);
        }
        for (int j = 0; j < 100; j++) {
            t = timestampRead(inFile);
            v = read(inFile);
            anglef = rot.updateangle(t);
            int n = fil->newVal(v, t);
            for (int k = 1; k <= n; k++) {
                if (fil->nextVal(k)) {
                    // Serial.println(String(fil.tf / 1000)+","+String(fil.vf));
                    if (fstack->append(fil->vf, fil->tf)) {
                        usure->update();
                        patinage->update(anglef);
                        if (comptage->update()) {
                            // Serial.println(String(fil.tf / 1000)+","+String(fil.vf));
                        }
                    }
                }
            }
        }
    }
    patinage->compressprofil();
    return true;
}
float algoPicots::getW(String path) {
    File infile = SD_MMC.open(path, FILE_READ);
    if (!infile) {
        error = "cannot open file";
        return 0;
    }
    nmeas = infile.size() / 2 / 308;
    if (nmeas == 0) {
        return 0;
    }
    long Gx = 0;
    long Gy = 0;
    long Gz = 0;
    for (int i = 0; i < nmeas; i++) {
        skip(infile, 10);
        Gx += read(infile);
        Gy += read(infile);
        Gz += read(infile);
        skip(infile, 600);
    }
    infile.close();
    return sqrt(pow(float(Gx) / float(nmeas) / 100.0, 2) + pow(float(Gy) / float(nmeas) / 100.0, 2) + pow(float(Gz) / float(nmeas) / 100.0, 2));
}