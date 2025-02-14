#include"utilsV2.h"
#include <utility>
void invTrinangleInf(float* L, float* LM, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            LM[n * i + j] = 0;
        }
        LM[(n + 1) * i] = 1 / L[(n + 1) * i];
        for (int k = 0; k < i; k++) {
            for (int j = 0; j <= k; j++) {
                LM[n * i + j] = LM[n * i + j] - L[n * i + k] * LM[n * k + j];
            }
        }
    }
}
void invD(float* D, int n) {
    for (int i = 0; i < n; i++) {
        D[i] = 1 / D[i];
    }
}
void transpose(float* L, float* LP, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            LP[n * i + j] = L[n * j + i];
        }
    }
}
void mult(float* A, float* D, float* B, float* res, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            res[n * i + j] = 0;
            for (int k = 0; k < n; k++) {
                res[n * i + j] = res[n * i + j] + A[n * i + k] * B[n * k + j] * D[k];
            }
        }
    }
}
void choleskyN(float* A, float* AM, int n) {
    static const int nmax = 25;
    float D[nmax];
    float L[nmax];
    float LM[nmax];
    float LMT[nmax];
    for (int j = 0; j < n; j++) {
        D[j] = A[(n + 1) * j];
        L[(n + 1) * j] = 1;
        for (int k = 0; k < j; k++) {
            D[j] = D[j] - L[n * j + k] * L[n * j + k] * D[k];
            //std::cout << j;
        }
        for (int i = 0; i < j; i++) {
            L[n * i + j] = 0;
        }
        for (int i = j + 1; i < n; i++) {
            L[n * i + j] = A[n * i + j];
            for (int k = 0; k < j; k++) {
                L[n * i + j] = L[n * i + j] - L[n * i + k] * L[n * j + k] * D[k];
            }
            L[n * i + j] = L[n * i + j] / D[j];
        }
    }
    //displayMat(L);
    invTrinangleInf(L, LM, n);
    invD(D, n);
    transpose(LM, LMT, n);
    mult(LMT, D, LM, AM, n);

}
bool algoStack::append(float v, unsigned long t)
{
    bool b = (count < 2 * demiplage + 1);
    fin = (fin + 1) % buffsize;
    if (count < 2 * demiplage + 1) {
        count++;
        milieu = std::min(demiplage, count);
    }
    else {
        debut = (debut + 1) % buffsize;
        milieu = (milieu + 1) % buffsize;
    }
    vstack[fin] = v;
    tstack[fin] = t;
    Min = vstack[debut];
    Max = Min;
    int i = debut;
    float vm = vstack[milieu];
    bool picMax = true;
    bool picMin = true;
    while (i != fin) {
        if (vstack[i] > vm) {
            picMax = false;
        }
        if (vstack[i] < vm) {
            picMin = false;
        }
        if (vstack[i] < Min) {
            Min =vstack[i];
        }
        if (vstack[i] > Max) {
            Max = vstack[i];
        }
        i = (i + 1) % buffsize;
    }
    picMid = picMin || picMax;
    return !b;
}

algoStack::algoStack(int dp, int sp)
{
    demiplage = dp;
    sampleperiod = sp;
}