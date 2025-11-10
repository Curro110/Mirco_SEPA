#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "driverlib2.h"
#include "utils/uartstdio.h"
#include "FT800_TIVA.h"

// =======================================================================
// DEFINICIONES
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

#define SLEEP SysCtlSleepFake()

enum estados { Inicio, Recarga, Mañana, Tarde, Noche, AvisoM, AvisoT, AvisoN, EsperoN };
enum estados estado;

// =======================================================================
// VARIABLES GLOBALES
// =======================================================================
unsigned long REG_TT[6];
const int32_t REG_CAL[6] = { CAL_DEFAULTS };

int RELOJ, PeriodoPWM;
volatile int Flag_ints = 0;
volatile int posicion;
volatile int Max_pos = 4200;
volatile int Min_pos = 1300;

int dias, horas, minutos, segundos;
int p; // contador segundos
char *dias_semana[] = {"Lunes","Martes","Miercoles","Jueves","Viernes","Sabado","Domingo"};
// Estados de los botones
bool estadoManana = false;
bool estadoTarde = false;
bool estadoNoche = false;

// =======================================================================
// FUNCIONES AUXILIARES
// =======================================================================
void flushUART(void) {
    while(UARTCharsAvail(UART0_BASE))
        UARTCharGetNonBlocking(UART0_BASE);
}

void UART_SendString(char *str) {
    while (*str) UARTCharPut(UART0_BASE, *str++);
}

int UART_ReadInt(void) {
    char buffer[6];
    int i = 0;
    while (1) {
        char c = UARTCharGet(UART0_BASE);
        if (c == '\r' || c == '\n') break;
        buffer[i++] = c;
    }
    buffer[i] = '\0';
    return atoi(buffer);
}

void set_color_verde(void){ ComColor(0,255,0); }
void set_color_azul(void){ ComColor(0,0,255); }
void set_color_blanco(void){ ComColor(255,255,255); }
void set_color_negro(void){ ComColor(0,0,0); }

void SysCtlSleepFake(void) {
    while(!Flag_ints);
    Flag_ints = 0;
}

void IntTimer0(void);

// =======================================================================
// MAIN
// =======================================================================
int main(void) {
    int i;

    // === Configuración hardware (igual que tenías) ===
    RELOJ = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN |
             SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480), 120000000);
    HAL_Init_SPI(1, RELOJ);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);

    PWMClockSet(PWM0_BASE, PWM_SYSCLK_DIV_64);

    GPIOPinConfigure(GPIO_PF1_M0PWM1);
    GPIOPinConfigure(GPIO_PF2_M0PWM2);
    GPIOPinConfigure(GPIO_PF3_M0PWM3);
    GPIOPinConfigure(GPIO_PG0_M0PWM4);

    GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
    GPIOPinTypePWM(GPIO_PORTG_BASE, GPIO_PIN_0);

    PWMGenConfigure(PWM0_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
    PWMGenConfigure(PWM0_BASE, PWM_GEN_1, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
    PWMGenConfigure(PWM0_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);

    PeriodoPWM = 37499; // 50Hz
    posicion = Min_pos;
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, PeriodoPWM);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, PeriodoPWM);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, PeriodoPWM);

    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, posicion);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, posicion);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, posicion);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, posicion);

    PWMGenEnable(PWM0_BASE, PWM_GEN_0);
    PWMGenEnable(PWM0_BASE, PWM_GEN_1);
    PWMGenEnable(PWM0_BASE, PWM_GEN_2);

    PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT | PWM_OUT_2_BIT |
                   PWM_OUT_3_BIT | PWM_OUT_4_BIT, true);

    // === Timer y UART (igual que antes) ===
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    TimerClockSourceSet(TIMER0_BASE, TIMER_CLOCK_SYSTEM);
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    TimerLoadSet(TIMER0_BASE, TIMER_A, (RELOJ/50)-1);
    TimerIntRegister(TIMER0_BASE,TIMER_A,IntTimer0);
    IntEnable(INT_TIMER0A);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    IntMasterEnable();
    TimerEnable(TIMER0_BASE, TIMER_A);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTStdioConfig(0, 115200, RELOJ);

    Inicia_pantalla();
    SysCtlDelay(RELOJ/3);

    // === INTERFAZ ===
    Nueva_pantalla(0x10,0x10,0x10);
    set_color_azul(); ComRect(0,0,HSIZE,VSIZE,true);
    set_color_blanco(); ComRect(53,30,265,90,true);

    set_color_blanco(); ComRect(20,140,96,220,true);
    set_color_negro(); ComTXT(53,180,26,OPT_CENTER,"MANANA");
    set_color_blanco(); ComRect(116,140,192,220,true);
    set_color_negro(); ComTXT(149,180,26,OPT_CENTER,"TARDE");
    set_color_blanco(); ComRect(212,140,288,220,true);
    set_color_negro(); ComTXT(245,180,26,OPT_CENTER,"NOCHE");
    Dibuja();

    for(i=0;i<6;i++) Esc_Reg(REG_TOUCH_TRANSFORM_A+4*i, REG_CAL[i]);
    estado = Inicio;

    // === Bucle principal ===
    while(1) {
        bool esManana = (horas >= 8  && horas < 10);
        bool esTarde  = (horas >= 14 && horas < 16);
        bool esNoche  = (horas >= 20 && horas < 22);

        // --- Lógica de estados ---
        switch(estado) {
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
            posicion = Max_pos;
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, posicion);
            SysCtlDelay(SysCtlClockGet() / 3);
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, Min_pos);

            if (horas >= 4 && horas < 12) estado = Mañana;
            else if (horas >= 12 && horas < 18) estado = Tarde;
            else estado = Noche;
            break;

        case Mañana:
        case Tarde:
        case Noche:
            // Color verde según hora
            estadoManana = esManana;
            estadoTarde  = esTarde;
            estadoNoche  = esNoche;

            // --- Dibujar botones ---
            if (estadoManana) set_color_verde(); else set_color_blanco();
            ComRect(20,140,96,220,true);
            set_color_negro(); ComTXT(53,180,26,OPT_CENTER,"MANANA");

            if (estadoTarde) set_color_verde(); else set_color_blanco();
            ComRect(116,140,192,220,true);
            set_color_negro(); ComTXT(149,180,26,OPT_CENTER,"TARDE");

            if (estadoNoche) set_color_verde(); else set_color_blanco();
            ComRect(212,140,288,220,true);
            set_color_negro(); ComTXT(245,180,26,OPT_CENTER,"NOCHE");

            // Día de la semana
            set_color_negro();
            ComTXT(160,60,28,OPT_CENTER, dias_semana[dias]);

            break;

        default:
            break;
        }

        Dibuja();

    }
}

// =======================================================================
// INTERRUPCIÓN TIMER
// =======================================================================
void IntTimer0(void) {
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    SysCtlDelay(20);
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
                if (horas >= 24) {
                    horas = 0;
                    dias++;
                    if (dias >= 7) dias = 0;
                }
            }
        }
    }
}
