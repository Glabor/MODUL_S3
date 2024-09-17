#include "angle.h"

angle::angle(Adafruit_LSM6DSOX* acc,String modele) {
    dsox = acc;
    if(modele=="ripperL17"){
        rotmat[0]=1;//axe x vers la gauche
        rotmat[5]=1;//axe z vers le haut
        rotmat[7]=-1;//axe y vers l'arrière
    }
    else if(modele=="etrier17"){
        rotmat[0]=1;//axe x vers la gauche
        rotmat[5]=1;//axe z vers le haut
        rotmat[7]=-1;//axe y vers l'arrière
    }
    else{//ancien l17
        rotmat[0]=1;//axe x vers la gauche
        rotmat[4]=1;//axe y vers lle haut
        rotmat[8]=1;//axe z vers l'avant
    }
}
void angle::initangle(float ax,float ay,float az,float gx,float gy,float gz,unsigned long time) {
    applyrot(&ax, & ay, & az, & gx, & gy, & gz);
    kalman_p = 3000;
    kalman_q = 1.5;  //bruit gyro
    kalman_r = 300;  //bruit accelero
    acfx = ax;
    acfy = ay * sqrt(ay + az) / abs(ay);
    wy =gy;
    wz = gz;
    w = sqrt(wy*wy + wz*wz) * wz / abs(wz);
    tangle0 = time;
    t = tangle0;
    anglef = atan2(-acfy, acfx) * 180 / M_PI;
    if (anglef < 0.0) {  // keep angles positive
        anglef += 360.0;
    }
}
float angle::correctionangle(float alpha,float ax, float ay, float az, float gx, float gy, float gz, unsigned long time) {
    applyrot(&ax, & ay, & az, & gx, & gy, & gz);
    double dtangle = float(time - tangle0) / 1000000;  //temps en millis
    tangle0 = time;
    wy = (1 - alpha) * wy + alpha * gy;
    wz = (1 - alpha) * wz + alpha * gz;
    w = sqrt(wy * wy + wz * wz) * wz / abs(wz);
    float rx = ax - acfx * cos(w * dtangle) - acfy * sin(w * dtangle);
    float ry = ay * sqrt(ay*ay + az*az) / abs(ay) + acfx * sin(w * dtangle) - acfy * cos(w * dtangle);
    /*float rx = ax - acfx + wz * acfy - wy * acfz;
    float ry = ay - acfy - wz * acfx;
    float rz = az - acfz + wy * acfx;*/
    float k = (kalman_p + kalman_q * dtangle) / (kalman_p + kalman_q * dtangle + kalman_r);
    acfx = acfx * cos(w * dtangle) + acfy * sin(w * dtangle) + k * rx;
    acfy = -acfx * sin(w * dtangle) + acfy * cos(w * dtangle) + k * ry;
    /*acfx = acfx - wz * acfy + wy * acfz + k * rx;
    acfy = acfy + wz * acfx + k * ry;
    acfz = acfz - wy * acfx + k * rz;*/
    kalman_p = (1 - k) * (kalman_p + kalman_q * dtangle);
    anglef = atan2(-acfy, acfx) * 180 / M_PI;
    if (anglef < 0.0) {  // keep angles positive 
        anglef += 360.0;
    }
    return anglef;
}
float angle::wheelRot2(void) {
    //int rotW;
    unsigned long sampleTime = 2000;
    unsigned long micro0 = millis();
    dsox->getEvent(&accel, &gyro, &temp);
    initangle(accel.acceleration.x, accel.acceleration.y, accel.acceleration.z, accel.gyro.x, accel.gyro.y, accel.gyro.z,micro0);
    while (micros() - micro0 < sampleTime*1000) {
        dsox->getEvent(&accel, &gyro, &temp);
        anglef=correctionangle(0.1,accel.acceleration.x, accel.acceleration.y, accel.acceleration.z, accel.gyro.x, accel.gyro.y, accel.gyro.z, micros() );
    }
    return w;
}
float angle::updateangle(unsigned long t_new) {
    dtt = std::min(float(t_new - t), float(t_new - tangle0)) / 1000000;
    anglef += dtt * w * 180 / M_PI;
    if (anglef >= 360) { anglef -= 360; }
    if (anglef < 0) { anglef += 360; }
    return anglef;
}
void angle::applyrot(float* ax, float* ay, float* az, float* gx, float* gy, float* gz){
    float axt=rotmat[0]* *ax+rotmat[1]* *ay+rotmat[2]* *az;
    float ayt=rotmat[3]* *ax+rotmat[4]* *ay+rotmat[5]* *az;
    float azt=rotmat[6]* *ax+rotmat[7]* *ay+rotmat[8]* *az;
    float gxt=rotmat[0]* *gx+rotmat[1]* *gy+rotmat[2]* *gz;
    float gyt=rotmat[3]* *gx+rotmat[4]* *gy+rotmat[5]* *gz;
    float gzt=rotmat[6]* *gx+rotmat[7]* *gy+rotmat[8]* *gz;
    *ax=axt;
    *ay=ayt;
    *ay=ayt;
    *gx=gxt;
    *gy=gyt;
    *gy=gyt;

}
