#pragma once
#include "utilsV2.h"
// #define nd 30 //nombre de dents 18"
#define nd 33 // nombre de dents 19"
#define md 20 // max dent perdues
class comptagePicots {
public:
    comptagePicots(algoStack *s);
    bool update();
    float seuilPics = 15;
    int mincor = 0;    // periode des pics
    float minsum = -1; // corelation
    int nPic = 0;      // nombre de pics
    unsigned int bestcorr[4 * nd];
    unsigned long tbestcorr[4 * nd];
    float vPic = 0;
    unsigned long tPic = 0;
    int vPicBuff[4 * nd];
    unsigned long tPicBuff[4 * nd];

private:
    void newpic();
    algoStack *stack = nullptr;
    float Maxtemp = 0;
    unsigned long tsMaxtemp;
    float Mintemp = 0;
    unsigned long tsMintemp;
    int indPic = 0;
    float sum[md];
    float sumPer[md];
    int countPer[md];
    bool updatePer(int i);
};
