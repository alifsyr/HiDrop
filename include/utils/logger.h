#pragma once
#include <Arduino.h>
#include <WebSerial.h>

class Logger {
public:
    template<typename T>
    static void print(T val) {
        Serial.print(val);
        WebSerial.print(val);
    }
    
    template<typename T, typename U>
    static void print(T val, U format) {
        Serial.print(val, format);
        WebSerial.print(val, format);
    }
    
    template<typename T>
    static void println(T val) {
        Serial.println(val);
        WebSerial.println(val);
    }
    
    template<typename T, typename U>
    static void println(T val, U format) {
        Serial.println(val, format);
        WebSerial.println(val, format);
    }
    
    static void println() {
        Serial.println();
        WebSerial.println();
    }
    
    static void printf(const char* format, ...) {
        va_list arg;
        va_start(arg, format);
        char loc_buf[128];
        char * temp = loc_buf;
        int len = vsnprintf(temp, sizeof(loc_buf), format, arg);
        if (len < 0) {
            va_end(arg);
            return;
        }
        if (len >= sizeof(loc_buf)){
            temp = (char*)malloc(len + 1);
            if (temp == NULL) {
                va_end(arg);
                return;
            }
            vsnprintf(temp, len + 1, format, arg);
        }
        va_end(arg);
        Serial.print(temp);
        WebSerial.print(temp);
        if (temp != loc_buf){
            free(temp);
        }
    }
};
