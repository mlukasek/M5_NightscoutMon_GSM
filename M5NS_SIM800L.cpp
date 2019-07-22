#include <M5Stack.h>
#include "sys/time.h"
// #include <util/eu_dst.h>
#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>

#include "M5NS_SIM800L.h"

int cmdOK(String cmd, unsigned long tmout=40000) {
  unsigned long zac=millis();
  Serial.print(cmd);
  Serial2.println(cmd);
  delay(100);
  String tmpstr;
  bool leaving = false;
  while(!leaving && (millis()-zac<tmout)) {
    if(Serial2.available()) {
      tmpstr += char(Serial2.read());
      // Serial.println(tmpstr.substring(tmpstr.length()-2));
      if(tmpstr.substring(tmpstr.length()-2)=="OK") {
        // Serial.println("Leaving with OK");
        leaving = true;
      }
      if(tmpstr.substring(tmpstr.length()-5)=="ERROR") {
        // Serial.println("Leaving with ERROR");
        leaving = true;
      }
    }
    delay(1);
  }
  if(millis()-zac>=tmout)
    Serial.println(" Command TIMEOUT");
  // Serial.print("cmd to compare = \'"); Serial.print(tmpstr.substring(0,cmd.length())); Serial.println("\'");
  if(tmpstr.substring(0,cmd.length())==cmd)
    tmpstr.remove(0,cmd.length());
  while(Serial2.available()) {
    Serial2.read();
    delay(1);
  }
  while(tmpstr.charAt(0)=='\r' || tmpstr.charAt(0)=='\n' || tmpstr.charAt(0)==' ')
    tmpstr.remove(0,1);
  while(tmpstr.charAt(tmpstr.length()-1)=='\r' || tmpstr.charAt(tmpstr.length()-1)=='\n' || tmpstr.charAt(tmpstr.length()-1)==' ')
    tmpstr.remove(tmpstr.length()-1,1);
  if(tmpstr.substring(tmpstr.length()-2)!="OK") {
    Serial.print(" - ERROR, response = \""); Serial.print(tmpstr); Serial.print("\"");
    Serial.print(", length = "); Serial.print(tmpstr.length());
    unsigned long trv=millis()-zac;
    Serial.print(", "); Serial.print(trv); Serial.println(" ms");
    return trv;
  }
  unsigned long trv=millis()-zac;
  Serial.print(" - OK "); Serial.print(trv); Serial.println(" ms");
  return 0;
}

String cmd_res(String cmd, unsigned long tmout=40000) {
  unsigned long zac=millis();
  Serial.print(cmd);
  Serial2.println(cmd);
  delay(100);
  String tmpstr;
  bool leaving = false;
  int err=0;
  while(!leaving && (millis()-zac<tmout)) {
    if(Serial2.available()) {
      tmpstr += char(Serial2.read());
      // Serial.println(tmpstr.substring(tmpstr.length()-2));
      if(tmpstr.substring(tmpstr.length()-2)=="OK") {
        // Serial.println("Leaving with OK");
        tmpstr.remove(tmpstr.length()-2,2);
        leaving = true;
      }
      if(tmpstr.substring(tmpstr.length()-5)=="ERROR") {
        Serial.print(" - ERROR");
        tmpstr.remove(tmpstr.length()-5,5);
        leaving = true;
        err=1;
      }
    }
    delay(1);
  }
  if(millis()-zac>=tmout) {
    Serial.print(" - TIMEOUT");
    err=2;
  }
  // Serial.print("cmd to compare = \'"); Serial.print(tmpstr.substring(0,cmd.length())); Serial.println("\'");
  if(tmpstr.substring(0,cmd.length())==cmd)
    tmpstr.remove(0,cmd.length());
  while(tmpstr.charAt(0)=='\r' || tmpstr.charAt(0)=='\n' || tmpstr.charAt(0)==' ')
    tmpstr.remove(0,1);
  while(tmpstr.charAt(tmpstr.length()-1)=='\r' || tmpstr.charAt(tmpstr.length()-1)=='\n' || tmpstr.charAt(tmpstr.length()-1)==' ')
    tmpstr.remove(tmpstr.length()-1,1);
  if(err) {
    Serial.print(" response = \""); Serial.print(tmpstr); Serial.print("\"");
    Serial.print(", length = "); Serial.print(tmpstr.length());
  } else
    Serial.print(" - OK");
  while(Serial2.available()) {
    Serial2.read();
    delay(1);
  }
  unsigned long trv=millis()-zac;
  Serial.print(", "); Serial.print(trv); Serial.println(" ms");
  if(!err)
    Serial.println(tmpstr);
  return tmpstr;
}

