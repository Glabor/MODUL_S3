#include <SD_MMC.h>
#include <Arduino.h>
#include "angle.h"
#include "filtreSick.h"
#include "patinagePicots.h"
#include "usurePicots.h"
#include "comptagePicots.h"
class algoPicots{
public:
bool runFromFile(float omega, int r, int R,String path);
filtreSick* fil=nullptr;
algoStack* fstack=nullptr;
usurePicots* usure=nullptr;
comptagePicots* comptage=nullptr;
patinagePicots* patinage=nullptr;
float w;
String error="";
private:
float getW(String path);
int nmeas=0;//nombre de mesures angle

};