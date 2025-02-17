#include "usurePicots.h"

usurePicots::usurePicots(algoStack* s)
{
    stack = s;
}

void usurePicots::update()
{
    float delta = stack->Max - stack->Min;    // amplitude of values in stack
    if (stack->Max > usureMu)
    { // max M ever
        usureMu = stack->Max;
    }
    if (stack->Min < usuremd)
    { // min m ever
        usuremd = stack->Min;
    }
    if (delta > deltaM)
    { // max amplitude ever
        deltaM = delta;
    }
    bool pat = false;
    if (delta > seuil1) {
        pat = true;
        usuremoyu = (stack->Max + usuremoyu * (nmoy - 1)) / nmoy; // usuremoyu = moy M

        if (stack->Max < usuremu)
        { // min M ever
            usuremu = stack->Max;
        }
        usuremoyd = (stack->Min + usuremoyd * (nmoy - 1)) / nmoy; // moy m
        if (stack->Min > usureMd)
        { // max m ever
            usureMd = stack->Min;
        }

    }
}
