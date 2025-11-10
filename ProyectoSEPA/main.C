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

 Reposo,
 Recarga,
 Mañana,
 Tarde,
 Noche,
 AvisoM,
 AvisoT,
 AvisoN,
 EsperoN

};

enum estados estado_actual, estado_anterior;



//----------------------------------Variables---------------------------------//

const char *semana[]= {"LUN","MAR","MIE","JUE","VIE","SAB","DOM"}; //Dias de la semana que puede introducir el usuario
char dia[4];//variable donde se registra el dia por el usuario
char hora[3];//variable donde se registra la hora por el usuario
char minuto[3];//variable donde se registra el minuto por el usuario

char c;//variable que registra lo que guarda el usuario
int h;//variable que registra el valor numerico de la hora introducida
int m;//variable que registra el valor numerico del minuto introducido

int dias,horas, minutos, segundos; //hora calculada por el micro
char buffer[9]; //Variable que imprime la hora en la pantalla
int p; //Contador para contar segundos

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

uint32_t temporizador=0;
int m = 0;
int n;




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
   ComTXT(53,180,26,OPT_CENTER,"M");

   set_color_blanco();
    ComRect (116,140,192,220,true); //boton tarde
     set_color_negro();
    ComTXT(149,180,26,OPT_CENTER,"T");

    set_color_blanco();
     ComRect (212,140,288,220,true); //boton noche
      set_color_negro();
     ComTXT(245,180,26,OPT_CENTER,"N");

    Dibuja();

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
            set_color_negro();
            ComRect(0,0, HSIZE, VSIZE, true);
            set_color_blanco();
            ComTXT(HSIZE/2,VSIZE/2,26,OPT_CENTER,"Introduce Dia, Hora y minuto");

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
                estado_actual = Dia2;  // siguiente estado
            }

            break;

        case Dia2:
            if(UARTCharsAvail(UART0_BASE))
            {
                c = UARTCharGetNonBlocking(UART0_BASE);
                dia[1] = c;
                UARTCharPutNonBlocking(UART0_BASE, c);
                estado_actual = Dia3;  // siguiente estado
            }

            break;

        case Dia3:
            if(UARTCharsAvail(UART0_BASE))
            {
                c = UARTCharGetNonBlocking(UART0_BASE);
                dia[2] = c;
                dia[3] = '\0'; // fin de cadena
                UARTCharPutNonBlocking(UART0_BASE, c);
                UARTCharPut(UART0_BASE, '\n');
                estado_actual = Verificacion_dia;  // siguiente estado

            }

            break;

        case Verificacion_dia:
            idx = validar_dia(dia);
            if (idx >= 0)
            { // día correcto

                UARTprintf("Dia valido: %s\n", semana[idx]);
                UARTprintf("\n Introuzca la hora del dia (00-23):\n");
                estado_actual = Hora1;
            }
            else
            {// día no válido

                UARTprintf("Error: dia no valido (se recibio '%c%c%c')\n", dia[0], dia[1], dia[2]);
                UARTprintf("Introducir de nuevo\n");
                estado_actual = Inicio;
            }
            break;


            //--------------------------------------Usuario introduce hora-----------------------------------------------//

        case Hora1:
            if (UARTCharsAvail(UART0_BASE)) {
                c = UARTCharGetNonBlocking(UART0_BASE);
                if (c >= '0' && c <= '9') {
                    hora[0] = c;
                    UARTCharPutNonBlocking(UART0_BASE, c);
                    estado_actual = Hora2;
                }
                else {
                    UARTprintf("\nError: solo se permiten números.\n");
                    UARTprintf("\nIntroducir de nuevo:\n");
                    estado_actual = Hora1;
                }
            }
            break;

        case Hora2:
            if (UARTCharsAvail(UART0_BASE)) {
                c = UARTCharGetNonBlocking(UART0_BASE);

                // Verificar si es un dígito
                if (c >= '0' && c <= '9') {
                    hora[1] = c;
                    hora[2] = '\0';
                    UARTCharPutNonBlocking(UART0_BASE, c);
                    estado_actual = Verificacion_hora;
                } else {
                    UARTprintf("\nError: solo se permiten números.\n");
                    UARTprintf("\nIntroducir de nuevo:\n");
                    estado_actual = Hora1;
                }
            }
            break;

        case Verificacion_hora:
            h = atoi(hora); //se convierte texto a entero

            if (h >= 0 && h <= 23)
            {
                UARTprintf("\nHora correcta: %d\n", h);
                UARTprintf("\nIntroducir minuto(0-59): \n");
                horas=h;
                estado_actual = Min1; // siguiente paso
            }
            else
            {
                UARTprintf("\nError: hora incorrecta\n");
                UARTprintf("Introducir de nuevo\n");
                estado_actual = Hora1;
            }

            break;


            //--------------------------------------Usuario introduce minutos-----------------------------------------------//

        case Min1:
            if (UARTCharsAvail(UART0_BASE)) {
                            c = UARTCharGetNonBlocking(UART0_BASE);
                            if (c >= '0' && c <= '9') {
                                minuto[0] = c;
                                UARTCharPutNonBlocking(UART0_BASE, c);
                                estado_actual = Min2;
                            }
                            else {
                                UARTprintf("\nError: solo se permiten números.\n");
                                UARTprintf("\nIntroducir de nuevo:\n");
                                estado_actual = Min1;
                            }
                        }
            break;

        case Min2:
            if (UARTCharsAvail(UART0_BASE)) {
                c = UARTCharGetNonBlocking(UART0_BASE);

                // Verificar si es un dígito
                if (c >= '0' && c <= '9') {
                    minuto[1] = c;
                    minuto[2] = '\0';
                    UARTCharPutNonBlocking(UART0_BASE, c);
                    estado_actual = Verificacion_min;
                } else {
                    UARTprintf("\nError: solo se permiten números.\n");
                    UARTprintf("\nIntroducir de nuevo:\n");
                    estado_actual = Min1;
                }
            }
            break;

        case Verificacion_min:
            m = atoi(minuto); //se convierte texto a entero

            if (m >= 0 && m <= 60)
            {
                UARTprintf("\nMinuto correcto: %d\n", m);
                minutos=m;
                estado_actual = Reposo;
            }
            else
            {
                UARTprintf("\nError: minuto incorrecto\n");
                UARTprintf("Introducir de nuevo\n");
                estado_actual = Min1;
            }

            break;

        case Reposo:



                    sprintf(buffer, "%02d:%02d:%02d", horas, minutos, segundos);

                    Nueva_pantalla(0x10,0x10,0x10);

                    //FONDO
                    set_color_azul();
                    ComRect(0,0, HSIZE, VSIZE, true);


                    //DIA DE LA SEMANA
                    sprintf(buffer, "%02d:%02d:%02d", horas, minutos, segundos);
                    set_color_blanco();             // borrar fondo de la hora
                    ComRect(53,30,265,90,true);
                    set_color_negro();              // color del texto hora
                    ComTXT((53+265)/2, (30+90)/2, 28, OPT_CENTER, buffer); // dibuja la hora centrada

                    //BOTONES
                    ComColor(0,0,0);
                    ComFgcolor(255,255, 255);
                    ComButton(20, 140, 80, 80, 22,OPT_FLAT, "M");//boton noche
                    ComButton(116, 140, 80, 80, 22,OPT_FLAT, "T");//boton tarde
                    ComButton(212, 140, 80, 80, 22,OPT_FLAT, "N");//boton noche

                    Dibuja();

                    if(Boton(20, 140, 80, 80,28, "M")) //Si aprieto boton Pieza A
                                {
                                    estado_actual=Mañana;

                                }

                    if(Boton(116, 140, 80, 80,28, "T")) //Si aprieto boton Pieza A
                                {
                                    estado_actual=Tarde;

                                }

                    if(Boton(212, 140, 80, 80,28, "N")) //Si aprieto boton Pieza A
                                {
                                    estado_actual=Noche;

                                }




            break;
