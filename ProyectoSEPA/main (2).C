/*
 * main.c
 */

#include <stdint.h>
#include <stdbool.h>
#include "driverlib2.h"
#include "utils/uartstdio.h"
#include "FT800_TIVA.h"

#include <stdlib.h> // Para atoi()
#include <string.h> // Para strcmp()
#include <ctype.h> // Para toupper()
#include <stdio.h> // Para sprintf()


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

#define ESP1_ON  GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_7, 1)// aqui estan los defines para que sea mas faciles para poner que se encienda los pines que comunican con la esp
#define ESP1_OFF GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_7, 0)
#define ESP2_ON  GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_6, 1)
#define ESP2_OFF GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_6, 0)
#define ESP3_ON  GPIOPinWrite(GPIO_PORTH_BASE, GPIO_PIN_1, 1)
#define ESP3_OFF GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_1, 0)

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




//----------------------------------Estados---------------------------------//

enum estados {
 Inicio, //Estado en el que el usuario introduce el dia y la hora actual

 Dia1, //Se introduce primera letra del dia de la semana
 Dia2,//Se introduce segunda letra del dia de la semana
 Dia3,//Se introduce tercera letra del dia de la semana

 Hora1, //Se introduce primer digito de la hora
 Hora2, //Se introduce segundo digito de la hora

 Min1, //Se introduce primer digito de los minutos
 Min2,//Se introduce segundo digito de los minutos

 Verificacion_dia, //Verificar si el dia es correcto
 Verificacion_hora,//Verificar si la hora es correcta
 Verificacion_min,//Verificar si el minuto es correcto

 Reposo1,//El usuario ha tomado la pastilla de la manana?
 Reposo2,//El usuario ha tomado la pastilla de la tarde?
 Reposo3,//El usuario ha tomado la pastilla de la noche?

MananaOK,// Se ha tomado la primera pastilla del dia
MananaNO,// No se ha tomado la primera pastilla del dia


TardeOK,// Se ha tomado la segunda pastilla del dia
TardeNO,// No se ha tomado la segunda pastilla del dia


NocheOK,// Se ha tomado la tercera pastilla del dia
NocheNO,// No se ha tomado la tercera pastilla del dia

Inicio_i,//Pantalla de inicio

Manana,//Pantalla para abrir manualmente el pastillero o poner alarma de la mañana
ConfigHora1,//Pantalla para configurar la alarma de mañana
AbrirManana, //Abrir pastillero de manera manual en caso de que no se haya tomado la pastilla

Tarde,//Pantalla para abrir manualmente el pastillero o poner alarma de la tarde
ConfigHora2,//Pantalla para configurar la alarma de tarde
AbrirTarde,//Abrir pastillero de manera manual en caso de que no se haya tomado la pastilla

Noche,//Pantalla para abrir manualmente el pastillero o poner alarma de la noche
ConfigHora3,//Pantalla para configurar la alarma de noche
AbrirNoche,//Abrir pastillero de manera manual en caso de que no se haya tomado la pastilla

AlarmaManana,//Alarma de la manana suena
AlarmaTarde,//Alarma de la tarde suena
AlarmaNoche//Alarma de la noche suena
};

enum estados estado_actual, estado_anterior;



//----------------------------------Variables---------------------------------//

const char *semana[]= {"LUN","MAR","MIE","JUE","VIE","SAB","DOM"}; //Dias de la semana que puede introducir el usuario
char *semana_completa[] = {"Lunes","Martes","Miercoles","Jueves","Viernes","Sabado","Domingo"};
char dia[4];//variable donde se registra el dia por el usuario
char hora[3];//variable donde se registra la hora por el usuario
char minuto[3];//variable donde se registra el minuto por el usuario
char buffer[10]; //Variable para guardar la hora en el que el usuario se ha tomado una pastilla


char c;//variable que registra lo que guarda el usuario
int h;//variable que registra el valor numerico de la hora introducida
int m;//variable que registra el valor numerico del minuto introducido

int dias,horas, minutos, segundos; //hora calculada por el micro
int horas_manana=0, minutos_manana=0;//variables para modificar la hora en la que se ha tomado la primera pastilla
int horas_tarde=12, minutos_tarde=0;//variables para modificar la hora en la que se ha tomado la segunda pastilla
int minutos_tarde_ant=0, minutos_manana_ant=0, minutos_noche_ant=0;
int horas_noche=18, minutos_noche=0;//variables para modificar la hora en la que se ha tomado la tercera pastilla
int alarma_manana=7, alarma_tarde=15, alarma_noche=23;//Alarmas predeterminadas cada 8h--> a modificar por el usuario segun la pastilla que necesite
char texto_hora_manana[10], texto_hora_tarde[10], texto_hora_noche[10];//variable para escribir la alarma predeterminada
char buffer2[9]; //Variable que imprime la hora en la pantalla
int p; //Contador para contar segundos
int espera; //contador para esperar que el usuario coja la pastilla
int tiempo_inicio; //variable para controlar cuanto tiempo se mantiene el pastillero abierto
int tiempo_mensaje;//variable para marcar cuando se envia un mensaje por whatsapp al familiar

int no_manana=0, no_tarde=0, no_noche=0; // variable que se pone a 1 si no se ha tomado la pastilla del dia

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
int parp=0;//parpadeo
int Periodo_parp;

uint32_t temporizador=0;
int m = 0;
int r_manana=1,g_manana=1,b_manana=1; //variables para cambiar el color de los botones de las alarmas
int r_tarde=1,g_tarde=1,b_tarde=1; //variables para cambiar el color de los botones de las alarmas
int r_noche=1,g_noche=1,b_noche=1; //variables para cambiar el color de los botones de las alarmas




//----------------------------------Funciones---------------------------------//

//Colores//
void set_color_verde(void){ComColor(0,255,0);}
void set_color_azul(void){ ComColor(0,0,255); }
void set_color_blanco(void){ ComColor(255,255,255); }
void set_color_rojo(void){ ComColor(255,0,0); }
void set_color_negro(void){ComColor(0,0,0);}

//Funcion pra verificar el dia de la semana
int idx;//variable que guarda lo que devuelva la funcion validar_dia
int j;
int validar_dia(char *s)
{
    char tmp[4];

    tmp[0] = toupper((unsigned char)s[0]);
    tmp[1] = toupper((unsigned char)s[1]);
    tmp[2] = toupper((unsigned char)s[2]);
    tmp[3] = '\0';
    for (j = 0; j < 7; j++) {
        if (strcmp(tmp, semana[j]) == 0) return j;
    }
    return -1;
}

//Funcion para eliminar en el buffer todo lo que se haya podido escribir por UART cuando no era pedido
void flushUART(void)
{
    while(UARTCharsAvail(UART0_BASE))
        UARTCharGetNonBlocking(UART0_BASE); // descarta
}

// Funcion que sale del modo de SLEEP de forma forzada
void SysCtlSleepFake(void)
{
    while(!Flag_ints);
    Flag_ints=0;
}

//Temporizador
void IntTimer0(void);
void IntTimer1(void);

