#include "timedbuffer.h"
#include "utils.h"
void timedbuffer::init(float plage, float p) {
    demiplage = floor((plage - 1) / 2);
    period0 = p;
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
float timedbuffer::convk(int k, int p, int i) {
    if (i == 0) {
        return 1 ;
    }
    if (i == 1) {
        return float(k) / float(p);
    }
    if (i == 2) {
        return float(k) * float(k) / float(p) / float(p);
    }
    if (i == 3) {
        return cos(float(k) / float(p) * M_PI) - 1.0;
    }
    if (i == 4) {
        return sin(float(k) / float(p) * M_PI);
    }
}
void timedbuffer::decaleFilt(float V) {
    float vF = 0;
    if (countFilt < 2 * demiplage + 1) {
        for (int i = 0; i < 5; i++) {
            Cstack[i] += convk(countFilt - demiplage, demiplage, i)*V;
        }
        countFilt++;
    }
    else {
        
        float Vold = vstack[debutFilt];
        float dp = float(demiplage);
        float c0old = Cstack[0];
        Cstack[0] += (V - Vold);
        Cstack[1] += V / dp * (dp + 1.0) + Vold-Cstack[0] / dp;
        Cstack[2] += V / dp / dp * (dp + 1.0) * (dp + 1.0) - Vold - 2 * Cstack[1] / dp - Cstack[0] / dp / dp;
        float cc = cos(M_PI / dp) * (Cstack[3] + c0old - V * cos(M_PI / dp) + Vold) + sin(M_PI / dp) * (Cstack[4] - sin(M_PI / dp) * V);
        float ss = cos(M_PI / dp) * (Cstack[4] - V * sin(M_PI / dp)) - sin(M_PI / dp) * (Cstack[3] + c0old - cos(M_PI / dp) * V + Vold);
        Cstack[3] = cc - Cstack[0];
        Cstack[4] = ss;
        for (int i = 0; i < 5; i++) {
            CPstack[i] += convk(CPind - demiplage, demiplage, i) * V;
        }
        CPind++;
        if (CPind == 2 * demiplage+1) {
            CPind = 0;
            for (int i = 0; i < 5; i++) {
                Cstack[i] = CPstack[i];
                CPstack[i]=0;
            }
        }
        
        vF = AM[0] * Cstack[0] + AM[1] * Cstack[1] + AM[2] * Cstack[2] + AM[3] * Cstack[3] + AM[4] * Cstack[4];
        finF = (finF + 1) % buffsize;
        if (countF < 2 * demiplage + 1) {
            countF++;
            milieuF = std::min(demiplage, countF);
        }
        else {
            debutF = (debutF + 1) % buffsize;
            milieuF = (milieuF + 1) % buffsize;
        }
        //vFstack[finF] = Cstack[3] /(2*dp+1);
        debutFilt = (debutFilt + 1) % buffsize;
        vFstack[finF] = vF;
        tFstack[finF] = tstack[(debutFilt + demiplage) % buffsize];
        //unsigned long ttt= tstack[(debutFilt + demiplage) % buffsize];
        //ttt = 0;
    }
}
void timedbuffer::appendFilt(int V, unsigned long t, unsigned long tmin) {
    if (count >= 2 * demiplage+1) {
        clear(tmin, 2 * demiplage);
        unsigned long Tf = tstack[fin];
        int N = 2 * demiplage + 1 - count;
        int Vf = vstack[fin];
        for (int i = 1; i <= N; i++) {
            float vflot = float(Vf) + float(i) / float(N) * (float(V) - float(Vf));
            double tflot = double(Tf) + double(i) / double(N) * double(t - Tf);
            append((int)vflot, (unsigned long) tflot);
            //append(1000, Tf + i / N * (t - Tf));
        }
    }
    else {
        append(V, t);
        //vstack[0] = 1000;
        /*fin = (fin + 1) % buffsize;
        vstack[fin] = V;
        tstack[fin] = t;
        count++;*/
    }
}
void timedbuffer::append(int V, unsigned long t) {
    if (count < buffsize) {
        count++;
        fin = (fin + 1) % buffsize;
        vstack[fin] = V;
        tstack[fin] = t;
        decaleFilt(float(V));
    }
}
void timedbuffer::clear(unsigned long tmin,int countmax) {
    while (count>countmax||(count > 0 && tstack[debut] < tmin)) {
        count--;
        debut = (debut + 1) % buffsize;
    }
}
bool timedbuffer::update() {
    int i = debut;
    Min = vstack[debut];
    Max = Min;
    while (i != fin) {
        if (vstack[i] < Min) {
            Min = vstack[i];
        }
        if (vstack[i] > Max) {
            Max = vstack[i];
        }
        i = (i + 1) % buffsize;
    }
    i = debutF;
    bool picMax = true;
    bool picMin = true;
    if (countF >= 2 * demiplage + 1) {
        if (Maxtemp < vFstack[milieuF] || Maxtemp == 0) {
            Maxtemp = vFstack[milieuF];
            tsMaxtemp = tFstack[milieuF];
        }
        if (Mintemp > vFstack[milieuF] || Mintemp == 0) {
            Mintemp = vFstack[milieuF];
            tsMintemp = tFstack[milieuF];
        }
        float MaxF = vFstack[i];
        float MinF = vFstack[i];
        while (i != finF) {
            MaxF = std::max(MaxF, vFstack[i]);
            MinF = std::min(MinF, vFstack[i]);
            if (vFstack[i] > vFstack[milieuF]) {
                picMax = false;
            }
            if (vFstack[i] < vFstack[milieuF]) {
                picMin = false;
            }
            i = (i + 1) % buffsize;
        }
        if (Maxtemp - vPic > seuilPics && Maxtemp - vFstack[milieuF] > seuilPics) {
            vPic = Maxtemp;
            tPic = tsMaxtemp;
            Maxtemp = vPic;
            Mintemp = vPic;
            return true;
        }
        if (Mintemp - vPic < -seuilPics && Mintemp - vFstack[milieuF] < -seuilPics) {
            vPic = Mintemp;
            tPic = tsMintemp;
            Maxtemp = vPic;
            Mintemp = vPic;
            return true;
        }
        if (picMax || picMin) {
            if (vPic >0&& MaxF - MinF < seuilPics) {
                return false;
            }
            vPic = vFstack[milieuF];
            tPic = tFstack[milieuF];
            Maxtemp = vPic;
            Mintemp = vPic;
            return true;
        }
    }
    return false;
}
bool timedbuffer::getVF(float* V, unsigned long* t) {
    
    if (countF >0) {
        *V = vFstack[finF];
        *t = tFstack[finF];
        return true;
    }
    return false;
}

