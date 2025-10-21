#include <stdint.h>
#include <stdbool.h>
#include "driverlib2.h"

#include "FT800_TIVA.h"


// =======================================================================
// Function Declarations
// =======================================================================
#define dword long
#define byte char

#define B1_OFF GPIOPinRead(GPIO_PORTJ_BASE,GPIO_PIN_0)
#define B1_ON !(GPIOPinRead(GPIO_PORTJ_BASE,GPIO_PIN_0))
#define B2_OFF GPIOPinRead(GPIO_PORTJ_BASE,GPIO_PIN_1)
#define B2_ON !(GPIOPinRead(GPIO_PORTJ_BASE,GPIO_PIN_1))

#define PosMin 750
#define PosMax 1000

#define XpMax 286
#define XpMin 224
#define YpMax 186
#define YpMin 54

unsigned int Yp=120, Xp=245;

unsigned long REG_TT[6];
const int32_t REG_CAL[6]= {CAL_DEFAULTS};


volatile int Flag_ints=0;
//void enciende_leds(uint8_t Est){
//    GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, GPIO_PIN_1*LED[Est][0]);
//    GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, GPIO_PIN_0*LED[Est][1]);
//    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_PIN_4*LED[Est][2]);
//    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_PIN_0*LED[Est][3]);
//}
void SysCtlSleepFake(void)
{
    while(!Flag_ints);
    Flag_ints=0;
}

//#define SLEEP SysCtlSleep()
#define SLEEP SysCtlSleepFake()

#define NUM_SSI_DATA            3

int RELOJ;
#define FREC 20 //Frecuencia en hercios del tren de pulsos: 20ms

char Fin=0;

void IntTimer(void)
{
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    SysCtlDelay(20); //Retraso necesario. Mirar Driverlib p.550
    Flag_ints++;
}

int Load;   // Variable para comprobar el % de carga del ciclo de trabajo

int main(void)
 {


    int i;

    RELOJ=SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480), 120000000);
    HAL_Init_SPI(2, RELOJ);  //Boosterpack a usar, Velocidad del MC
    Inicia_pantalla();       //Arranque de la pantalla

    // Note: Keep SPI below 11MHz here

    // =======================================================================
    // Delay before we begin to display anything
    // =======================================================================

    SysCtlDelay(RELOJ/3);

    // ================================================================================================================
    // PANTALLA INICIAL
    // ================================================================================================================
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);       //Habilita T0

    TimerClockSourceSet(TIMER0_BASE, TIMER_CLOCK_SYSTEM);   //T0 a 120MHz
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);    //T0 periodico y conjunto (32b)
    TimerLoadSet(TIMER0_BASE, TIMER_A, (RELOJ/FREC)-1);
    TimerIntRegister(TIMER0_BASE,TIMER_A,IntTimer);


    IntEnable(INT_TIMER0A); //Habilitar las interrupciones globales de los timers
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);    // Habilitar las interrupciones de timeout
    IntMasterEnable();  //Habilitacion global de interrupciones
    TimerEnable(TIMER0_BASE, TIMER_A);  //Habilitar Timer0, 1, 2A y 2B

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);

    GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_0|GPIO_PIN_1);
    GPIOPadConfigSet(GPIO_PORTJ_BASE,GPIO_PIN_0|GPIO_PIN_1,GPIO_STRENGTH_2MA,GPIO_PIN_TYPE_STD_WPU);

    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_0 |GPIO_PIN_4);    //F0 y F4: salidas
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0 |GPIO_PIN_1); //N0 y N1: salidas
// quitar la linea de codigo esta y poner la del ejemplo 1_1
    Nueva_pantalla(16,16,16);
    ComColor(21,160,6);
    ComLineWidth(5);
    ComRect(10, 10, HSIZE-10, VSIZE-10, true);
    ComColor(65,202,42);
    ComRect(12, 12, HSIZE-12, VSIZE-12, true);//las isntrucciones siguen hasta que no se cambia las propiedasdes

    ComColor(255,255,255);
    ComTXT(HSIZE/2,VSIZE/5, 22, OPT_CENTERX,"EJEMPLO de PANTALLA");
    ComTXT(HSIZE/2,50+VSIZE/5, 22, OPT_CENTERX," SEPA GIERM. 2024 ");
    ComTXT(HSIZE/2,100+VSIZE/5, 20, OPT_CENTERX,"M.A.P.E.");

    ComRect(40,40, HSIZE-40, VSIZE-40, false);



    Dibuja();
    Espera_pant();

// mete los parametros definidos de la calibracion
    for(i=0;i<6;i++)    Esc_Reg(REG_TOUCH_TRANSFORM_A+4*i, REG_CAL[i]);



    while(1){
        SLEEP;
        Lee_pantalla();

        Nueva_pantalla(16,16,16);

        ComGradient(0,0,GRIS_CLARO,0,240,GRIS_OSCURO);
        ComColor(255,0,0);
        ComFgcolor(200, 200, 10);

//         tengo que hacer lo siguiente primero hacer una maquina de estados
                if(Boton(10, 90, 50, 50, 28, "L1"))
                {
                    Xp--; if (Xp<=XpMin)  Xp=XpMin;
                }

                if(Boton(130, 90, 50, 50, 28, "L2"))
                {
                    Xp++; if (Xp>=XpMax) Xp=XpMax;
                }



                if(Boton(70, 30, 50, 50, 28, "L3"))
                {
                    Yp--; if (Yp<=YpMin) Yp=YpMin;
                }
                if(Boton(70, 150, 50, 50, 28, "L4"))
                {
                    Yp++; if (Yp>=YpMax) Yp=YpMax;
                }

                if(POSX>XpMin && POSX<XpMax && POSY>YpMin && POSY<YpMax) //Pulsando en el rectangulo
                {
                    Xp=POSX;    //Llevar la bola a la posición
                    Yp=POSY;
                    ComColor(0xff,0xE0,0xE0);   //Fondo rosa

                }
                else
                {
                    ComColor(0xff,0xff,0xff);   //Fondo blanco
                }

                ComRect(200, 30, 310, 210, true);
                ComColor(0x30,0x50,0x10);
                ComLineWidth(3);

                ComRect(200, 30, 310, 210, false);
                ComCirculo(Xp, Yp, 20);

        Dibuja();
        Load=100 - (100*TimerValueGet(TIMER0_BASE,TIMER_A))/((RELOJ/FREC)-1); // comprobacion que esta afuera del tiempo de reloj
    }

}



