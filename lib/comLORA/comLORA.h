#ifndef COMLORA_H
#define COMLORA_H
#include "pinout.h"
#include <RHSoftwareSPI.h>
#include <RH_RF95.h>

#define RF95_FREQ 433.0

class comLORA {
public:
    comLORA(pinout *p);
    bool rf95Setup(void);
    void pinSetup();
    RH_RF95 *rf95;

private:
    pinout *pins;
    RHSoftwareSPI *rhSPI;
};
#endif