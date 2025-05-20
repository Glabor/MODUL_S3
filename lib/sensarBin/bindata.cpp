#include "bindata.h"
static num numRead(dataType code,File inFile){
    num n;
    n.i=0;
    if(code==FLOAT){
        inFile.readBytes(n.bytes,4);
        return n;
    }
    int l=getL(code);
    if(isByteorderBig(code)){
        for(int i=l-1;i>=0;i--){
            inFile.readBytes(n.bytes+i,1);
        }
        if(isSigned(code)){
            for(int i=l;i<4;i++){
                n.bytes[i]=0xff;
            }
        }
    }
    else{
        inFile.readBytes(n.bytes,l);
    }
    return n;
}
void numWrite(num n,dataType code,File outFile){
    int l=getL(code);
    if(isByteorderBig(code)){
        for(int i=l-1;i>=0;i--){
            outFile.write(n.bytes[i]);
        }
    }
    else{
        for(int i=0;i<l;i++){
            outFile.write(n.bytes[i]);
        }
    }
}
metaData::metaData(File inFile){
    key=strRead(inFile);
    l=key.length()+1;
    inFile.readBytes((char*)&code, 1);
    if (code == STRING){
        s=strRead(inFile);
        l+=s.length()+2;
    }
    else{
        inFile.readBytes(n.bytes, 4);
        l+=5;
    }
}
metaData::metaData(String k, String ss){
    key=k;
    code=STRING;
    s=ss;
    l=k.length()+ss.length()+2;
}
metaData::metaData(String k, DateTime  d){
    key=k;
    code=DATETIME;
    //Serial.print(code);
    n.ui=d.unixtime();
    l=k.length()+6;
}
metaData::metaData(String k, float f){
    key=k;
    code=FLOAT;
    n.f=f;
    l=k.length()+6;
}
metaData::metaData(String k, int32_t i){
    key=k;
    code=SIGNED_4BYTES_L;
    n.i=i;
    l=k.length()+6;
}
metaData::metaData(String k, uint32_t ui){
    key=k;
    code=UNSIGNED_4BYTES_L;
    n.ui=ui;
    l=k.length()+6;
}
void metaData::print(){
    if(code==STRING){
        Serial.print(key+": "+s);
    }
    if(code==DATETIME){
        DateTime d=DateTime(n.ui);
        String str = String(d.year(), DEC) + '/' + String(d.month(), DEC) + '/' + String(d.day(), DEC) + " " + String(d.hour(), DEC) + ':' + String(d.minute(), DEC) + ':' + String(d.second(), DEC);
        Serial.print(key+": "+str);
    }
    if(code==UNSIGNED_4BYTES_L){
        Serial.print(key+": "+String(n.ui));
    }
    if(code==SIGNED_4BYTES_L){
        Serial.print(key+": "+String(n.i));
    }
    if(code==FLOAT){
        Serial.print(key+": "+String(n.f));
    }
}
void metaData::write(File outFile){
    strWrite(key,outFile);
    byte buff[1];
    buff[0]=code;
    //Serial.print(code);
    outFile.write(buff[0]);
    if(code==STRING){
         strWrite(s,outFile);
    }
    else{
        numWrite(n,code,outFile);
    }
}
String strRead(File inFile) {
    String str = "";
    uint8_t length;
    inFile.readBytes((char*)&length, 1);
    char c;
    for (int i = 0; i < length; i++) {
        str += inFile.readString();
    }
    return str;
}
void strWrite(String s,File outFile) {
    byte buff[1];
    buff[0]=s.length();
    outFile.write(buff[0]);
    outFile.print(s);
}
field::field(String nm,String u, dataType code, float m){
    name = nm;
    typeCode = code;
    unit = u;
    multiplier = m;
    nameLength = name.length()+1;
    unitLength = unit.length()+1;
    length = nameLength + unitLength + 3;
};
field::field(File inFile) {
    uint8_t n;
    inFile.readBytes((char*)&n, 1);
    typeCode = dataType(n);
    name = strRead(inFile);
    nameLength = name.length();
    unit=strRead(inFile);
    unitLength = unit.length();
    num nn;
    //inFile.readBytes((char*)&multiplier, 2);
    inFile.readBytes(nn.bytes, 4);
    multiplier=nn.f;
    length = nameLength + unitLength + 7;
}
void field::write(File outFile) {
    byte buff[1];
    buff[0]=typeCode;
    outFile.write(buff[0]);
    strWrite(name,outFile);
    strWrite(unit,outFile);
    num n;
    n.f=multiplier;
    numWrite(n,FLOAT,outFile);
}
void field::valueWrite(File outFile){
    num val;
    val.ui=333;
    if(pf!=nullptr){
        val.f=*pf*multiplier;
    }
    if(pi!=nullptr){
        val.i=*pi*multiplier;
    }
    if(pu!=nullptr){
        val.ui=*pu*multiplier;
    }
    numWrite(val,typeCode,outFile);
    //Serial.println("writing "+name);
}
measurement::measurement(File inFile) {
    inFile.readBytes((char*)&nCol, 1);
    inFile.readBytes((char*)&nRow, 2);
    for (int i = 0; i < nCol; i++) {
        Fields.push_back(field(inFile));
        length += Fields[i].length;
    }
};