
/*
 * ospi.c:
 *	Simple test program to test ospi function
 */

#include <wiringPi.h>
#include <wiringShift.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define NUM_STATIONS 16

#define SR_CLK_PIN  7
#define SR_NOE_PIN  0
#define SR_DAT_PIN  2
#define SR_LAT_PIN  3

int values[NUM_STATIONS];
void setShfitRegister(int values[]);

void disableShiftRegisterOutput()
{
  digitalWrite(SR_NOE_PIN, 1);
}

void enableShiftRegisterOutput()
{
  digitalWrite(SR_NOE_PIN, 0);
}

void setShiftRegister(int values[NUM_STATIONS])
{
  digitalWrite(SR_CLK_PIN, 0);
  digitalWrite(SR_LAT_PIN, 0);
  unsigned char s;
  for(s=0; s<NUM_STATIONS; s++) {
    digitalWrite(SR_CLK_PIN, 0);
    digitalWrite(SR_DAT_PIN, values[NUM_STATIONS-1-s]);
    digitalWrite(SR_CLK_PIN, 1);
  }
  digitalWrite(SR_LAT_PIN, 1);
}

void resetStations()
{
  unsigned char s;
  for(s=0; s<NUM_STATIONS; s++) {
    values[s] = 0;
  }
  setShiftRegister(values);
}

int main (void)
{
  printf ("OpenSprinkler Raspberry Pi Edition Test Program\n") ;

  if (wiringPiSetup () == -1)
    exit (1) ;

  printf ("wiringPi initialized.\n");

  pinMode (SR_CLK_PIN, OUTPUT);
  pinMode (SR_NOE_PIN, OUTPUT);
  disableShiftRegisterOutput();
  pinMode (SR_DAT_PIN, OUTPUT);
  pinMode (SR_LAT_PIN, OUTPUT);

  printf ("Shift register initialized.\n");

  resetStations();
  enableShiftRegisterOutput();

  printf ("Entering loop...\n");
  while(1) {
    unsigned char i;
    for(i=0; i<NUM_STATIONS; i++) {
      printf ("Stn %d on\n", i);
      values[i] = 1;
      setShiftRegister(values);
      delay(10000);
      values[i] = 0;
      setShiftRegister(values);
      delay(500);
    }
  }

  return 0 ;
}
