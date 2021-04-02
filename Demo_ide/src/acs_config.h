#include <Arduino.h>
#define analogInPin A0
#define timeprocess 100

typedef struct
{
    boolean t100ms = 0;
    boolean t250ms = 0;
    boolean t500ms = 0;
    boolean t1s = 0;
    boolean t5s = 0;
    boolean t10s = 0;
    boolean fault = 0;
    boolean doProcess = 0;
    boolean lowLoad = false;
} flagType;
flagType Flag;
typedef struct
{
    unsigned long t100ms = 0;
    unsigned long t250ms = 0;
    unsigned long t500ms = 0;
    unsigned long t1s = 0;
    unsigned long t5s = 0;
    unsigned long t10s = 0;
    unsigned long tprocess = 0;
} startType;
startType StartTimer;

int maxValue = 0;
int minValue = 0;
double Vpp = 0;
double VppOld = 0;
double Peak = 0;
double _Ireal;