#include "filtreN.h"

#include <utility>
#include <iostream>
void filtreSick::init(float ratioplage, long per,int sampleper) {
    float plage = (float)per / (float)sampleper * ratioplage;
    rp = ratioplage;
    int iplage = (int)std::floor(plage);
    if ( iplage % 2 == 1) {
        demiplage = (iplage - 1) / 2;
    }
    else {
        demiplage = iplage / 2;
    }
    period0 = per;
    sampleperiod = sampleper;
    float A[25];
    for (int i = 0; i < ncor; i++) {
        for (int j = 0; j < ncor; j++) {
            A[ncor * i + j] = 0;
            AM[ncor * i + j] = 0;
        }
    }
    for (int k = -demiplage; k <= demiplage; k++) {
        for (int i = 0; i < ncor; i++) {
            for (int j = 0; j < ncor; j++) {
                A[ncor * i + j] = A[ncor * i + j] + convk(k, demiplage, i) * convk(k, demiplage, j);
            }
        }
    }
    choleskyN((float*)&A, (float*)&AM, 5);
}
float filtreSick::convk(int k, int p, int i) {
    if (i == 0) {
        return 1.0;
    }
    if (i == 1) {
        return float(k) / float(p);
    }
    if (i == 2) {
        return float(k) * float(k) / float(p) / float(p);
    }
    if (i == 3) {
        return cos(rp*float(k) / float(p) * M_PI) - 1.0;
    }
    if (i == 4) {
        return sin(rp*float(k) / float(p) * M_PI);
    }
    return 0;
}
int filtreSick::newVal(int V, unsigned long t,bool set) {
    if ((tsetold==0&&!set)||(tresetold==0&&set)) {
        vold = (float)V;
        tsold=t;
        told=t;
        if(set){
            vsetold=(float)V;
            tsetold=t;
        }
        else{
            vresetold=(float)V;
            tresetold=t;
        }
        count ++;
        return 0;
    }
    if(set){
        vnew = V-vresetold;
        tnew = (t+tresetold)/2000000;
    }
    else{
        vnew = vsetold-V;
        tnew = (t+tsetold)/2000000;
    }
    Nnew = (t - tsold) / sampleperiod;
    return Nnew;
}
bool filtreSick::nextVal(int i){
    unsigned long tsnew = tsold + (unsigned long)i * (unsigned long)sampleperiod;
    tf = tsnew- (unsigned long)demiplage* (unsigned long)sampleperiod;
    float vsnew = vold+float(vnew-vold)*float(tsnew-told)/float(tnew-told);
    bool b = updateCstack(vsnew);
    if (b) {
        vf = AM[0] * Cstack[0] + AM[1] * Cstack[1] + AM[2] * Cstack[2] + AM[3] * Cstack[3] + AM[4] * Cstack[4];
        float c = AM[15] * Cstack[0] + AM[16] * Cstack[1] + AM[17] * Cstack[2] + AM[18] * Cstack[3] + AM[19] * Cstack[4];
        float s = AM[20] * Cstack[0] + AM[21] * Cstack[1] + AM[22] * Cstack[2] + AM[23] * Cstack[3] + AM[24] * Cstack[4];
        nc=c*c+s*s;
    }
    //vf = vsnew;
    if (i == Nnew) {
        vold = vnew;
        told = tnew;
        tsold = tsnew;
    }
    return b;
}
bool filtreSick::updateCstack(float vn) {
    bool b = updatePstack(vn);
    count++;
    if (count <= 2 * demiplage + 1) {
        fin ++;
        Vstack[fin] = vn;
        if (b) {
            for (int i = 0; i < ncor; i++) {
                Cstack[i] = CPstack[i];
            }
            return true;
        }
        return false;
    }
    if (b) {
        for (int i = 0; i < ncor; i++) {
            Cstack[i] = CPstack[i];
        }
    }
    else {
        float vnold = Vstack[debut];
        float dp = float(demiplage);
        float c0old = Cstack[0];
        Cstack[0] += (vn - vnold);
        Cstack[1] += vn / dp * (dp + 1.0) + vnold - Cstack[0] / dp;
        Cstack[2] += vn / dp / dp * (dp + 1.0) * (dp + 1.0) - vnold - 2 * Cstack[1] / dp -  Cstack[0] / dp / dp;
        float cc = cos(rp * M_PI / dp) * (Cstack[3] + c0old + vn * cos(rp * M_PI * (dp + 1.0) / dp) - vnold * cos(-rp * M_PI)) + sin(rp * M_PI / dp) * (Cstack[4] + vn * sin(rp * M_PI * (dp + 1.0) / dp) - vnold * sin(-rp * M_PI));
        float ss = cos(rp * M_PI / dp) * (Cstack[4] + vn * sin(rp * M_PI * (dp + 1.0) / dp) - vnold * sin(-rp * M_PI)) - sin(rp * M_PI / dp) * (Cstack[3] + c0old + vn * cos(rp * M_PI * (dp + 1.0) / dp) - vnold * cos(-rp * M_PI));
        Cstack[3] = cc - Cstack[0];
        Cstack[4] = ss;
    }
    debut = (debut + 1) % buffsize;
    fin = (fin + 1) % buffsize;
    Vstack[fin] = vn;
    return true;
}
bool filtreSick::updatePstack(float vn) {
    if (countper == 0) {
        for (int i = 0; i < ncor; i++) {
            CPstack[i] = 0;
        }
    }
    for (int i = 0; i < ncor; i++) {
        CPstack[i] += convk(countper - demiplage, demiplage, i) * vn;
    }
    countper++;
    if (countper==2*demiplage+1) {
        countper = 0;
        return true;
    }
    return false;
}
