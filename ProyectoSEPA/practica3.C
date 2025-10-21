//*****************************************************************************
//
// hello.c - Simple hello world example.
//
// Copyright (c) 2013-2014 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
//
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
//
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
//
// This is part of revision 2.1.0.12573 of the EK-TM4C1294XL Firmware Package.
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "driverlib2.h"
#include "utils/uartstdio.h"



/*
 * EJEMPLO 4: Manejo simple del terminal usando las funciones del UARTstdio
 * Es necesario incluir el fichero uartstdio.c, que estÃ¡ en el directorio
 * <TIVAWARE_INSTALL>/utils/
 * Para ello: add files
 *              -> se busca
 *                  ->link to files
 *                      -> create link locations relative to
 *                          -> TIVAWARE_INSTALL
 */


//*****************************************************************************
//
// System clock rate in Hz.
//
//*****************************************************************************

#define T_maxespera 6
#define T_maxejecucion 40
#define T_interejecucion 20
#define MSEC 40000
#define MaxEst 10


#define B1_OFF GPIOPinRead(GPIO_PORTJ_BASE,GPIO_PIN_0)
#define B1_ON !(GPIOPinRead(GPIO_PORTJ_BASE,GPIO_PIN_0))
#define B2_OFF GPIOPinRead(GPIO_PORTJ_BASE,GPIO_PIN_1)
#define B2_ON !(GPIOPinRead(GPIO_PORTJ_BASE,GPIO_PIN_1))

volatile int Max_pos = 4000; //3750
volatile int pos_arriba = 4000; //3750

volatile int pos_bajada = 2750;

volatile int Min_pos = 1300; //1875
int RELOJ, PeriodoPWM;
volatile int pos_100, pos_1000;
volatile int posicion;

//#define SLEEP SysCtlSleep()
#define SLEEP SysCtlSleepFake()

volatile int Flag_ints=0;

void SysCtlSleepFake(void)
{
    while(!Flag_ints);
    Flag_ints=0;
}
void IntTimer0(void);
void IntTimer1(void);

char respuesta[50];
int num;
int estado=0;
char letra;
int main(void)
{
    //
    // Run from the PLL at 120 MHz.
    //
    RELOJ = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
            SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
            SYSCTL_CFG_VCO_480), 120000000);


    //
    // Initialize the UART.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    PWMClockSet(PWM0_BASE,PWM_SYSCLK_DIV_64);   // al PWM le llega un reloj de 1.875MHz

    GPIOPinConfigure(GPIO_PG0_M0PWM4);          //Configurar el pin a PWM
    GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_4);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_0 |GPIO_PIN_4); //F0 y F4: salidas
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0 |GPIO_PIN_1); //N0 y N1: salidas

    GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_0|GPIO_PIN_1);
    GPIOPadConfigSet(GPIO_PORTJ_BASE,GPIO_PIN_0|GPIO_PIN_1,GPIO_STRENGTH_2MA,GPIO_PIN_TYPE_STD_WPU);

    //Configurar el pwm0, contador descendente y sin sincronización (actualización automática)
    PWMGenConfigure(PWM0_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);

    PeriodoPWM=37499; // 50Hz  a 1.875MHz
    pos_100=50; //Inicialmente, 50%
    pos_1000=500;
    posicion=Min_pos+((Max_pos-Min_pos)*pos_100)/100;
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, PeriodoPWM); //frec:50Hz
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, posicion);   //Inicialmente, 1ms

    PWMGenEnable(PWM0_BASE, PWM_GEN_2);     //Habilita el generador 0

    PWMOutputState(PWM0_BASE, PWM_OUT_4_BIT , true);    //Habilita la salida 1


    UARTStdioConfig(0, 115200, RELOJ);
    UARTprintf("\033[1;37;40m \033[2J \033[0;0f");
    UARTprintf("------MENU de PRUEBA--------- \n");
    UARTprintf("| - Elige una opción (1..3)-| \n");
    UARTprintf("| - 1.Huevo frito          -| \n");
    UARTprintf("| - 2.Tortilla francesa    -| \n");
    UARTprintf("| - 3.Revuelto             -| \n");
    UARTprintf("----------------------------- \n");

    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);       //Habilita T0
     TimerClockSourceSet(TIMER0_BASE, TIMER_CLOCK_SYSTEM);   //T0 a 120MHz
     TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);    //T0 periodico y conjunto (32b)
     TimerLoadSet(TIMER0_BASE, TIMER_A, (RELOJ/50) -1);  //20ms
     TimerIntRegister(TIMER0_BASE,TIMER_A,IntTimer0);
     IntEnable(INT_TIMER0A); //Habilitar las interrupciones globales de los timers
     TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);    // Habilitar las interrupciones de timeout
     IntMasterEnable();  //Habilitacion global de interrupciones
     TimerEnable(TIMER0_BASE, TIMER_A);  //Habilitar Timer0

     SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);       //Habilita T0
         TimerClockSourceSet(TIMER1_BASE, TIMER_CLOCK_SYSTEM);   //T0 a 120MHz
         TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);    //T0 periodico y conjunto (32b)
         TimerLoadSet(TIMER1_BASE, TIMER_A, (RELOJ/8) -1);  //125ms
         TimerIntRegister(TIMER1_BASE,TIMER_A,IntTimer0);
         IntEnable(INT_TIMER1A); //Habilitar las interrupciones globales de los timers
         TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);    // Habilitar las interrupciones de timeout
         IntMasterEnable();  //Habilitacion global de interrupciones
         TimerEnable(TIMER1_BASE, TIMER_A);  //Habilitar Timer0

    estado=0;
    while(1){
        SLEEP;

        switch(estado)
        {
        case 0:
            posicion = pos_arriba;
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, posicion);

         break;
        }


    }
}



void IntTimer0(void)
{
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT); // Borra flag
    SysCtlDelay(20); //Retraso necesario. Mirar Driverlib p.550
    Flag_ints++;
}
void IntTimer1(void)// aqui tengo que meterle los estados para que hagan los estados de encendido y apagado
{
    TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT); // Borra flag
    SysCtlDelay(20); //Retraso necesario. Mirar Driverlib p.550
    //me va a servir para los leds
    GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, GPIO_PIN_1);
    GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, GPIO_PIN_0);
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_PIN_4);
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_PIN_0);
    Flag_ints++;
}



