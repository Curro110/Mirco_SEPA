/*
 * main.c
 */

#include <stdint.h>
#include <stdbool.h>
#include "driverlib2.h"
#include "utils/uartstdio.h"
#include "FT800_TIVA.h"


// =======================================================================
// Function Declarations
// =======================================================================
#define dword long
#define byte char

#define LED1_ON  GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, GPIO_PIN_1)
#define LED1_OFF GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 0)
#define LED2_ON  GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, GPIO_PIN_0)
#define LED2_OFF GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, 0)
#define LED3_ON  GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_PIN_4)
#define LED3_OFF GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, 0)
#define LED4_ON  GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_PIN_0)
#define LED4_OFF GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, 0)



#define XpMax 300
#define XpMin 210
#define YpMax 200
#define YpMin 40

#define B1_OFF GPIOPinRead(GPIO_PORTJ_BASE,GPIO_PIN_0)
#define B1_ON !(GPIOPinRead(GPIO_PORTJ_BASE,GPIO_PIN_0))
#define B2_OFF GPIOPinRead(GPIO_PORTJ_BASE,GPIO_PIN_1)
#define B2_ON !(GPIOPinRead(GPIO_PORTJ_BASE,GPIO_PIN_1))



//#define SLEEP SysCtlSleep()
#define SLEEP SysCtlSleepFake()

unsigned int  Xp=22;






enum estados {
 Inicio,
 Recarga,
 Mañana,
 Tarde,
 Noche,
 AvisoM,
 AvisoT,
 AvisoN,
 EsperoN

};

enum estados

unsigned long REG_TT[6];
const int32_t REG_CAL[6]= {CAL_DEFAULTS};


#define NUM_SSI_DATA            3

int RELOJ, PeriodoPWM;
char led1=0, led2=0;
volatile int Flag_ints=0;
volatile int posicion;
volatile int Max_pos = 4200; //3750
volatile int Min_pos = 1300; //1875
volatile int pos_100, pos_1000;

int dias,horas, minutos, segundos;
char hora[9]; //Variable para guardar la hora en texto
int p; //Contador para contar segundos

uint32_t temporizador=0;
int m = 0;
int n;








void flushUART(void) // funcion para eliminar en el buffer todo lo que se haya podido escribir por UART cuando no era pedido
{
    while(UARTCharsAvail(UART0_BASE))
        UARTCharGetNonBlocking(UART0_BASE); // descarta
}
void UART_SendString(char *str);
int  UART_ReadInt(void);
void set_color_verde(void){ComColor(0,255,0);}
void set_color_azul(void){ ComColor(0,0,255); }
void set_color_blanco(void){ ComColor(255,255,255); }
void set_color_rojo(void){ ComColor(255,0,0); }
void set_color_negro(void){ComColor(0,0,0);}
void SysCtlSleepFake(void)
{
    while(!Flag_ints);
    Flag_ints=0;
}
void UART_SendString(char *str) {
    while (*str) UARTCharPut(UART0_BASE, *str++);
}

int UART_ReadInt(void) {
    char buffer[5];
    int i = 0;
    while (1) {
        char c = UARTCharGet(UART0_BASE);
        if (c == '\r' || c == '\n') break;
        buffer[i++] = c;
    }
    buffer[i] = '\0';
    return atoi(buffer);
}
void IntTimer0(void);

