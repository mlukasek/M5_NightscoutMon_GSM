#ifndef _M5NS_SIM800L_H
#define _M5NS_SIM800L_H

#include <M5Stack.h>

String get_json(String url);
int reset_SIM800();
int SIM800_beginGPRS(String apn);
int SIM800_endGPRS();

#endif