/*
        case Recarga:
            set_color_negro();

            ComTXT(160,60,28,OPT_CENTER, dias_semana[dias]);
            posicion=Max_pos;
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, posicion); // Mueve un poco el servo
            SysCtlDelay(SysCtlClockGet() / 3); // espera 1 seg

            // Decide siguiente estado según hora
            if (horas >= 4 && horas < 12)
                estado_actual = Mañana;
            else if (horas >= 12 && horas < 18)
                estado_actual = Tarde;
            else
                estado_actual = Noche;
            break;

        case Mañana:
            if (Tarde) set_color_verde();
            else set_color_blanco();
            ComRect(20,140,96,220,true);
            set_color_negro();
            ComTXT(53,180,26,OPT_CENTER,"MANANA");

            if (horas >= 12 && horas < 18) estado_actual = Tarde;
            if (horas >= 18 || horas < 4) estado_actual = Noche;
            break;
        case Tarde:
            if (horas >= 18 || horas < 4) estado_actual = Noche;
            if (horas >= 4 && horas < 12) estado_actual = Mañana;
            break;
        case Noche:
            if (horas >= 4 && horas < 12) estado_actual = Mañana;
            if (horas >= 12 && horas < 18) estado_actual = Tarde;
            break;
        case AvisoM:
            break;
        case AvisoT:
            break;
        case AvisoN:
            break;
        case EsperoN:
            break;

        } */


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
                    dias++;
                    if (dias >= 7)
                        dias=0;
                }
            }
        }
    }
}






