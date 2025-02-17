#ifndef filtreSick_H
#define filtreSick_H
//#include <ArduinoJson.h>
#include <math.h>
#include<cmath>
#include"utilsV2.h"
#define M_PI 3.14159265358979323846
#define ncor 5
class filtreSick
{
public:
	void init(float ratioplage, long per, int sampleper);//plage, periode
	float vf;
	unsigned long tf;
	int newVal(int V, unsigned long t);//en sortie le nombre de nouvelles valeurs filtrées
	bool nextVal(int i);
	int demiplage;
	
private:
	float rp;
	int sampleperiod;
	float AM[ncor*ncor];
	int period0;
	
	float convk(int k, int p, int i);
	int debut = 0;
	int fin = 0;
	int count = 0;//nb de points dans le stack
	float Vstack[buffsize];           // value stack
	float vold;
	float vsold;//sync
	float vnold;
	unsigned long told;
	unsigned long tsold;
	float vnew;
	unsigned long tnew;
	int Nnew;
	float Cstack[ncor] = { 0,0,0,0,0 }; // conv stack
	float CPstack[ncor] = { 0,0,0,0,0 }; // conv stack periodique
	//int counter = 0;//n
	int countper = 0;
	bool updatePstack(float vn);
	bool updateCstack(float vn);
};
#endif