int main(void) {


    int i;

    RELOJ=SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480), 120000000);

    HAL_Init_SPI(1, RELOJ);  //Boosterpack a usar, Velocidad del MC

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0 |GPIO_PIN_1);

    //Habilitar los periféricos implicados: GPIOF, J, N
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);

    PWMClockSet(PWM0_BASE,PWM_SYSCLK_DIV_64);   // al PWM le llega un reloj de 1.875MHz

    GPIOPinConfigure(GPIO_PF1_M0PWM1);
    GPIOPinConfigure(GPIO_PF2_M0PWM2);
    GPIOPinConfigure(GPIO_PF3_M0PWM3);
    GPIOPinConfigure(GPIO_PG0_M0PWM4);


    //Configurar el pin a PWM
    GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 );
    GPIOPinTypePWM(GPIO_PORTG_BASE, GPIO_PIN_0);

    //Configurar el pwm0, contador descendente y sin sincronización (actualización automática)
    PWMGenConfigure(PWM0_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
    PWMGenConfigure(PWM0_BASE, PWM_GEN_1, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
    PWMGenConfigure(PWM0_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);

    PeriodoPWM=37499; // 50Hz  a 1.875MHz
    posicion=Min_pos;
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, PeriodoPWM);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, PeriodoPWM);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, PeriodoPWM);
                                                          //frec:50Hz
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, posicion);   //Inicialmente, 1ms
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, posicion);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, posicion);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, posicion);

    PWMGenEnable(PWM0_BASE, PWM_GEN_0);
    PWMGenEnable(PWM0_BASE, PWM_GEN_1);
    PWMGenEnable(PWM0_BASE, PWM_GEN_2); //Habilita el generador 0

    PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT | PWM_OUT_2_BIT | PWM_OUT_3_BIT | PWM_OUT_4_BIT, true);   //Habilita la salida 1

    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_0 |GPIO_PIN_4); //F0 y F4: salidas
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0 |GPIO_PIN_1); //N0 y N1: salidas
    GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_0|GPIO_PIN_1); //J0 y J1: entradas
    GPIOPadConfigSet(GPIO_PORTJ_BASE,GPIO_PIN_0|GPIO_PIN_1,GPIO_STRENGTH_2MA,GPIO_PIN_TYPE_STD_WPU);

    // Timer T0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);       //Habilita T0
    TimerClockSourceSet(TIMER0_BASE, TIMER_CLOCK_SYSTEM);   //T0 a 120MHz
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);    //T0 periodico y conjunto (32b)
    TimerLoadSet(TIMER0_BASE, TIMER_A, (RELOJ/50) -1);  //20ms
    TimerIntRegister(TIMER0_BASE,TIMER_A,IntTimer0);
    IntEnable(INT_TIMER0A); //Habilitar las interrupciones globales de los timers
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);    // Habilitar las interrupciones de timeout
    IntMasterEnable();  //Habilitacion global de interrupciones
    TimerEnable(TIMER0_BASE, TIMER_A);  //Habilitar Timer0

    //Habilitar UART
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    UARTStdioConfig(0, 115200, RELOJ);

    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOJ);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_TIMER0);       //Habilita T0
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPION);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_PWM0);

    SysCtlPeripheralClockGating(true);


    Inicia_pantalla();
    // Note: Keep SPI below 11MHz here

    // =======================================================================
    // Delay before we begin to display anything
    // =======================================================================

    SysCtlDelay(RELOJ/3);

    // ================================================================================================================
    // PANTALLA INICIAL
    // ================================================================================================================

    Nueva_pantalla(0x10,0x10,0x10);

    //FONDO
    set_color_azul();
    ComRect(0,0, HSIZE, VSIZE, true);


    //DIA DE LA SEMANA
    set_color_blanco();
    ComRect(53,30,265,90,true);

    //BOTONES
    set_color_blanco();
   ComRect (20,140,96,220,true); //boton mañana
    set_color_negro();
   ComTXT(53,180,26,OPT_CENTER,"MANANA");

   set_color_blanco();
    ComRect (116,140,192,220,true); //boton tarde
     set_color_negro();
    ComTXT(149,180,26,OPT_CENTER,"TARDE");

    set_color_blanco();
     ComRect (212,140,288,220,true); //boton noche
      set_color_negro();
     ComTXT(245,180,26,OPT_CENTER,"NOCHE");

    Dibuja();

    for(i=0;i<6;i++)    Esc_Reg(REG_TOUCH_TRANSFORM_A+4*i, REG_CAL[i]);
    estado= Inicio;

    char *dias_semana[] = {"Lunes","Martes","Miercoles","Jueves","Viernes","Sabado","Domingo"};

    while(1)
    {


        bool PMañana = (horas >= 8  && horas < 10);
        bool PTarde  = (horas >= 14 && horas < 16);
        bool PNoche  = (horas >= 20 && horas < 22);


        switch(estado){

        case Inicio:
            UART_SendString("Introduce el dia (0-6): ");
            dias = UART_ReadInt();
            UART_SendString("\r\nIntroduce la hora (0-23): ");
            horas = UART_ReadInt();
            UART_SendString("\r\nIntroduce los minutos (0-59): ");
            minutos = UART_ReadInt();
            UART_SendString("\r\nDatos recibidos. Pasando a recarga...\r\n");
            estado = Recarga;
            break;
        case Recarga:
            set_color_negro();

              ComTXT(160,60,28,OPT_CENTER, dias_semana[dias]);
            posicion=Max_pos;
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, posicion); // Mueve un poco el servo
            SysCtlDelay(SysCtlClockGet() / 3); // espera 1 seg
            Servo_Mover4(0);
            // Decide siguiente estado según hora
            if (horas >= 4 && horas < 12)
                estado = Mañana;
            else if (horas >= 12 && horas < 18)
                estado = Tarde;
            else
                estado = Noche;
            break;

        case Mañana:
            if (esTarde) set_color_verde();
            else set_color_blanco();
            ComRect(20,140,96,220,true);
            set_color_negro();
            ComTXT(53,180,26,OPT_CENTER,"MANANA");

            if (horas >= 12 && horas < 18) estado = Tarde;
            if (horas >= 18 || horas < 4) estado = Noche;
            break;
        case Tarde:
            if (horas >= 18 || horas < 4) estado = Noche;
                            if (horas >= 4 && horas < 12) estado = Mañana;
            break;
        case Noche:
            if (horas >= 4 && horas < 12) estado = Mañana;
                          if (horas >= 12 && horas < 18) estado = Tarde;
            break;
        case AvisoM:
            break;
        case AvisoT:
            break;
        case AvisoN:
            break;
        case EsperoN:
            break;

        }


        //posicion= Max_pos;
//        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, posicion);
//        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, posicion);
//        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, posicion);
//        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, posicion);
        Dibuja();
    }
}

void IntTimer0(void)
{
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT); // Borra flag
    SysCtlDelay(20); //Retraso necesario.
    Flag_ints++;

    p++;
    if (p >= 50) {
        p = 0;
        segundos++;

        if (segundos >= 60) {
            segundos = 0;
            minutos++;
            if (minutos >= 60) {
                minutos = 0;
                horas++;
                if (horas >= 24){
                    horas = 0;
                    dias++;
                    if (dias >= 7)
                        dias=0;
                }
            }
        }
    }
}