int reset_SIM800() {
  Serial.println("Resetting SIM800 modem ... ");
  // digitalWrite(RESET_PIN, LOW);
  // delay(100);
  // digitalWrite(RESET_PIN, HIGH);
  // delay(1000);
  cmdOK("AT+CFUN=0");
  delay(2000);
  cmdOK("AT+CFUN=0");
  delay(1000);
  cmdOK("AT+CFUN=1,1");
  delay(10000);
  unsigned long zac=millis();
  while(!cmd_res("AT+CREG?", 2000).startsWith("+CREG: 0,1") && millis()-zac<60000) {
    delay(2000);
  }
  if(millis()-zac>=60000) {
    Serial.println("Network connection TIMEOUT");
    return 1;
  } else
    Serial.println("Network connection OK");
  delay(1000);
  return 0;
}

int SIM800_beginGPRS(String apn) {
  String tmpstr;
  
  cmdOK("AT");
  cmdOK("AT+CFUN=1");
  if(!cmd_res("AT+CREG?").startsWith("+CREG: 0,1"))
    return 1;
  if(!cmd_res("AT+COPS?").startsWith("+COPS: 0,0,")) // +COPS: 0,0,"T-Mobile CZ"
    return 2;
  tmpstr=cmd_res("AT+CSQ");  // +CSQ: 19,0
  if(!tmpstr.startsWith("+CSQ: "))
    return 3;
  tmpstr.remove(0,6);
  tmpstr.remove(tmpstr.indexOf(','));
  int signalStrength=tmpstr.toInt();
  Serial.print("Signal strength: "); Serial.println(signalStrength);

  cmdOK("AT+SAPBR=3,1,\"Contype\",\"GPRS\"");
  tmpstr="AT+SAPBR=3,1,\"APN\",\"";
  tmpstr+=apn;
  tmpstr+="\"";
  cmdOK(tmpstr);
  cmdOK("AT+SAPBR=1,1");
  if(!cmd_res("AT+SAPBR=2,1").startsWith("+SAPBR: 1,1,"))
    return 4;

  /*
  delay(2000);
  cmdOK("AT+CLTS=1");
  cmdOK("AT+CENG=3");
  cmd_res("AT+CCLK?"); // +CCLK: "04/01/01,00:57:43+00"
  */
  
  cmdOK("AT+HTTPINIT");
  delay(1000);
  return 0;
}

int SIM800_endGPRS() {
  cmdOK("AT+HTTPTERM");
  cmdOK("AT+SAPBR=0,1");
  delay(1000);
  return 0;
}

String get_json(String url) {
  String tmpstr;
  
  if(url.startsWith("https://"))
    cmdOK("AT+HTTPSSL=1");
  else
    cmdOK("AT+HTTPSSL=0");
  cmdOK("AT+HTTPPARA=\"CID\",1");
  String cmd="AT+HTTPPARA=\"URL\",\"";
  cmd+=url;
  cmd+="\"";
  cmdOK(cmd);
  cmdOK("AT+HTTPACTION=0");

  delay(100);
  unsigned long zac=millis();
  bool leaving = false;
  char ch;
  int err=0;
  while(!leaving && (millis()-zac<40000)) {
    if(Serial2.available()) {
      ch = char(Serial2.read());
      if(ch!='\r' && ch!='\n')
        tmpstr += ch;
      // Serial.println(tmpstr.substring(tmpstr.length()-2));
      if(tmpstr.substring(tmpstr.length()-12)=="+HTTPACTION:") {
        // Serial.println("Leaving with +HTTPACTION:");
        // tmpstr.remove(tmpstr.length()-12,2);
        leaving = true;
      }
    }
    delay(1);
  }
  if(millis()-zac>=40000) {
    Serial.print(" - TIMEOUT");
    err=2;
  }
  while(Serial2.available()) {
    tmpstr += char(Serial2.read());
    delay(1);
  }
  Serial.print("result = "); Serial.print(tmpstr);
  unsigned long trv=millis()-zac;
  Serial.print("Waited for "); Serial.print(trv); Serial.println(" ms");
  delay(100);
  
  String json = cmd_res("AT+HTTPREAD");
  while(json.length()>0 && json.charAt(0)!='\r' && json.charAt(0)!='\n') {
    json.remove(0,1);
  }
  while(json.length()>0 && (json.charAt(0)=='\r' || json.charAt(0)=='\n')) {
    json.remove(0,1);
  }
  while(json.length()>0 && (json.charAt(json.length()-1)=='\r' || json.charAt(json.length()-1)=='\n')) {
    json.remove(json.length()-1,1);
  }
  delay(100);
  // cmd_res("AT+CCLK?"); // +CCLK: "04/01/01,00:57:43+00"
  while(Serial2.available()) {
    Serial2.read();
    delay(1);
  }  
  // Serial.print("Mame \'"); Serial.print(json); Serial.println("\'"); 
  return json;
}
