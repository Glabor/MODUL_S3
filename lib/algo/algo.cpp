#include "algo.h"
bool algo::newval(int V, unsigned long t, float anglef)
{
    unsigned long dt = t - told;
    told = t;
    stack.appendFilt(V, t, t-(unsigned long)period);
    bool ispic = stack.update();                    // pic detecté
    delta = stack.Max - stack.Min;    // amplitude of values in stack
    M=stack.Max;
    m=stack.Min;
    if (delta > deltaM)
    { // max amplitude ever
        deltaM = delta;
    }
    if (delta > seuil1) {
        pat = true;
        pat1 += dt;
        nmoy++;
        usuremoyu = (M + usuremoyu * (nmoy - 1)) / nmoy; // usuremoyu = moy M
        if (M > usureMu)
        { // max M ever
            usureMu = M;
        }
        if (M < usuremu)
        { // min M ever
            usuremu = M;
        }
        usuremoyd = (m + usuremoyd * (nmoy - 1)) / nmoy; // moy m
        if (m > usureMd)
        { // max m ever
            usureMd = m;
        }
        if (m < usuremd)
        { // min m ever
            usuremd = m;
        }
    }
    else
    {
        pat = false;
    }
    if (pat) { pattemp += dt; }
    //pattemp += pat[typepat] * dt;
    npattemp += dt;
    if (abs(anglef - angleold) > 0.9) {

        profil[(int)floor(anglef)] = float(pattemp) / float(npattemp) > 0.5;
        //profil[(int)floor(anglef)] = float(npattemp);
        angleold = anglef;
        pattemp = 0;
        npattemp = 0;
    }
    if (ispic) {
        newpic(stack.vPic, stack.tPic);
    }
    return ispic;
}

