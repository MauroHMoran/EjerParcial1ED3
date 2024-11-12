/*1) Un estacionamiento automatizado utiliza una barrera que se abre y cierra en función de la validación de un ticket de acceso
utilizando una LPC1769 Rev. D trabajando a una frecuencia de CCLK a 70 [MHz]

Cuando el sistema detecta que un automóvil se ha posicionado frente a la barrera, se debe activar un sensor conectado
al pin P2[10] mediante una interrupción externa (EINT). Una vez validado el ticket, el sistema activa un motor que abre
la barrera usando el pin P0[15]. El motor debe estar activado por X segundos y luego apagarse, utilizando el temporizador Systick
para contar el tiempo. Si el ticket es inválido, se encenderá un LED rojo conectado al pin P1[5].

Para gestionar el tiempo de apertura de la barrera, existe un switch conectado al pin P2[11] que dispone de una ventana
de configuración de 3 segundos gestionada por el temporizador Systick.
Durante dicha ventana, se debe contar cuantas veces se presiona el switch y en función de dicha cantidad, establecer el
tiempo de la barrera. Siendo:
    0 clicks:  6 segundos
    1 clicks:  8 segundos
    2 clciks: 10 segundos
    3 clicks: 12 segundos

como el clock es de 70MHz, el maximo tiempo con el que puedo cargar el systick es de: 239,67ms
*/

/*  falta agregar lo del led rojo, pero pasa que como el sensor interrumpe cuando es valido, en todo caso siempre deberia estar prendido 
hasta que el sensor interrumpa. Y volverlo a prender en algun momento*/

#ifdef _USE_CMSIS
#include "LPC17xx.h"
#endif

#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_exti.h"
#include "lpc17xx_systick.h"

#define ALTO 1
#define BAJO 0

#define ENABLE 1
#define DISABLE 0

#define OUTPUT 1
#define INPUT 0

#define RISE 1
#define FALL 0

/*declaracion de pines*/
#define SENSOR      ((uint32_t)(1<<10))       //pin del sensor P2.10
#define MOTOR       ((uint32_t)(1<<15))       //pin del motor P0.15
#define LED_ROJO    ((uint32_t)(1<<5))        //pin del led rojo P1.5
#define SWTICH      ((uint32_t)(1<<11))       //swtich conectado a P2.11  

/*declaracion del tiempo del systick en milisegundos*/
#define SYSTICK_TIME    200

/*variables globales*/
static uint8_t ticket_valido=0;
static uint8_t clicks_switch=0;
static uint8_t tres_seg=15;
static uint8_t tiempo=0;


/*funcion principal*/
int main()
{
    SystemInit();
    cfg_ports();
    cfg_exti();
    cfg_systick();
    /*prioridad de las interrupciones*/
    NVIC_SetPriority(EINT1_IRQn,0);
    NVIC_SetPriority(Systick_IRQn,1);
    NVIC_SetPriority(EINT0_IRQn,2);

    while(1)
    {
        asm _nop;       //waiting for interruption;       
    }
}



/*declaracion de funciones de configuracion*/

void cfg_ports(void)
{
    PINSEL_CFG_Type port;
    port.Portnum=PINSEL_PORT_0;
    port.Pinnum=PINSEL_PIN_15;
    port.Funcnum=PINSEL_FUNC_0;
    port.Pinnmode=PINSEL_PINMODE_PULLUP;
    port.OpenDrain=PINSEL_PINMODE_NORMAL;
    PINSEL_ConfigPin(&port);

    port.Portnum=PINSEL_PORT_1;
    port.Pinnum=PINSEL_PIN_5;
    PINSEL_ConfigPin(&port);

    GPIO_SetDir(PINSEL_PORT_0, MOTOR, OUTPUT);
    GPIO_SetDir(PINSEL_PORT_1, LED_ROJO, OUTPUT);

    port.Portnum=PINSEL_PORT_2;
    port.Pinnum=PINSEL_PIN_10;
    port.Funcnum=PINSEL_FUNC_1;
    PINSEL_ConfigPin(&port);

    port.Pinnum=PINSEL_PIN_11;
    PINSEL_ConfigPin(&port);
}

void cfg_systick(void)
{
    SYSTICK_InternalInit(SYSTICK_TIME);
    SYSTICK_Cmd(ENABLE);
    SYSTICK_IntCmd(ENABLE);
    
}

void cfg_exti(void)
{
    EXTI_InitTypeDef exti;
    exti.EXTI_Line=EXTI_EINT0;
    exti.EXTI_Mode=EXTI_MODE_LEVEL_SENSITIVE;
    exti.EXTI_polarity=EXTI_POLARITY_LOW_ACTIVE_OR_FALLING_EDGE;
    EXTI_Config(&exti);
    
    exti.EXTI_LINE=EXTI_EIN1;
    EXTI_Config(&exti);

    NVIC_EnableIRQ(EINT0_IRQn);
    NVIC_EnableIRQ(EINT1_IRQn);
}

/*funciones de interrupcion*/

void EINT0_IRQHandler(void)
{
    EXTI_ClearEXTIFlag(EXTI_EINT0);
    ticket_valido^=1;
}

void EINT1_IRQHandler(void)
{
    EXTI_ClearEXTIFlag(EXTI_EINT1);
    clicks_switch++;
}

void SYSTICK_IRQHandler(void)
{
    SYSTICK_ClearCounterFlag();
    if(tres_seg!=0)
    {
        NVIC_EnableIRQ(EINT1_IRQn);
        tres_seg--;
    }
    else
    {
        NVIC_DisableIRQ(EINT1_IRQn);

        configurar_tiempo(tiempo);

        if(tiempo!=0)
        {
            encender_motor();
            tiempo--;
        }
        else
        {
            apagar_motor();
            tres_seg=15;
            ticket_valido^=1;
            clicks_switch=0;
        }
    }
}

/*funciones auxiliares*/
int configurar_tiempo(uint8_t num)
{
    switch(clicks_switch)
    {
        case 0:
            num=30;
            break;
        case 1:
            num=40;
            break;
        case 2:
            num=50;
            break;
        default:
            num=60;
            break;
    }
    return num;
}

void encender_motor(void)
{
    GPIO_SetValue(PINSEL_PORT_0, MOTOR);
}

void apagar_motor(void)
{
    GPIO_ClearValue(PINSEL_PORT_0, MOTOR);
}
