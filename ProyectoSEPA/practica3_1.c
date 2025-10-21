#include <stdint.h>
#include <stdbool.h>
#include "utils/uartstdio.h"
#include "driverlib2.h"
#include <stdlib.h>
/***********************************

 *************************************/
#define T_maxespera 6
#define T_maxejecucion 40
#define T_interejecucion 20
#define MSEC 40000
#define MaxEst 10


#define B1_OFF GPIOPinRead(GPIO_PORTJ_BASE,GPIO_PIN_0)
#define B1_ON !(GPIOPinRead(GPIO_PORTJ_BASE,GPIO_PIN_0))
#define B2_OFF GPIOPinRead(GPIO_PORTJ_BASE,GPIO_PIN_1)
#define B2_ON !(GPIOPinRead(GPIO_PORTJ_BASE,GPIO_PIN_1))

volatile bool toggle_L2 = false;
volatile bool toggle_L3 = false;

volatile int barrera_arriba = 4200; //3750
volatile int barrera_abajoleft =3475;
volatile int barrera_abajo = 2750;
volatile int barrera_abajoright =2025;
volatile int Min_pos = 1300; //1875

int LED[10][4]={
                    1,0,0,1,
                    0,0,0,1,
                    1,0,0,0,
                    0,0,0,1,
                    1,0,0,0,
                    0,0,0,1,
                    1,0,0,0,
                    0,0,0,1,
                    1,0,0,0,
                    0,0,0,0
};


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

void enciende_leds(uint8_t Est)
{
    GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, GPIO_PIN_1*LED[Est][0]);
    GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, GPIO_PIN_0*LED[Est][1]);
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_PIN_4*LED[Est][2]);
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_PIN_0*LED[Est][3]);
}
char respuesta[5];
int estado=0;
char identificacion;
char codigo[8][5]={
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    1,0,0,0,
    0,0,0,1,
    0,0,1,0,
    0,1,0,0,
    1,1,1,1
};

int main(void)
{

    int t_espera=0;
    int t_ejecucion=0;

    RELOJ=SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480), 120000000);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);

	PWMClockSet(PWM0_BASE,PWM_SYSCLK_DIV_64);	// al PWM le llega un reloj de 1.875MHz

	GPIOPinConfigure(GPIO_PG0_M0PWM4);			//Configurar el pin a PWM
    GPIOPinTypePWM(GPIO_PORTG_BASE, GPIO_PIN_0);

    GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_0|GPIO_PIN_1);
    GPIOPadConfigSet(GPIO_PORTJ_BASE,GPIO_PIN_0|GPIO_PIN_1,GPIO_STRENGTH_2MA,GPIO_PIN_TYPE_STD_WPU);

    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_0 |GPIO_PIN_4);    //F0 y F4: salidas
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0 |GPIO_PIN_1); //N0 y N1: salidas

    //Configurar el pwm0, contador descendente y sin sincronización (actualización automática)
    PWMGenConfigure(PWM0_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);

	PeriodoPWM=37499; // 50Hz  a 1.875MHz
    pos_100=50; //Inicialmente, 50%
    pos_1000=500;
    posicion=Min_pos+((barrera_arriba-Min_pos)*pos_100)/100;
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, PeriodoPWM); //frec:50Hz
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, posicion); 	//Inicialmente, 1ms

    PWMGenEnable(PWM0_BASE, PWM_GEN_2);		//Habilita el generador 0

    PWMOutputState(PWM0_BASE, PWM_OUT_4_BIT , true);	//Habilita la salida 1



    //inicializacion de la uart
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    UARTStdioConfig(0, 115200, RELOJ);
// timer 0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);       //Habilita T0
    TimerClockSourceSet(TIMER0_BASE, TIMER_CLOCK_SYSTEM);   //T0 a 120MHz
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);    //T0 periodico y conjunto (32b)
    TimerLoadSet(TIMER0_BASE, TIMER_A, (RELOJ/20) -1);  //50ms
    TimerIntRegister(TIMER0_BASE,TIMER_A,IntTimer0);
    IntEnable(INT_TIMER0A); //Habilitar las interrupciones globales de los timers
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);    // Habilitar las interrupciones de timeout
    IntMasterEnable();  //Habilitacion global de interrupciones
    TimerEnable(TIMER0_BASE, TIMER_A);  //Habilitar Timer0