bool algo::init(float omega, unsigned long t, int r, int R)
{
    t0 = t;
    told = t;
    r_molette = r;
    R_RDC = R;
    period = 2*M_PI*1000000 * float(r_molette) / abs(omega) / float(R_RDC) / nd;
    if(period<5){
        return false;
    }
    float fs = 1000 / 1.06;//echantillonage
    stack.init(period /1000000* 0.8 *fs, period);
    for (int i = 0; i < nd; i++) {
        sum[i] = 0;
    }
    return true;
}
void algo::newpic(float vPic, unsigned long tPic) {
    int vPicOld = vPicBuff[indPic];
    vPicBuff[indPic] = vPic;
    tPicBuff[indPic] = tPic;
    nPic++;
    int ncor;
    for (int i = 0; i < nd;i++) {
        ncor = 2 * nd - i;
        //long sumsum = 0;
        if (nPic > ncor) {
            if (nPic <= 2*ncor) {
                sum[i] += pow(vPic - float(vPicBuff[(indPic + 4 * nd - ncor) % (4 * nd)]),2);
            }
            else {
                float sumold = sum[i];
                sum[i] += pow(vPic - float(vPicBuff[(indPic + 4 * nd - ncor) % (4 * nd)]), 2);
                if (i > 0) {
                    sum[i] -= pow(float(vPicBuff[(indPic + 4 * nd - ncor) % (4 * nd)]) - float(vPicBuff[(indPic + 4 * nd - 2 * ncor) % (4 * nd)]), 2);
                }
                else {
                    sum[i] -= pow(float(vPicBuff[(indPic + 4 * nd - ncor) % (4 * nd)]) - vPicOld, 2);
                }
                //sumsum += sum[i];
                /*if (i == 0 && nPic == 121) {
                    int i1 = (indPic + 4 * nd - ncor) % (4 * nd);
                    float f1= float(vPicBuff[(indPic + 4 * nd - ncor) % (4 * nd)]);
                    float f2 = float(vPicBuff[(indPic + 4 * nd - ncor) % (4 * nd)]);
                    int i3 = (indPic + 4 * nd - 2 * ncor) % (4 * nd);
                    float f3 = float(vPicBuff[(indPic + 4 * nd - 2 * ncor) % (4 * nd)]);
                    int g = 0;
                }*/
                if (minsum<0 || minsum>sqrt(sum[i] / float(2*ncor))) {
                    //if (true) {
                    minsum = sqrt(sum[i] / float(2*ncor));
                    //ncor = 2 * nd;//pics complet
                    mincor = ncor;
                    //ncor = 2 * nd;//pics complet
                    for (int j = 0; j < ncor * 2; j++) {
                        bestcorr[j] = vPicBuff[(indPic + 1 + 4 * nd - 2 * ncor + j) % (4 * nd)];
                        tbestcorr[j] = tPicBuff[(indPic + 1 + 4 * nd - 2 * ncor + j) % (4 * nd)];
                        //int ttt = 0;
                        
                    }
                }
            }
        }
        
    }
    indPic = (indPic + 1) % (4 * nd);//prochain
}
int read(File inFile){
    int8_t highbyte;
	uint8_t lowbyte;
    inFile.readBytes((char*)&highbyte, sizeof(highbyte));
    inFile.readBytes((char*)&lowbyte, sizeof(lowbyte)); 
    return highbyte<<8|lowbyte;
}
unsigned long uread(File inFile){
    uint8_t highbyte;
	uint8_t lowbyte;
    inFile.readBytes((char*)&highbyte, sizeof(highbyte));
    inFile.readBytes((char*)&lowbyte, sizeof(lowbyte)); 
    return highbyte<<8|lowbyte;
}
unsigned long timestampRead(File inFile){
    uint8_t t1;
	uint8_t t2;
    uint8_t t3;
    uint8_t t4;
    inFile.readBytes((char*)&t1, sizeof(t1));
    inFile.readBytes((char*)&t2, sizeof(t2)); 
    inFile.readBytes((char*)&t3, sizeof(t3));
    inFile.readBytes((char*)&t4, sizeof(t4)); 
    return t1<<24|t2<<16|t3<<8|t4;
}
bool algo::runFromFile(float omega, int r, int R,String path)
{
    float ax, ay, az, gx, gy, gz, v,anglef;
    unsigned long t;
    bool first = true;
    angle rot=angle(nullptr,"");
    File inFile=SD_MMC.open(path,FILE_READ);
    if(inFile){
        Serial.println("opening file");
    }
    else{
        Serial.println("cannot open file");
        return false;
    }
    int long t0=millis();
    int n=inFile.size()/2/308;
    for (int i = 0; i < n; i++) {
        t=timestampRead(inFile);
        Serial.println(t);
        ax=float(read(inFile)/100);
        ay=float(read(inFile)/100);
        az=float(read(inFile)/100);
        gx=float(read(inFile)/100);
        gy=float(read(inFile)/100);
        gz=float(read(inFile)/100);
        if (first) {
            Serial.println("init");
            Serial.print("omega = ");
            Serial.println(omega);
            Serial.print("r = ");
            Serial.println(r);
            Serial.print("R = ");
            Serial.println(R);
            bool in=init(abs(omega), t,  r,  R);
            if(!in){
                return false;
            }
			rot.initangle(ax, ay, az, gx, gy, gz, t);
			first = false;
            
			//init(0.236, t,121,2660);//test2 //periodes en ms
		}
        anglef=rot.correctionangle(0.1,ax, ay, az, gx, gy, gz, t);
        for (int j = 0; j < 100; j++) {
			t=timestampRead(inFile);
			v = read(inFile);
			anglef=rot.updateangle(t);
            bool pic=newval(v, t, anglef);
            Serial.println(j);
        }
    }
    unsigned long TIME = micros() - t0; 
    pat1f = (float)pat1 / (float)(TIME - period * nd); // percent value of patc
    inFile.close();
    compressprofil();
    return true;
}

void algo::compressprofil()
{
    for(int i=0;i<45;i++){
        byte  tempByte=0;
        for(int j=0;j<8;j++){
            tempByte = tempByte | (profil[8 * i + (j)] << (7 - j));
        }
        probfilb[i] = tempByte;
    }
}
