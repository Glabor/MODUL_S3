#ifndef algo_H
#define algo_H
//#include <ArduinoJson.h>
#include "timedbuffer.h"
#include <SD_MMC.h>
#include "angle.h"
//#include <climits>
#define nd 30

class algo {
public:
    //algo(float r, float R);
    bool init(float omega, unsigned long t,int r, int R);
    bool newval(int V, unsigned long t, float angle);
    unsigned int bestcorr[4 * nd];
    unsigned long tbestcorr[4 * nd];
    int mincor = 0;//periode des pics
    byte probfilb[45];
    timedbuffer stack;
    String error="";
    int nPic = 0;//nombre de pics
    int indPic = 0;
    float minsum = -1;//corelation
    float sum[nd];
    int vPicBuff[4 * nd];
    unsigned long tPicBuff[4 * nd];
    bool runFromFile(float omega, int r, int R,String path);
    float usuremoyu = 0;        // average wear M
    int usureMu = INT_MIN;      // max wear M ever
    int usuremu = INT_MAX;      // min wear M ever
    float usuremoyd = 0;        // average wear m
    int usureMd = INT_MIN;      // max wear m ever
    int usuremd = INT_MAX;      // min wear m ever
    int deltaM;                 // max amplitude ever
    float pat1f; // percent value of pat1
private:
    void compressprofil();
    bool profil[360];
    float period;               // period on which to perform analysis
    int n0;                     // size of stack
    int m;                      // min value of stack
    int M;                      // max value of stack
    int delta;                  // amplitude of stack
    int seuil1 = 20;            // premier seuil
    //int seuil2 = 50;           // deuxieme seuil (plus grand)
    
    long nmoy = 0;              // number of values in average calculation
    unsigned long t0;       // first time of stack
    unsigned long told;
    int pat1 = 0;
    bool pat = false;
    int r_molette;
    int R_RDC;
    void newpic(float vPic, unsigned long tPic);

    //int vPicMax[4 * nd];
    //unsigned long tPicMax[4 * nd];  
    unsigned long pattemp = 0;//temps d'adherence sur un degre
    float angleold = -1;//angle profil precedent
    unsigned long npattemp = 0;//temps total sur un degre
};
#endif

