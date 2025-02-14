#include "comptagePicots.h"
#include <math.h>
#include<cmath>
#include <utility>
comptagePicots::comptagePicots(algoStack* s)
{
    stack = s;
    for (int i = 0; i < md; i++) {
        sum[i] = 0;
        sumPer[i] = 0;
        countPer[i] = 0;
    }
}

bool comptagePicots::update()
{
    bool np = false;
    int i = stack->debut;
    bool picMax = true;
    bool picMin = true;
    if (Maxtemp < stack->vstack[stack->milieu] || Maxtemp == 0) {
        Maxtemp = stack->vstack[stack->milieu];
        tsMaxtemp = stack->tstack[stack->milieu];
    }
    if (Mintemp > stack->vstack[stack->milieu] || Mintemp == 0) {
        Mintemp = stack->vstack[stack->milieu];
        tsMintemp = stack->tstack[stack->milieu];
    }
    if (Maxtemp - vPic > seuilPics && Maxtemp - stack->vstack[stack->milieu] > seuilPics) {
        vPic = Maxtemp;
        tPic = tsMaxtemp;
        Maxtemp = vPic;
        Mintemp = vPic;
        newpic();
        return true;
    }
    if (Mintemp - vPic < -seuilPics && Mintemp - stack->vstack[stack->milieu] < -seuilPics) {
        vPic = Mintemp;
        tPic = tsMintemp;
        Maxtemp = vPic;
        Mintemp = vPic;
        newpic();
        return true;
    }
    if (stack->picMid) {
        if (stack->Max - stack->Min < seuilPics) {
            return false;
        }
        else {
            vPic = stack->vstack[stack->milieu];
            tPic = stack->tstack[stack->milieu];
            Maxtemp = vPic;
            Mintemp = vPic;
            newpic();
            return true;
        }
    }
    return false;
}

void comptagePicots::newpic()
{
    int vPicOld = vPicBuff[indPic];
    vPicBuff[indPic] = vPic;
    tPicBuff[indPic] = tPic;
    nPic++;
    int ncorr;
    for (int i = 0; i < md; i++) {
        ncorr = (2 * (nd - i));
        if (updatePer(i)) {
            sum[i] = sumPer[i];
        }
        else {
            if (nPic > 2 * ncorr) {
                //float sumold = sum[i];
                sum[i] += pow(vPic - float(vPicBuff[(indPic + 4 * nd - ncorr) % (4 * nd)]), 2);
                if (i > 0) {
                    sum[i] -= pow(float(vPicBuff[(indPic + 4 * nd - ncorr) % (4 * nd)]) - float(vPicBuff[(indPic + 4 * nd - 2 * ncorr) % (4 * nd)]), 2);
                }
                else {
                    sum[i] -= pow(float(vPicBuff[(indPic + 4 * nd - ncorr) % (4 * nd)]) - vPicOld, 2);
                }
            }
        }
        if (nPic > 2 * ncorr){
            if (minsum<0 || minsum>sqrt(sum[i] / float(2 * ncorr))) {
                minsum = sqrt(sum[i] / float(2 * ncorr));
                mincor = ncorr;
                for (int j = 0; j < ncorr * 2; j++) {
                    bestcorr[j] = vPicBuff[(indPic + 1 + 4 * nd - 2 * ncorr + j) % (4 * nd)];
                    tbestcorr[j] = tPicBuff[(indPic + 1 + 4 * nd - 2 * ncorr + j) % (4 * nd)];
                }
            }
        }
    }
    indPic = (indPic + 1) % (4 * nd);//prochain
}

bool comptagePicots::updatePer(int i)
{
    int ncorr;
    if (countPer[i] == 0) {
        sumPer[i] = 0;
    }
    ncorr = 2 * (nd - i);
    sumPer[i] += pow(vPic - float(vPicBuff[(indPic + 4 * nd - ncorr) % (4 * nd)]), 2);
    countPer[i]++;
    /*if (count <= 2 * demiplage + 1) {
        std::cout << "cpstack" << CPstack[0] << "\n";
    }*/
    if (countPer[i] == 2*ncorr) {
        countPer[i] = 0;
        return true;
    }
    return false;
}
