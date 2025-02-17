#pragma once
#include"utilsV2.h"
class patinagePicots
{
public:
	patinagePicots(algoStack* s);
	void update(float anglef);
	void compressprofil();
	float pat1f; // percent value of pat1
	int seuil1 = 20;            // premier seuil
	unsigned char probfilb[45];
private:
	algoStack* stack = nullptr;
	unsigned long pat1 = 0;
	unsigned long nmoy = 0;
	unsigned long npattemp = 0;//temps total sur un degre
	unsigned long pattemp = 0;//temps d'adherence sur un degre
	int angleold = -1;//angle profil precedent
	bool profil[360];
};