int main(void) {


    int i;

    RELOJ=SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480), 120000000);

    HAL_Init_SPI(1, RELOJ);  //Boosterpack a usar, Velocidad del MC

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0 |GPIO_PIN_1);

    //Habilitar los perifericos implicados: GPIOF, J, N
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOH);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);

    PWMClockSet(PWM0_BASE,PWM_SYSCLK_DIV_64);   // al PWM le llega un reloj de 1.875MHz

    GPIOPinConfigure(GPIO_PF1_M0PWM1);
    GPIOPinConfigure(GPIO_PF2_M0PWM2);
    GPIOPinConfigure(GPIO_PF3_M0PWM3);
    GPIOPinConfigure(GPIO_PG0_M0PWM4);


    //Configurar el pin a PWM
    GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 );
    GPIOPinTypePWM(GPIO_PORTG_BASE, GPIO_PIN_0);

    //Configurar el pwm0, contador descendente y sin sincronizacion (actualizacion automatica)
    PWMGenConfigure(PWM0_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
    PWMGenConfigure(PWM0_BASE, PWM_GEN_1, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
    PWMGenConfigure(PWM0_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);

    PeriodoPWM=37499; // 50Hz  a 1.875MHz
    posicion=Min_pos;
    Periodo_parp=14999999; // 4 Hz
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

    GPIOPinTypeGPIOOutput(GPIO_PORTK_BASE, GPIO_PIN_7 |GPIO_PIN_6); //K6 y K7: salidas en la placa son los pines de abajo de la derecha los interirores el primero y segundo por abajo
    GPIOPinTypeGPIOOutput(GPIO_PORTH_BASE, GPIO_PIN_1 ); // justo el de arriba a estos dos
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

    // Timer 1
       SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1); // Habilita T1
       TimerClockSourceSet(TIMER1_BASE, TIMER_CLOCK_SYSTEM);       // T1 a 120MHz
       TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);   //T1 periodico y conjunto (32b)
       TimerLoadSet(TIMER1_BASE, TIMER_A, Periodo_parp); //125 ms
       TimerIntRegister(TIMER1_BASE, TIMER_A, IntTimer1);
       IntEnable(INT_TIMER1A);    //Habilitar las interrupciones globales de los timers
       TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);    // Habilitar las interrupciones de timeout
       IntMasterEnable();//Habilitacion global de interrupciones
       TimerEnable(TIMER1_BASE, TIMER_A);      //Habilitar Timer1

    //Habilitar UART
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    UARTStdioConfig(0, 115200, RELOJ);

    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOK);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOH);// estas dos lineas de codigo habilita las salidas a la esp
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOJ);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_TIMER0);       //Habilita T0
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_TIMER1);       //Habilita T1
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


    for(i=0;i<6;i++)    Esc_Reg(REG_TOUCH_TRANSFORM_A+4*i, REG_CAL[i]);

    estado_actual= Inicio;


    while(1)
    {
        SLEEP;

        switch(estado_actual){

        case Inicio:
            //Se debe primero introducir el dia y la hora correcta
            Nueva_pantalla(0x10,0x10,0x10);

            //FONDO
            set_color_azul();
            ComRect(0,0, HSIZE, VSIZE, true);
            set_color_blanco();
            ComTXT(HSIZE/2,VSIZE/2,26,OPT_CENTER,"Introduce Dia, Hora y Minuto en el ordenador");

            Dibuja();

            UARTprintf("\nIntroduce el dia actual (LUN-DOM): \n");
            flushUART(); // descartamos input previo
            estado_actual=Dia1;

            break;


            //--------------------------------------Usuario introduce dia-----------------------------------------------//

        case Dia1:

            if(UARTCharsAvail(UART0_BASE))
            {
                c = UARTCharGetNonBlocking(UART0_BASE);
                dia[0] = c;
                UARTCharPutNonBlocking(UART0_BASE, c);
                estado_actual = Dia2;
            }

            break;

        case Dia2:
            if(UARTCharsAvail(UART0_BASE))
            {
                c = UARTCharGetNonBlocking(UART0_BASE);
                if (c == 0x08) {
                    // borrar el caracter anterior
                    dia[0] = '\0';
                    UARTprintf("\b \b");  // borrar en pantalla
                    estado_actual = Dia1;
                }
                else {
                    dia[1] = c;
                    UARTCharPutNonBlocking(UART0_BASE, c);
                    estado_actual = Dia3;
                }
            }

            break;

        case Dia3:
            if(UARTCharsAvail(UART0_BASE))
            {
                c = UARTCharGetNonBlocking(UART0_BASE);
                if (c == 0x08) {
                    dia[1] = '\0';
                    UARTprintf("\b \b");
                    estado_actual = Dia2;
                }
                else {
                    dia[2] = c;
                    dia[3] = '\0';
                    UARTCharPutNonBlocking(UART0_BASE, c);
                    UARTCharPut(UART0_BASE, '\n');
                    estado_actual = Verificacion_dia;
                }

            }

            break;

        case Verificacion_dia:
            idx = validar_dia(dia);
            if (idx >= 0)
            { // dia correcto

                UARTprintf("Dia valido: %s\n", semana_completa[idx]);
                UARTprintf("Introuzca la hora del dia (1-24):\n");
                estado_actual = Hora1;
            }
            else
            {// dia no valido

                UARTprintf("Error: dia no valido (se recibio '%c%c%c')\n", dia[0], dia[1], dia[2]);
                UARTprintf("Introducir de nuevo\n");
                estado_actual = Inicio;
            }
            break;


            //--------------------------------------Usuario introduce hora-----------------------------------------------//

        case Hora1:
            if (UARTCharsAvail(UART0_BASE)) {
                c = UARTCharGetNonBlocking(UART0_BASE);
                if (c == 0x08) {
                    // No hay nada que borrar
                }
                else if (c >= '0' && c <= '9') {
                    hora[0] = c;
                    UARTCharPutNonBlocking(UART0_BASE, c);
                    estado_actual = Hora2;
                }
                else {
                    UARTprintf("Error: solo se permiten números.\n");
                    UARTprintf("Introducir de nuevo:\n");
                    estado_actual = Hora1;
                }
            }
            break;

        case Hora2:
            if (UARTCharsAvail(UART0_BASE)) {
                c = UARTCharGetNonBlocking(UART0_BASE);

                if (c == 0x08) {
                    // Borrar primer dígito
                    hora[0] = '\0';
                    UARTprintf("\b \b");
                    estado_actual = Hora1;
                }
                else if (c >= '0' && c <= '9') {
                    hora[1] = c;
                    hora[2] = '\0';
                    UARTCharPutNonBlocking(UART0_BASE, c);
                    UARTCharPut(UART0_BASE, '\n');
                    estado_actual = Verificacion_hora;
                }
                else {
                    UARTprintf("Error: solo se permiten números.\n");
                    UARTprintf("Introducir de nuevo:\n");
                    estado_actual = Hora1;
                }
            }
            break;

        case Verificacion_hora:
            h = atoi(hora); //se convierte texto a entero

            if (h >= 1 && h <= 24)
            {
                UARTprintf("Hora correcta: %d\n", h);
                UARTprintf("Introducir minuto(0-59): \n");
                horas=h;
                estado_actual = Min1; // siguiente paso
            }
            else
            {
                UARTprintf("Error: hora incorrecta\n");
                UARTprintf("Introducir de nuevo\n");
                estado_actual = Hora1;
            }

            break;


            //--------------------------------------Usuario introduce minutos-----------------------------------------------//

        case Min1:
            if (UARTCharsAvail(UART0_BASE)) {
                c = UARTCharGetNonBlocking(UART0_BASE);

                if (c == 0x08) {
                    // No hay nada que borrar aquí
                }
                else if (c >= '0' && c <= '9') {
                    minuto[0] = c;
                    UARTCharPutNonBlocking(UART0_BASE, c);
                    estado_actual = Min2;
                }
                else {
                    UARTprintf("Error: solo se permiten numeros.\n");
                    UARTprintf("Introducir de nuevo:\n");
                    estado_actual = Min1;
                }
            }
            break;

        case Min2:
            if (UARTCharsAvail(UART0_BASE)) {
                c = UARTCharGetNonBlocking(UART0_BASE);
                if (c == 0x08) {
                    // Borrar minuto[0]
                    minuto[0] = '\0';
                    UARTprintf("\b \b");
                    estado_actual = Min1;
                }
                else if (c >= '0' && c <= '9') {
                    minuto[1] = c;
                    minuto[2] = '\0';
                    UARTCharPutNonBlocking(UART0_BASE, c);
                    UARTCharPut(UART0_BASE, '\n');
                    estado_actual = Verificacion_min;
                }
                else {
                    UARTprintf("Error: solo se permiten numeros.\n");
                    UARTprintf("Introducir de nuevo:\n");
                    estado_actual = Min1;
                }
            }
            break;

        case Verificacion_min:
            m = atoi(minuto); //se convierte texto a entero

            if (m >= 0 && m <= 60)
            {
                UARTprintf("Minuto correcto: %d\n", m);
                minutos=m;
                if(horas>=6 && horas<12)estado_actual = Reposo1;
                if(horas>=12 && horas<=18)estado_actual = Reposo2;
                if(horas>=18 && horas<=24)estado_actual = Reposo3;

            }
            else
            {
                UARTprintf("\nError: minuto incorrecto\n");
                UARTprintf("Introducir de nuevo\n");
                estado_actual = Min1;
            }

            break;

            //--------------------------------------Usuario ha tomado primera pastilla?-----------------------------------------------//

        case Reposo1:

            Nueva_pantalla(0x10,0x10,0x10);

            //FONDO
            set_color_azul();
            ComRect(0,0, HSIZE, VSIZE, true);
            set_color_blanco();
            ComTXT(HSIZE/2,VSIZE/2,26,OPT_CENTER,"Ha tomado la primera pastilla del dia?");

            //BOTONES
            ComColor(0,0,0);
            ComFgcolor(0,255, 0);
            ComButton(65, 160, 40, 40, 22,OPT_FLAT, "SI");//boton SI
            ComFgcolor(255,0, 0);
            ComButton(170, 160, 40, 40, 22,OPT_FLAT, "NO");//boton NO

            Dibuja();

            if(Boton(65, 160, 40, 40, 22, "SI")) //Si aprieto boton SI
            {
                estado_actual=MananaOK;

            }

            if(Boton(170, 160, 40, 40, 22, "NO")) //Si aprieto boton NO
            {
                estado_actual=Inicio_i;
                segundos=0;
                r_manana=1;
                g_manana=0;
                b_manana=0;
                no_manana=1;

            }

            break;

        case MananaOK:
            Nueva_pantalla(0x10,0x10,0x10);

            //FONDO
            set_color_azul();
            ComRect(0,0, HSIZE, VSIZE, true);
            set_color_blanco();
            ComTXT(150,VSIZE/4,26,OPT_CENTER,"A que hora?");

            // ----- Etiquetas -----
            set_color_blanco();
            ComTXT(90, 80, 26, OPT_CENTER, "HORA");
            ComTXT(210, 80, 26, OPT_CENTER, "MINUTO");

            // ----- Triangulos arriba -----
            ComColor(0,0,0);
            ComFgcolor(0,0, 255);
            ComButton(75, 100, 30, 35, 22,OPT_FLAT, "^");   // â–² hora
            ComButton(195, 100, 30, 35, 22,OPT_FLAT, "^"); // â–² minuto

            // ----- Numeros -----
            set_color_blanco();
            ComRect(80,140, 100, 160, true);
            ComRect(200,140, 220, 160, true);
            set_color_negro();
            sprintf(buffer, "%02d", horas_manana);
            ComTXT(90, 150, 28, OPT_CENTER, buffer);
            sprintf(buffer, "%02d", minutos_manana);
            ComTXT(210, 150, 28, OPT_CENTER, buffer);

            // ----- Triangulos abajo -----
            ComColor(0,0,0);
            ComFgcolor(0,0, 255);
            ComButton(75, 170, 30, 35, 22,OPT_FLAT, "v");   // â–¼ hora
            ComButton(195, 170, 30, 35, 22,OPT_FLAT, "v"); // â–¼ minuto

            // ----- Boton OK -----
            ComColor(0,0,0);
            ComFgcolor(0,255, 0);
            ComButton(280, 200, 30, 30, 22,OPT_CENTER, "OK");

            Dibuja();
            horas_manana=6;
            if(Boton(75, 100, 30, 30, 22, "^"))//horas
            {
                if (horas_manana < 12)
                {
                    horas_manana++;
                }
            }
            if(Boton(195, 100, 30, 30, 22, "^"))//minutos
            {
                if (minutos_manana < 59)
                {
                    minutos_manana++;
                }
            }
            if(Boton(75, 170, 30, 30, 22, "v"))//horas
            {
                if (horas_manana >6)
                {
                    horas_manana--;
                }
            }
            if(Boton(195, 170, 30, 30, 22,"v"))//minutos
            {
                if (minutos_manana >0)
                {
                    minutos_manana--;
                }
            }
            if(Boton(280, 200, 30, 30, 22, "OK"))
            {
                estado_actual=Inicio_i;
                segundos=0;
                r_manana=0;
                g_manana=1;
                b_manana=0;
                alarma_tarde=horas_manana+8;
                minutos_tarde_ant=minutos_manana;
                alarma_noche=horas_manana+16;
                minutos_noche_ant=minutos_manana;
            }
            break;



            //--------------------------------------Usuario ha tomado segunda pastilla?-----------------------------------------------//

        case Reposo2:

            Nueva_pantalla(0x10,0x10,0x10);

            //FONDO
            set_color_azul();
            ComRect(0,0, HSIZE, VSIZE, true);
            set_color_blanco();
            ComTXT(HSIZE/2,VSIZE/2,26,OPT_CENTER,"Ha tomado la segunda pastilla del dia?");

            //BOTONES
            ComColor(0,0,0);
            ComFgcolor(0,255, 0);
            ComButton(65, 160, 40, 40, 22,OPT_FLAT, "SI");//boton SI
            ComFgcolor(255,0, 0);
            ComButton(170, 160, 40, 40, 22,OPT_FLAT, "NO");//boton NO

            Dibuja();

            if(Boton(65, 160, 40, 40, 22, "SI")) //Si aprieto boton SI
            {
                estado_actual=TardeOK;

            }

            if(Boton(170, 160, 40, 40, 22, "NO")) //Si aprieto boton NO
            {
                estado_actual=Inicio_i;
                segundos=0;
                r_tarde=1;
                g_tarde=0;
                b_tarde=0;
                no_tarde=1;

            }

            break;

        case TardeOK:
            Nueva_pantalla(0x10,0x10,0x10);

            //FONDO
            set_color_azul();
            ComRect(0,0, HSIZE, VSIZE, true);
            set_color_blanco();
            ComTXT(150,VSIZE/4,26,OPT_CENTER,"A que hora?");

            // ----- Etiquetas -----
            set_color_blanco();
            ComTXT(90, 80, 26, OPT_CENTER, "HORA");
            ComTXT(210, 80, 26, OPT_CENTER, "MINUTO");

            // ----- Triangulos arriba -----
            ComColor(0,0,0);
            ComFgcolor(0,0, 255);
            ComButton(75, 100, 30, 35, 22,OPT_FLAT, "^");   // â–² hora
            ComButton(195, 100, 30, 35, 22,OPT_FLAT, "^"); // â–² minuto

            // ----- Numeros -----
            set_color_blanco();
            ComRect(80,140, 100, 160, true);
            ComRect(200,140, 220, 160, true);
            set_color_negro();
            sprintf(buffer, "%02d", horas_tarde);
            ComTXT(90, 150, 28, OPT_CENTER, buffer);
            sprintf(buffer, "%02d", minutos_tarde);
            ComTXT(210, 150, 28, OPT_CENTER, buffer);

            // ----- Triangulos abajo -----
            ComColor(0,0,0);
            ComFgcolor(0,0, 255);
            ComButton(75, 170, 30, 35, 22,OPT_FLAT, "v");   // â–¼ hora
            ComButton(195, 170, 30, 35, 22,OPT_FLAT, "v"); // â–¼ minuto

            // ----- Boton OK -----
            ComColor(0,0,0);
            ComFgcolor(0,255, 0);
            ComButton(280, 200, 30, 30, 22,OPT_CENTER, "OK");

            Dibuja();

            if(Boton(75, 100, 30, 30, 22, "^"))//horas
            {
                if (horas_tarde < 18)
                {
                    horas_tarde++;
                }
            }
            if(Boton(195, 100, 30, 30, 22, "^"))//minutos
            {
                if (minutos_tarde < 59)
                {
                    minutos_tarde++;
                }
            }
            if(Boton(75, 170, 30, 30, 22, "v"))//horas
            {
                if (horas_tarde >12)
                {
                    horas_tarde--;
                }
            }
            if(Boton(195, 170, 30, 30, 22,"v"))//minutos
            {
                if (minutos_tarde >0)
                {
                    minutos_tarde--;
                }
            }
            if(Boton(280, 200, 30, 30, 22, "OK"))
            {
                estado_actual=Inicio_i;
                r_tarde=0;
                g_tarde=1;
                b_tarde=0;
                alarma_noche=horas_tarde+8;
                minutos_noche_ant=minutos_tarde;

            }
            break;



            //--------------------------------------Usuario ha tomado tercera pastilla?-----------------------------------------------//

        case Reposo3:

            Nueva_pantalla(0x10,0x10,0x10);

            //FONDO
            set_color_azul();
            ComRect(0,0, HSIZE, VSIZE, true);
            set_color_blanco();
            ComTXT(HSIZE/2,VSIZE/2,26,OPT_CENTER,"Ha tomado la tercera pastilla del dia?");

            //BOTONES
            ComColor(0,0,0);
            ComFgcolor(0,255, 0);
            ComButton(65, 160, 40, 40, 22,OPT_FLAT, "SI");//boton SI
            ComFgcolor(255,0, 0);
            ComButton(170, 160, 40, 40, 22,OPT_FLAT, "NO");//boton NO

            Dibuja();

            if(Boton(105, 160, 40, 40, 22, "SI")) //Si aprieto boton SI
            {
                estado_actual=NocheOK;

            }

            if(Boton(210, 160, 40, 40, 22, "NO")) //Si aprieto boton NO
            {
                estado_actual=Inicio_i;
                segundos=0;
                r_noche=1;
                g_noche=0;
                b_noche=0;
                no_noche=1;
                tiempo_mensaje=0;//comienza tiempo antes de enviar mensaje
            }

            break;

        case NocheOK:
            Nueva_pantalla(0x10,0x10,0x10);

            //FONDO
            set_color_azul();
            ComRect(0,0, HSIZE, VSIZE, true);
            set_color_blanco();
            ComTXT(150,VSIZE/4,26,OPT_CENTER,"A que hora?");

            // ----- Etiquetas -----
            set_color_blanco();
            ComTXT(90, 80, 26, OPT_CENTER, "HORA");
            ComTXT(210, 80, 26, OPT_CENTER, "MINUTO");

            // ----- Triangulos arriba -----
            ComColor(0,0,0);
            ComFgcolor(0,0, 255);
            ComButton(75, 100, 30, 35, 22,OPT_FLAT, "^");   // â–² hora
            ComButton(195, 100, 30, 35, 22,OPT_FLAT, "^"); // â–² minuto

            // ----- Numeros -----
            set_color_blanco();
            ComRect(80,140, 100, 160, true);
            ComRect(200,140, 220, 160, true);
            set_color_negro();
            sprintf(buffer, "%02d", horas_noche);
            ComTXT(90, 150, 28, OPT_CENTER, buffer);
            sprintf(buffer, "%02d", minutos_noche);
            ComTXT(210, 150, 28, OPT_CENTER, buffer);

            // ----- Triangulos abajo -----
            ComColor(0,0,0);
            ComFgcolor(0,0, 255);
            ComButton(75, 170, 30, 35, 22,OPT_FLAT, "v");   // â–¼ hora
            ComButton(195, 170, 30, 35, 22,OPT_FLAT, "v"); // â–¼ minuto

            // ----- Boton OK -----
            ComColor(0,0,0);
            ComFgcolor(0,255, 0);
            ComButton(280, 200, 30, 30, 22,OPT_CENTER, "OK");

            Dibuja();

            if(Boton(75, 100, 30, 30, 22, "^"))//horas
            {
                if (horas_noche <= 23)
                {
                    horas_noche++;
                }
            }
            if(Boton(195, 100, 30, 30, 22, "^"))//minutos
            {
                if (minutos_noche < 59)
                {
                    minutos_noche++;
                }
            }
            if(Boton(75, 170, 30, 30, 22, "v"))//horas
            {
                if (horas_noche >18)
                {
                    horas_noche--;
                }
            }
            if(Boton(195, 170, 30, 30, 22,"v"))//minutos
            {
                if (minutos_noche >0)
                {
                    minutos_noche--;
                }
            }
            if(Boton(280, 200, 30, 30, 22, "OK"))
            {
                estado_actual=Inicio_i;
                r_noche=0;
                g_noche=1;
                b_noche=0;
            }
            break;

            //--------------------------------------INICIO-----------------------------------------------//

        case Inicio_i:



            Nueva_pantalla(0x10,0x10,0x10);

            //FONDO
            set_color_azul();
            ComRect(0,0, HSIZE, VSIZE, true);


            //DIA DE LA SEMANA
            sprintf(buffer, "%02d:%02d:%02d", horas, minutos, segundos);
            set_color_blanco();
            ComRect(53, 30, 265, 90, true);

            set_color_negro();
            ComTXT((53+265)/2, (30+90)/2 - 10, 28, OPT_CENTER, buffer); // hora centrada un poco hacia arriba

            // DIBUJAR DIA DE LA SEMANA DEBAJO
            set_color_negro();
            ComTXT((53+265)/2, (30+90)/2 + 20, 22, OPT_CENTER, semana_completa[idx]); // dia debajo

            //BOTONES
            ComColor(0,0,0);
            ComFgcolor(255*r_manana,255*g_manana,255*b_manana);
            ComButton(20, 140, 80, 80, 22,OPT_FLAT, "M");//boton manana
            ComFgcolor(255*r_tarde,255*g_tarde,255*b_tarde);
            ComButton(116, 140, 80, 80, 22,OPT_FLAT, "T");//boton tarde
            ComFgcolor(255*r_noche,255*g_noche,255*b_noche);
            ComButton(212, 140, 80, 80, 22,OPT_FLAT, "N");//boton noche

            Dibuja();
            estado_anterior=estado_actual;

            if(horas>=2 && horas<=3){//Se reinicia todo a las 2 de la manana
                r_manana=255;
                g_manana=255;
                b_manana=255;

                r_tarde=255;
                g_tarde=255;
                b_tarde=255;

                r_noche=255;
                g_noche=255;
                b_noche=255;

            }

            if((no_manana==1 || no_tarde==1 || no_noche==1) && segundos>30)//Se envia mensaje por whatsapp a adulto

                if(Boton(20, 140, 80, 80,28, "M")) //Si aprieto boton M
                {
                    estado_actual=Manana;

                }
                if(Boton(116, 140, 80, 80,28, "T")) //Si aprieto boton T
                {
                    estado_actual=Tarde;

                }
                if(Boton(212, 140, 80, 80,28, "N")) //Si aprieto boton N
                {
                    estado_actual=Noche;

                }

            if(horas==alarma_manana && minutos==minutos_manana_ant)//Si se cumple el tiempo de la alarma de la manana
            {
                estado_actual=AlarmaManana;
            }
            if(horas==alarma_tarde && minutos==minutos_tarde_ant)//Si se cumple el tiempo de la alarma de la tarde
            {
                estado_actual=AlarmaTarde;
            }
            if(horas==alarma_noche && minutos==minutos_noche_ant)//Si se cumple el tiempo de la alarma de la noche
            {
                estado_actual=AlarmaNoche;
            }

            break;


            //--------------------------------------Usuario quiere poner alarma de mañana-----------------------------------------------//

        case Manana:

            horas_manana=alarma_manana;
            minutos_manana=minutos_manana_ant;
            sprintf(texto_hora_manana, "%02d:%02d", alarma_manana,minutos_manana);   // convierte 12 → "12"

            Nueva_pantalla(0x10,0x10,0x10);

            //FONDO
            set_color_azul();
            ComRect(0,0, HSIZE, VSIZE, true);
            ComFgcolor(255,255, 255);
            ComButton(20,20,100,30,22,OPT_FLAT,texto_hora_manana);

            ComFgcolor(170,170, 170);
            if(no_manana==1) ComFgcolor(0,255, 0);
            ComButton(20,70,100,30,22,OPT_FLAT,"ABRIR");

            // ----- Boton CANCEL -----
            ComColor(0,0,0);
            ComFgcolor(255,0, 0);
            ComButton(10, 200, 70, 30, 22,OPT_CENTER, "CANCEL");

            Dibuja();
            estado_anterior=estado_actual;

            if(Boton(20,70,100,30,22,"ABRIR") && no_manana==1)
            {
                estado_actual=AbrirManana;
                tiempo_inicio=segundos;
            }
            if(Boton(20,20,100,30,22,texto_hora_manana))
            {
                estado_actual=ConfigHora1;
            }
            if(Boton(10, 200, 70, 30, 22,"CANCEL"))
            {
                estado_actual=Inicio_i;
            }

            if((no_manana==1 || no_tarde==1 || no_noche==1) && segundos>30)//Se envia mensaje por whatsapp a adulto


                if(horas==alarma_manana && minutos==minutos_manana_ant)//Si se cumple el tiempo de la alarma de la manana
                {
                    estado_actual=AlarmaManana;
                }
                if(horas==alarma_tarde && minutos==minutos_tarde_ant)//Si se cumple el tiempo de la alarma de la tarde
                {
                    estado_actual=AlarmaTarde;
                }
                if(horas==alarma_noche && minutos==minutos_noche_ant)//Si se cumple el tiempo de la alarma de la noche
                {
                    estado_actual=AlarmaNoche;
                }



            break;

        case ConfigHora1:

            Nueva_pantalla(0x10,0x10,0x10);

            //FONDO
            set_color_azul();
            ComRect(0,0, HSIZE, VSIZE, true);
            set_color_blanco();
            ComTXT(150,VSIZE/4,26,OPT_CENTER,"Selecciona hora a la que quiere la alarma");

            // ----- Etiquetas -----
            set_color_blanco();
            ComTXT(90, 80, 26, OPT_CENTER, "HORA");
            ComTXT(210, 80, 26, OPT_CENTER, "MINUTO");

            // ----- Triangulos arriba -----
            ComColor(0,0,0);
            ComFgcolor(0,0, 255);
            ComButton(75, 100, 30, 35, 22,OPT_FLAT, "^");   // â–² hora
            ComButton(195, 100, 30, 35, 22,OPT_FLAT, "^"); // â–² minuto

            // ----- Numeros -----
            set_color_blanco();
            ComRect(80,140, 100, 160, true);
            ComRect(200,140, 220, 160, true);
            set_color_negro();
            sprintf(buffer, "%02d", horas_manana);
            ComTXT(90, 150, 28, OPT_CENTER, buffer);
            sprintf(buffer, "%02d", minutos_manana);
            ComTXT(210, 150, 28, OPT_CENTER, buffer);

            // ----- Triangulos abajo -----
            ComColor(0,0,0);
            ComFgcolor(0,0, 255);
            ComButton(75, 170, 30, 35, 22,OPT_FLAT, "v");   // â–¼ hora
            ComButton(195, 170, 30, 35, 22,OPT_FLAT, "v"); // â–¼ minuto

            // ----- Boton OK -----
            ComColor(0,0,0);
            ComFgcolor(0,255, 0);
            ComButton(280, 200, 30, 30, 22,OPT_CENTER, "OK");

            // ----- Boton CANCEL -----
            ComColor(0,0,0);
            ComFgcolor(255,0, 0);
            ComButton(10, 200, 70, 30, 22,OPT_CENTER, "CANCEL");


            Dibuja();
            estado_anterior=estado_actual;

            if(Boton(75, 100, 30, 30, 22, "^"))//horas
            {
                if (horas_manana < 12)
                {
                    horas_manana++;
                }
            }
            if(Boton(195, 100, 30, 30, 22, "^"))//minutos
            {
                if (minutos_manana < 59)
                {
                    minutos_manana++;
                }
            }
            if(Boton(75, 170, 30, 30, 22, "v"))//horas
            {
                if (horas_manana >6)
                {
                    horas_manana--;
                }
            }
            if(Boton(195, 170, 30, 30, 22,"v"))//minutos
            {
                if (minutos_manana >0)
                {
                    minutos_manana--;
                }
            }
            if(Boton(280, 200, 30, 30, 22, "OK"))
            {
                estado_actual=Manana;
                alarma_manana=horas_manana;
                minutos_manana_ant=minutos_manana;

                horas_tarde=horas_manana+8;
                minutos_tarde=minutos_manana;
                horas_noche=horas_manana+16;
                minutos_noche=minutos_manana;

            }
            if(Boton(10, 200, 70, 30, 22, "CANCEL"))
            {
                estado_actual=Manana;
                minutos_manana=minutos_manana_ant;
            }

            if((no_manana==1 || no_tarde==1 || no_noche==1) && segundos>30)//Se envia mensaje por whatsapp a adulto


                if(horas==alarma_manana && minutos==minutos_manana_ant)//Si se cumple el tiempo de la alarma de la manana
                {
                    estado_actual=AlarmaManana;
                }
                if(horas==alarma_tarde && minutos==minutos_tarde_ant)//Si se cumple el tiempo de la alarma de la tarde
                {
                    estado_actual=AlarmaTarde;
                }
                if(horas==alarma_noche && minutos==minutos_noche_ant)//Si se cumple el tiempo de la alarma de la noche
                {
                    estado_actual=AlarmaNoche;
                }

            break;

        case AbrirManana:

            Nueva_pantalla(0x10,0x10,0x10);

            //FONDO
            set_color_azul();
            ComRect(0,0, HSIZE, VSIZE, true);
            set_color_blanco();
            ComTXT(HSIZE/2, VSIZE/3, 28, OPT_CENTER, "Recoja su pastilla");
            no_manana=0;
            r_manana=0;
            g_manana=1;
            b_manana=0;

            Dibuja();

            posicion=3233;
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, posicion);

            if ((segundos - tiempo_inicio) >= 5) {  // han pasado 5 s
                estado_actual = Manana;
                posicion=Min_pos;
                PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, posicion);
            }

            break;


            //--------------------------------------Usuario quiere poner alarma de tarde-----------------------------------------------//

        case Tarde:

            horas_tarde=alarma_tarde;
            minutos_tarde=minutos_tarde_ant;
            sprintf(texto_hora_tarde, "%02d:%02d", alarma_tarde,minutos_tarde);   // convierte 12 → "12"

            Nueva_pantalla(0x10,0x10,0x10);

            //FONDO
            set_color_azul();
            ComRect(0,0, HSIZE, VSIZE, true);
            ComFgcolor(255,255, 255);
            ComButton(20,20,100,30,22,OPT_FLAT,texto_hora_tarde);

            ComFgcolor(170,170, 170);
            if(no_tarde==1) ComFgcolor(0,255, 0);
            ComButton(20,70,100,30,22,OPT_FLAT,"ABRIR");

            // ----- Boton CANCEL -----
            ComColor(0,0,0);
            ComFgcolor(255,0, 0);
            ComButton(10, 200, 70, 30, 22,OPT_CENTER, "CANCEL");

            Dibuja();

            estado_anterior=estado_actual;
            if(Boton(20,70,100,30,22,"ABRIR") && no_tarde==1)
            {
                estado_actual=AbrirTarde;
                tiempo_inicio=segundos;
            }
            if(Boton(20,20,100,30,22,texto_hora_tarde))
            {
                estado_actual=ConfigHora2;
            }
            if(Boton(10, 200, 70, 30, 22,"CANCEL"))
            {
                estado_actual=Inicio_i;
            }

            if((no_manana==1 || no_tarde==1 || no_noche==1) && segundos>30)//Se envia mensaje por whatsapp a adulto


                if(horas==alarma_manana && minutos==minutos_manana_ant)//Si se cumple el tiempo de la alarma de la manana
                {

                    estado_actual=AlarmaManana;
                }
                if(horas==alarma_tarde && minutos==minutos_tarde_ant)//Si se cumple el tiempo de la alarma de la tarde
                {
                    estado_actual=AlarmaTarde;
                }
                if(horas==alarma_noche && minutos==minutos_noche_ant)//Si se cumple el tiempo de la alarma de la noche
                {
                    estado_actual=AlarmaNoche;
                }



            break;

        case ConfigHora2:

            Nueva_pantalla(0x10,0x10,0x10);

            //FONDO
            set_color_azul();
            ComRect(0,0, HSIZE, VSIZE, true);
            set_color_blanco();
            ComTXT(150,VSIZE/4,26,OPT_CENTER,"Selecciona hora a la que quiere la alarma");

            // ----- Etiquetas -----
            set_color_blanco();
            ComTXT(90, 80, 26, OPT_CENTER, "HORA");
            ComTXT(210, 80, 26, OPT_CENTER, "MINUTO");

            // ----- Triangulos arriba -----
            ComColor(0,0,0);
            ComFgcolor(0,0, 255);
            ComButton(75, 100, 30, 35, 22,OPT_FLAT, "^");   // â–² hora
            ComButton(195, 100, 30, 35, 22,OPT_FLAT, "^"); // â–² minuto

            // ----- Numeros -----
            set_color_blanco();
            ComRect(80,140, 100, 160, true);
            ComRect(200,140, 220, 160, true);
            set_color_negro();
            sprintf(buffer, "%02d", horas_tarde);
            ComTXT(90, 150, 28, OPT_CENTER, buffer);
            sprintf(buffer, "%02d", minutos_tarde);
            ComTXT(210, 150, 28, OPT_CENTER, buffer);

            // ----- Triangulos abajo -----
            ComColor(0,0,0);
            ComFgcolor(0,0, 255);
            ComButton(75, 170, 30, 35, 22,OPT_FLAT, "v");   // â–¼ hora
            ComButton(195, 170, 30, 35, 22,OPT_FLAT, "v"); // â–¼ minuto

            // ----- Boton OK -----
            ComColor(0,0,0);
            ComFgcolor(0,255, 0);
            ComButton(280, 200, 30, 30, 22,OPT_CENTER, "OK");

            // ----- Boton CANCEL -----
            ComColor(0,0,0);
            ComFgcolor(255,0, 0);
            ComButton(10, 200, 70, 30, 22,OPT_CENTER, "CANCEL");


            Dibuja();

            if(Boton(75, 100, 30, 30, 22, "^"))//horas
            {
                if (horas_tarde < 18)
                {
                    horas_tarde++;
                }
            }
            if(Boton(195, 100, 30, 30, 22, "^"))//minutos
            {
                if (minutos_tarde < 59)
                {
                    minutos_tarde++;
                }
            }
            if(Boton(75, 170, 30, 30, 22, "v"))//horas
            {
                if (horas_tarde >12)
                {
                    horas_tarde--;
                }
            }
            if(Boton(195, 170, 30, 30, 22,"v"))//minutos
            {
                if (minutos_tarde >1)
                {
                    minutos_tarde--;
                }
            }
            if(Boton(280, 200, 30, 30, 22, "OK"))
            {
                estado_actual=Manana;
                alarma_tarde=horas_tarde;
                minutos_tarde_ant=minutos_tarde;

            }
            if(Boton(10, 200, 70, 30, 22, "CANCEL"))
            {
                estado_actual=Tarde;
                minutos_tarde=minutos_tarde_ant;
            }

            estado_anterior=estado_actual;
            if((no_manana==1 || no_tarde==1 || no_noche==1) && segundos>30)//Se envia mensaje por whatsapp a adulto


                if(horas==alarma_manana && minutos==minutos_manana_ant)//Si se cumple el tiempo de la alarma de la manana
                {
                    estado_actual=AlarmaManana;
                }
                if(horas==alarma_tarde && minutos==minutos_tarde_ant)//Si se cumple el tiempo de la alarma de la tarde
                {
                    estado_actual=AlarmaTarde;
                }
                if(horas==alarma_noche && minutos==minutos_noche_ant)//Si se cumple el tiempo de la alarma de la noche
                {
                    estado_actual=AlarmaNoche;
                }

            break;

        case AbrirTarde:

            Nueva_pantalla(0x10,0x10,0x10);

            //FONDO
            set_color_azul();
            ComRect(0,0, HSIZE, VSIZE, true);
            set_color_blanco();;
            ComTXT(HSIZE/2, VSIZE/3, 28, OPT_CENTER, "Recoja su pastilla");
            no_tarde=0;
            r_tarde=0;
            g_tarde=1;
            b_tarde=0;

            Dibuja();

            posicion=3233;
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, posicion);

            if ((segundos - tiempo_inicio) >= 5) {  // han pasado 3 s
                estado_actual = Tarde;
                posicion=Min_pos;
                PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, posicion);
            }

            break;


            //--------------------------------------Usuario quiere poner alarma de noche-----------------------------------------------//

        case Noche:

            horas_noche=alarma_noche;
            minutos_noche=minutos_noche_ant;
            sprintf(texto_hora_noche, "%02d:%02d", alarma_noche,minutos_noche);   // convierte 12 → "12"

            Nueva_pantalla(0x10,0x10,0x10);

            //FONDO
            set_color_azul();
            ComRect(0,0, HSIZE, VSIZE, true);
            ComFgcolor(255,255, 255);
            ComButton(20,20,100,30,22,OPT_FLAT,texto_hora_noche);

            ComFgcolor(170,170, 170);
            if(no_noche==1) ComFgcolor(0,255, 0);
            ComButton(20,70,100,30,22,OPT_FLAT,"ABRIR");

            // ----- Boton CANCEL -----
            ComColor(0,0,0);
            ComFgcolor(255,0, 0);
            ComButton(10, 200, 70, 30, 22,OPT_CENTER, "CANCEL");

            Dibuja();
            estado_anterior=estado_actual;

            if(Boton(20,70,100,30,22,"ABRIR") && no_noche==1)
            {
                estado_actual=AbrirNoche;
                tiempo_inicio=segundos;
            }
            if(Boton(20,20,100,30,22,texto_hora_noche))
            {
                estado_actual=ConfigHora3;
            }
            if(Boton(10, 200, 70, 30, 22,"CANCEL"))
            {
                estado_actual=Inicio_i;
            }

            if((no_manana==1 || no_tarde==1 || no_noche==1) && segundos>30)//Se envia mensaje por whatsapp a adulto


                if(horas==alarma_manana && minutos==minutos_manana_ant)//Si se cumple el tiempo de la alarma de la manana
                {
                    estado_actual=AlarmaManana;
                }
                if(horas==alarma_tarde && minutos==minutos_tarde_ant)//Si se cumple el tiempo de la alarma de la tarde
                {
                    estado_actual=AlarmaTarde;
                }
                if(horas==alarma_noche && minutos==minutos_noche_ant)//Si se cumple el tiempo de la alarma de la noche
                {
                    estado_actual=AlarmaNoche;
                }



            break;

        case ConfigHora3:

            Nueva_pantalla(0x10,0x10,0x10);

            //FONDO
            set_color_azul();
            ComRect(0,0, HSIZE, VSIZE, true);
            set_color_blanco();
            ComTXT(150,VSIZE/4,26,OPT_CENTER,"Selecciona hora a la que quiere la alarma");

            // ----- Etiquetas -----
            set_color_blanco();
            ComTXT(90, 80, 26, OPT_CENTER, "HORA");
            ComTXT(210, 80, 26, OPT_CENTER, "MINUTO");

            // ----- Triangulos arriba -----
            ComColor(0,0,0);
            ComFgcolor(0,0, 255);
            ComButton(75, 100, 30, 35, 22,OPT_FLAT, "^");   // â–² hora
            ComButton(195, 100, 30, 35, 22,OPT_FLAT, "^"); // â–² minuto

            // ----- Numeros -----
            set_color_blanco();
            ComRect(80,140, 100, 160, true);
            ComRect(200,140, 220, 160, true);
            set_color_negro();
            sprintf(buffer, "%02d", horas_noche);
            ComTXT(90, 150, 28, OPT_CENTER, buffer);
            sprintf(buffer, "%02d", minutos_noche);
            ComTXT(210, 150, 28, OPT_CENTER, buffer);

            // ----- Triangulos abajo -----
            ComColor(0,0,0);
            ComFgcolor(0,0, 255);
            ComButton(75, 170, 30, 35, 22,OPT_FLAT, "v");   // â–¼ hora
            ComButton(195, 170, 30, 35, 22,OPT_FLAT, "v"); // â–¼ minuto

            // ----- Boton OK -----
            ComColor(0,0,0);
            ComFgcolor(0,255, 0);
            ComButton(280, 200, 30, 30, 22,OPT_CENTER, "OK");

            // ----- Botonn CANCEL -----
            ComColor(0,0,0);
            ComFgcolor(255,0, 0);
            ComButton(10, 200, 70, 30, 22,OPT_CENTER, "CANCEL");


            Dibuja();

            if(Boton(75, 100, 30, 30, 22, "^"))//horas
            {
                if (horas_noche <= 23)
                {
                    horas_noche++;
                }
            }
            if(Boton(195, 100, 30, 30, 22, "^"))//minutos
            {
                if (minutos_noche < 59)
                {
                    minutos_noche++;
                }
            }
            if(Boton(75, 170, 30, 30, 22, "v"))//horas
            {
                if (horas_noche >18)
                {
                    horas_noche--;
                }
            }
            if(Boton(195, 170, 30, 30, 22,"v"))//minutos
            {
                if (minutos_noche >1)
                {
                    minutos_noche--;
                }
            }
            if(Boton(280, 200, 30, 30, 22, "OK"))
            {
                estado_actual=Noche;
                alarma_noche=horas_noche;
                minutos_noche_ant=minutos_noche;

            }
            if(Boton(10, 200, 70, 30, 22, "CANCEL"))
            {
                estado_actual=Noche;
                minutos_noche=minutos_noche_ant;
            }

            estado_anterior=estado_actual;
            if((no_manana==1 || no_tarde==1 || no_noche==1) && segundos>30)//Se envia mensaje por whatsapp a adulto


                if(horas==alarma_manana && minutos==minutos_manana_ant)//Si se cumple el tiempo de la alarma de la manana
                {
                    estado_actual=AlarmaManana;
                }
                if(horas==alarma_tarde && minutos==minutos_tarde_ant)//Si se cumple el tiempo de la alarma de la tarde
                {
                    estado_actual=AlarmaTarde;
                }
                if(horas==alarma_noche && minutos==minutos_noche_ant)//Si se cumple el tiempo de la alarma de la noche
                {
                    estado_actual=AlarmaNoche;
                }
            break;

        case AbrirNoche:

            Nueva_pantalla(0x10,0x10,0x10);

            //FONDO
            set_color_azul();
            ComRect(0,0, HSIZE, VSIZE, true);
            set_color_blanco();
            ComTXT(HSIZE/2, VSIZE/3, 28, OPT_CENTER, "Recoja su pastilla");
            no_noche=0;
            r_noche=0;
            g_noche=1;
            b_noche=0;

            Dibuja();

            posicion=3233;
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, posicion);

            if ((segundos - tiempo_inicio) >= 5) {  // han pasado 3 s
                estado_actual = Noche;
                posicion=Min_pos;
                PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, posicion);
            }

            break;


            //--------------------------------------Alarmas-----------------------------------------------//

        case AlarmaManana :
            Nueva_pantalla(0x10,0x10,0x10);


                        VolNota(255);        // volumen
                        TocaNota(S_BEEP, 50);
                        set_color_negro();
                        set_color_blanco();
                        ComTXT(HSIZE/2, VSIZE/3, 28, OPT_CENTER, "ALARMA MANANA");
                        if(parp==1)
                        {
                            set_color_blanco();
                            ComTXT(HSIZE/2, VSIZE/2, 28, OPT_CENTER, texto_hora_manana);
                        }
                        else
                        {
                            set_color_negro();
                            ComTXT(HSIZE/2, VSIZE/2, 28, OPT_CENTER, texto_hora_manana);
                        }

                        // ----- Boton Apagar -----
                        ComColor(0,0,0);
                        ComFgcolor(0,255, 0);
                        ComButton(250, 200, 70, 30, 22,OPT_CENTER, "Apagar");

                        // ----- Boton Cancelar -----
                        ComColor(0,0,0);
                        ComFgcolor(255,0, 0);
                        ComButton(10, 200, 70, 30, 22,OPT_CENTER, "Cancelar");

                        Dibuja();
                        if(Boton(250, 200, 70, 30, 22, "Apagar"))
                                   {
                                       estado_actual=AbrirManana;
                                       alarma_manana=0;
                                       minutos_manana_ant=0;
                                       FinNota();

                                   }
                        if(Boton(10, 200, 70, 30, 22, "Cancelar"))
                                   {
                                       estado_actual=estado_anterior;
                                       alarma_manana=0;
                                       minutos_manana_ant=0;
                                       FinNota();
                                       no_manana=1;

                                   }

            break;

        case AlarmaTarde :

            Nueva_pantalla(0x10,0x10,0x10);


                        VolNota(255);        // volumen
                        TocaNota(S_BEEP, 50);
                        set_color_negro();
                        ComRect(0,0, HSIZE, VSIZE, true);
                        set_color_blanco();
                        ComTXT(HSIZE/2, VSIZE/3, 28, OPT_CENTER, "ALARMA TARDE");
                        if(parp==1)
                        {
                            set_color_blanco();
                            ComTXT(HSIZE/2, VSIZE/2, 28, OPT_CENTER, texto_hora_tarde);
                        }
                        else
                        {
                            set_color_negro();
                            ComTXT(HSIZE/2, VSIZE/2, 28, OPT_CENTER, texto_hora_tarde);
                        }

                        // ----- Boton Apagar -----
                        ComColor(0,0,0);
                        ComFgcolor(0,255, 0);
                        ComButton(250, 200, 70, 30, 22,OPT_CENTER, "Apagar");

                        // ----- Boton Cancelar -----
                        ComColor(0,0,0);
                        ComFgcolor(255,0, 0);
                        ComButton(10, 200, 70, 30, 22,OPT_CENTER, "Cancelar");

                        Dibuja();
                        if(Boton(250, 200, 70, 30, 22, "Apagar"))
                                   {
                                       estado_actual=AbrirTarde;
                                       alarma_tarde=0;
                                       minutos_tarde_ant=0;
                                       FinNota();


                                   }
                        if(Boton(10, 200, 70, 30, 22, "Cancelar"))
                                   {
                                       estado_actual=estado_anterior;
                                       alarma_tarde=0;
                                       minutos_tarde_ant=0;
                                       FinNota();
                                       no_tarde=1;

                                   }

            break;

        case AlarmaNoche :

            Nueva_pantalla(0x10,0x10,0x10);


                        VolNota(255);        // volumen
                        TocaNota(S_BEEP, 50);
                        set_color_negro();
                        ComRect(0,0, HSIZE, VSIZE, true);
                        set_color_blanco();
                        ComTXT(HSIZE/2, VSIZE/3, 28, OPT_CENTER, "ALARMA NOCHE");
                        if(parp==1)
                        {
                            set_color_blanco();
                            ComTXT(HSIZE/2, VSIZE/2, 28, OPT_CENTER, texto_hora_noche);
                        }
                        else
                        {
                            set_color_negro();
                            ComTXT(HSIZE/2, VSIZE/2, 28, OPT_CENTER, texto_hora_noche);
                        }

                        // ----- Boton Apagar -----
                        ComColor(0,0,0);
                        ComFgcolor(0,255, 0);
                        ComButton(250, 200, 70, 30, 22,OPT_CENTER, "Apagar");

                        // ----- Boton Cancelar -----
                        ComColor(0,0,0);
                        ComFgcolor(255,0, 0);
                        ComButton(10, 200, 70, 30, 22,OPT_CENTER, "Cancelar");

                        Dibuja();

                        if(Boton(250, 200, 70, 30, 22, "Apagar"))
                                   {
                                       estado_actual=AbrirNoche;
                                       alarma_noche=0;
                                       minutos_noche_ant=0;
                                       FinNota();


                                   }
                        if(Boton(10, 200, 70, 30, 22, "Cancelar"))
                                   {
                                       estado_actual=estado_anterior;
                                       alarma_noche=0;
                                       minutos_noche_ant=0;
                                       FinNota();
                                       no_noche=1;

                                   }

            break;

        }
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
                    idx=(idx+1)%7;
                }
            }
        }
    }
}

void IntTimer1(void)
{
    TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT); // Borra flag
    SysCtlDelay(20); //Retraso necesario. Mirar Driverlib p.550-->delay suficiente para que se borre el timer
    Flag_ints++;

    parp=!parp; //variable que se devuelve 1 o 0 cada 125 ms
}




