/*
 * main.c
 */

#include <stdint.h>
#include <stdbool.h>
#include "driverlib2.h"

#include "FT800_TIVA.h"


// =======================================================================
// Function Declarations
// =======================================================================
#define dword long
#define byte char



#define PosMin 750
#define PosMax 1000

#define XpMax 300
#define XpMin 210
#define YpMax 200
#define YpMin 40

unsigned int Yp=120, Xp=245;

unsigned long REG_TT[6];
const int32_t REG_CAL[6]= {CAL_DEFAULTS};

#define NUM_SSI_DATA            3

int RELOJ;
char led1=0, led2=0;


int main(void) {


    int i;
    int NoFin=1;

    RELOJ=SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480), 120000000);

    HAL_Init_SPI(2, RELOJ);  //Boosterpack a usar, Velocidad del MC

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0 |GPIO_PIN_1);




    Inicia_pantalla();
    // Note: Keep SPI below 11MHz here

    // =======================================================================
    // Delay before we begin to display anything
    // =======================================================================

    SysCtlDelay(RELOJ/3);

    // ================================================================================================================
    // PANTALLA INICIAL
    // ================================================================================================================

    while(1)
    {
        NoFin=1;
        Nueva_pantalla(0x10,0x10,0x10);


        Nueva_pantalla(16,16,16);
        ComColor(21,160,6);
        ComLineWidth(5);
        ComRect(10, 10, HSIZE-10, VSIZE-10, true);
        ComColor(65,202,42);
        ComRect(12, 12, HSIZE-12, VSIZE-12, true);
        ComColor(255,255,255);

        ComTXT(HSIZE/2,VSIZE/5, 22, OPT_CENTERX,"CALIBRACION de PANTALLA");
        ComTXT(HSIZE/2,50+VSIZE/5, 22, OPT_CENTERX," SEPA GIERM. 2021 ");
        ComTXT(HSIZE/2,100+VSIZE/5, 20, OPT_CENTERX,"M.A.P.E.");

        ComRect(40,40, HSIZE-40, VSIZE-40, false);

        Dibuja();
        Espera_pant();

        SysCtlDelay(RELOJ/6);

        Nueva_pantalla(0xf0,0xf0,0x10);


        ComTXT(60,30,27,0,"Pulsa en el punto");
        EscribeRam32(CMD_CALIBRATE);

        Dibuja();

        Nueva_pantalla(0,0,0);
        for(i=0;i<6;i++)    REG_TT[i]=Lee_Reg(REG_TOUCH_TRANSFORM_A+4*i);
        ComTXT(20,20,27,0, " PARAMETROS: ");

        for(i=0;i<6;i++) {
            ComNum(50,50+25*i,22,OPT_SIGNED,REG_TT[i]);
        }
        ComTXT(20,210,22,0,"Apunta estos valores. Pulsa la pantalla");
        Dibuja();

        Espera_pant();

        ComFgcolor(0xff,0,0xff);
        while(NoFin){
            Nueva_pantalla(0,0,0xf7);

            Lee_pantalla();
            if(POSY!=0x8000){
                ComCirculo(POSX, POSY, 10);
                if(POSX>(HSIZE-20) && POSY >(VSIZE-20)) NoFin=0;
            }
            Dibuja();
        }
    }
}









