#ifndef timedbuffer_H
#define timedbuffer_H
#include <Arduino.h>
//#include <math.h>
//#include<cmath>
#define buffsize 100 //0.3tour secondes
#define M_PI 3.14159265358979323846

class timedbuffer {
public:
    void init(float plage, float p);
    int Max = 0;
    int Min = 0;
    void append(int V, unsigned long t);
    void appendFilt(int V, unsigned long t, unsigned long period);
    void clear(unsigned long tmin, int countmax);
    bool update();
    bool getVF(float* V, unsigned long* t);
    float seuilPics = 15;
    float vPic = 0;
    unsigned long tPic;
private:
    float convk(int k, int p, int i);
    int count = 0;
    int countFilt = 0;
    int countF = 0;
    void decaleFilt(float V);
    int debutFilt = 0;
    int ncor = 5;
    float AM[25];
    //float A[25];
    float period0;
    int demiplage;
    int debut = 0;
    int fin = buffsize - 1;
    int debutF = 0;
    int milieuF = 0;
    int finF = buffsize - 1;
    int vstack[buffsize];           // value stack
    unsigned long tstack[buffsize]; // time stack
    float vFstack[buffsize];           // filtered value stack
    unsigned long tFstack[buffsize]; // filtered time stack
    float Cstack[5] = { 0,0,0,0,0 }; // conv stack
    float CPstack[5] = { 0,0,0,0,0 }; // conv stack periodique
    int CPind = 0;
    float Maxtemp=0;
    unsigned long tsMaxtemp;
    float Mintemp=0;
    unsigned long tsMintemp;
    //int MaxF=0;
    //int MinF=0;
};
#endif

