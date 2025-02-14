#include "patinagePicots.h"

patinagePicots::patinagePicots(algoStack* s)
{
    stack = s;
}

void patinagePicots::update(float anglef)
{
    if (stack->Max-stack->Min > seuil1) {
        pattemp += stack->sampleperiod;
        pat1+= stack->sampleperiod;
    }
    nmoy+= stack->sampleperiod;
    //pattemp += pat[typepat] * dt;
    npattemp += stack->sampleperiod;
    if (int(anglef) != angleold) {
        if (angleold >= 0 && angleold < 360) {
            profil[angleold] = (float(pattemp) / float(npattemp)) > 0.5;
        }
        angleold = int(anglef);
        pattemp = 0;
        npattemp = 0;
    }
    pat1f = (float)pat1 / (float)(nmoy); // percent value of patc
}

void patinagePicots::compressprofil()
{
    for (int i = 0; i < 45; i++) {
        unsigned char  tempByte = 0;
        for (int j = 0; j < 8; j++) {
            tempByte = tempByte | (profil[8 * i + (j)] << (7 - j));
        }
        probfilb[i] = tempByte;
    }
}
