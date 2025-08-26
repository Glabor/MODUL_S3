#include "algoPicots.h"
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
void skip(File inFile,int n){
    uint8_t temp;
    for(int i=0;i<n;i++){
        inFile.readBytes((char*)&temp, sizeof(temp));
    }
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
    //return t4<<24|t3<<16|t2<<8|t1;
}
bool algoPicots::runFromFile(float omega, int r, int R,String path){
    if(omega==0){
        omega=getW(path);
        Serial.println("rotation from file: "+String(omega));
    }
    //File inFile=SD_MMC.open(path,FILE_READ);
    binFile file;
    file.readHeader(path);
    file.header.print();
    if(!file.inFile){
        error="cannot open file";
        return false;
    }
    int long t0=millis();
    nmeas=(file.inFile.size()-file.header.headerLength)/2/308;
    Serial.print("nb de cycles: ");
    Serial.println(nmeas);
    //inFile=SD_MMC.open(path,FILE_READ);
    float perf=2*M_PI*float(r)/float(R)/omega*1000000/float(nd);
    fil=new filtreSick;
    fil->init(0.8, long(perf), 1200);
    if(fil->demiplage<2){
        error="period too short";
        return false;
    }
    if(fil->demiplage>=buffsize/2-1){
        error="period too long";
        return false;
    }
    fstack= new algoStack (fil->demiplage, 1200);
    usure = new usurePicots(fstack);
    comptage=new comptagePicots (fstack);
    patinage=new patinagePicots(fstack);
    float ax, ay, az, gx, gy, gz, v,anglef;
    unsigned long t;
    bool first = true;
    angle rot=angle(nullptr,"etrier17");
    for (int i = 0; i < nmeas; i++) {
        t=timestampRead(file.inFile);
        Serial.println(t/1000);
        ax=float(read(file.inFile))/100;
        ay=float(read(file.inFile))/100;
        az=float(read(file.inFile))/100;
        gx=float(read(file.inFile))/100;
        gy=float(read(file.inFile))/100;
        gz=float(read(file.inFile))/100;
        if (first) {
            Serial.println("init");
            Serial.print("omega = ");
            Serial.println(omega);
            Serial.print("r = ");
            Serial.println(r);
            Serial.print("R = ");
            Serial.println(R);
			rot.initangle(ax, ay, az, gx, gy, gz, t);
			first = false;
		}
        else{
            anglef=rot.correctionangle(0.1,ax, ay, az, gx, gy, gz, t);
        }
        for (int j = 0; j < 100; j++) {
			t=timestampRead(file.inFile);
			v = read(file.inFile);
			anglef=rot.updateangle(t);
            int n = fil->newVal(v, t);
			for (int k = 1; k <= n;k++) {
				if (fil->nextVal(k)) {
                    //Serial.println(String(fil.tf / 1000)+","+String(fil.vf));
					if (fstack->append(fil->vf, fil->tf)){
						usure->update();
						patinage->update(anglef);
						if (comptage->update()) {
                            //Serial.println(String(fil.tf / 1000)+","+String(fil.vf));
						}
                    }
                }
            }
        }
    }
    patinage->compressprofil();
    file.close();
    Serial.println("algo done");
    return true;
}
float algoPicots::getW(String path){
    binFile file;
    file.readHeader(path);
    //File infile=SD_MMC.open(path,FILE_READ);
    if(!file.inFile){
        error="cannot open file";
        return 0;
    }
    nmeas=(file.inFile.size()-file.header.headerLength)/2/308;
    if(nmeas==0){return 0;}
    long Gx=0;
    long Gy=0;
    long Gz=0;
    for (int i = 0; i < nmeas; i++) {
        skip(file.inFile,10);
        Gx+=read(file.inFile);
        Gy+=read(file.inFile);
        Gz+=read(file.inFile);
        skip(file.inFile,600);
    }
    file.close();
    return sqrt(pow(float(Gx)/float(nmeas)/100.0,2)+pow(float(Gy)/float(nmeas)/100.0,2)+pow(float(Gz)/float(nmeas)/100.0,2));
}