// timer 1
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);       //Habilita T0
       TimerClockSourceSet(TIMER1_BASE, TIMER_CLOCK_SYSTEM);   //T0 a 120MHz
       TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);    //T0 periodico y conjunto (32b)
       TimerLoadSet(TIMER1_BASE, TIMER_A, (RELOJ/8) -1);  //125ms
       TimerIntRegister(TIMER1_BASE,TIMER_A,IntTimer1);
       IntEnable(INT_TIMER1A); //Habilitar las interrupciones globales de los timers
       TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);    // Habilitar las interrupciones de timeout
       IntMasterEnable();  //Habilitacion global de interrupciones
       TimerEnable(TIMER1_BASE, TIMER_A);  //Habilitar Timer0

    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOG);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOJ);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_PWM0);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_TIMER0);       //Habilita T0
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_TIMER1);
    SysCtlPeripheralClockGating(true);

	while(1){
	    SLEEP;
	    switch (estado){
	    case 0:
	        posicion =barrera_abajo;
	        enciende_leds(estado);

	        if(B1_ON && B2_OFF){
	            estado=1;
	        }
	        if(B1_OFF && B2_ON){
	            estado=2;
	        }
	        if(B1_ON && B2_ON){
	            estado=2;
	        }
	        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, posicion);

	        break;
	    case 1:



	    enciende_leds(estado);
	    estado=3;
	    break;

	    case 2:
	        enciende_leds(estado);
	        estado = 4;
	        break;
	    case 3:
	        enciende_leds(estado);
	        posicion +=10;

	        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, posicion);
	        if (posicion >= barrera_arriba || B2_ON){
	            posicion = barrera_arriba;
	            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, posicion);
	            estado = 5 ;
	        }
	        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, posicion);
	        break;
	    case 4:
	        enciende_leds(estado);
	        posicion +=10;

	        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, posicion);
	        if (posicion >= barrera_arriba || B1_ON){
	            posicion = barrera_arriba;
	                    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, posicion);
	            estado = 5 ;
	        }
	        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, posicion);

	        break;
	    case 5:
	        enciende_leds(estado);
	        if (B1_OFF){
	            estado=7;
	        }
	        break;
	    case 6:
	        enciende_leds(estado);
	        if (B2_OFF){
	                       estado=8;
	                   }

	        break;
	    case 7:
	        enciende_leds(estado);
	        if (B2_OFF){
	            estado=9;
	        }
	        break;
	    case 8:
	        enciende_leds(estado);
	        if (B1_OFF){
	            estado=9;
	        }
	        break;
	    case 9:
	        enciende_leds(estado);
	        posicion -=10;
	        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, posicion);
	        if(posicion <=barrera_abajo){
	            posicion= barrera_abajo;
	            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, posicion);
	            estado=0;
	        }
	        break;



	    }

//		if(B1_ON){	//Si se pulsa B1
//			if(pos_100<100) pos_100++;	//Incrementa el periodo, saturando
//		}
//		if(B2_ON){	//Si se pulsa B2
//			if(pos_100>0) pos_100--;//Decrementa el periodo, saturando
//		}
//		posicion=Min_pos+((barrera_arriba-Min_pos)*pos_100)/100;
//        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, posicion);
//
	    //	    GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, GPIO_PIN_1*LED[Est][0]);
	    //	        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, GPIO_PIN_0*LED[Est][1]);
	    //	        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_PIN_4*LED[Est][2]);
	    //	        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_PIN_0*LED[Est][3]);

}
}

void IntTimer0(void)
{
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT); // Borra flag
    SysCtlDelay(20); //Retraso necesario. Mirar Driverlib p.550
    Flag_ints++;
}
void IntTimer1(void)
{
    TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT); // Borra flag
    SysCtlDelay(20); //Retraso necesario. Mirar Driverlib p.550
    if (estado == 1 || estado == 3 || estado == 5 || estado == 7 ) {  // Coche entrando
            toggle_L2 = !toggle_L2;
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, toggle_L2 ? GPIO_PIN_4 : 0);
        }
        else if (estado == 2 || estado == 4 || estado == 6 || estado == 8  ) {  // Coche saliendo
            toggle_L3 = !toggle_L3;
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, toggle_L3 ? GPIO_PIN_0 : 0);
        }
        else {
            // En otros estados, aseguramos LEDs según tabla
            enciende_leds(estado);
            toggle_L2 = false;
            toggle_L3 = false;
        }
}


