#pragma once
#include"utilsV2.h"
#include <Arduino.h>
class usurePicots
{
public:
	usurePicots(algoStack* s);
	void update();
	float usuremoyu = 0;        // average wear M
	int usureMu = INT_MIN;      // max wear M ever
	int usuremu = INT_MAX;      // min wear M ever
	float usuremoyd = 0;        // average wear m
	int usureMd = INT_MIN;      // max wear m ever
	int usuremd = INT_MAX;      // min wear m ever
	int deltaM;                 // max amplitude ever
	int seuil1 = 20;            // premier seuil
private:
	algoStack* stack = nullptr;
	unsigned long nmoy = 0;

};

