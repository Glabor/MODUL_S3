#ifndef COMLORA_H
#define COMLORA_H
#include "capteurs.h"
#include "pinout.h"
#include <RHSoftwareSPI.h>
#include <RH_RF95.h>
#define RF95_FREQ 433.0

class comLORA {
public:
    comLORA(pinout *p, capteurs *c);
    bool rf95Setup(void);
    void pinSetup();
    void rafale(byte *mess, int length, int id);
    RH_RF95 *rf95;

private:
    pinout *pins;
    capteurs *cap;
    RHSoftwareSPI *rhSPI;
};
#endif