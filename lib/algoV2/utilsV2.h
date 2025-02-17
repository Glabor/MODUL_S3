#ifndef UTILS_H
#define UTILS_H
#define buffsize 200 //0.2tour secondes
#include <Arduino.h>
void invTrinangleInf(float* L, float* LM, int n);
void invD(float* D, int n);
void transpose(float* L, float* LP, int n);
void mult(float* A, float* D, float* B, float* res, int n);
void choleskyN(float* A, float* AM, int n);

class algoStack {
public:
	algoStack(int dp=3,int sp=1000);
	bool append(float v, unsigned long t);
	float vstack[buffsize];           //  value stack
	unsigned long tstack[buffsize]; //  time stack
	int debut = buffsize - 1;
	int milieu = 0;
	int count = 0;
	int fin = buffsize - 1;
	int demiplage = 3;
	int sampleperiod = 1000;
	float Max=0;
	float Min=0;
	bool picMid = false;
};
#endif

