/*
2) En una fábrica, hay un sistema de alarma utilizando una LPC1769 Rev. D trabajando a una frecuencia de CCLK a 100 [MHz],
conectado a un sensor de puerta que se activa cuando la puerta se abre.

El sensor está conectado al pin P0[6], el cual genera una interrupción externa (EINT) cuando se detecta una apertura
(cambio de estado). Al detectar que la puerta se ha abierto, el sistema debe iniciar un temporizador utilizando el Systick
para contar un período de 30 segundos.

Durante estos 30 segundos, el usuario deberá introducir un código de desactivación mediante un DIP switch de 4 entradas
conectado a los pines P2[0] - P2[3]. El código correcto es 0xAA (1010 en binario). El usuario tiene dos intentos para
introducir el código correcto. Si después de dos intentos el código ingresado es incorrecto, la alarma se activará,
encendiendo un buzzer conectado al pin P1[11]
*/

/*> <* g h */

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


#define SENSOR  ((uint32_t)(1<<6))  //Sensor en P0.6
#define ALARMA  ((uint32_t)(1<<11)) //alarama en P1.11
/*pines para el codigo en el puerto2*/
#define PIN0    ((uint32_t)(1<<0))
#define PIN1    ((uint32_t)(1<<1))
#define PIN2    ((uint32_t)(1<<2))
#define PIN3    ((uint32_t)(1<<3))

#define ENTER   ((uint32_t)(1<<10))  //pin para el enter del codigo P2.10

#define CODIGO  ((uint32_t)(0xA<<0))    

#define SYSTICK_TIME 100            //tiempo maximo para systick es 100ms

/*variables globales*/
static uint16_t treinta=300;

/*funciones de configuracion*/
void cfg_ports(void)
{
    PINSEL_CFG_Type ports;
    ports.Portnum=PINSEL_PORT_0;
    ports.Pinnum=PINSEL_PIN_6;
    ports.Funcnum=PINSEL_FUNC_0;
    ports.Pinmode=PINSEL_PINMODE_PULLUP;
    ports.OpenDrain=PINSEL_PINMODE_NORMAL;
    PINSEL_ConfigPin(&ports);

    ports.Portnnum=PINSEL_PORT_1;
    ports.Pinnum=PINSEL_PIN_11;
    PINSEL_ConfigPin(&ports);

    ports.Portnum=PINSEL_PORT_2;
    ports.Pinnum=PINSEL_PIN_0;
    PINSEL_ConfigPin(&ports);

    ports.Pinnum=PINSEL_PIN_1;
    PINSEL_ConfigPin(&ports);

    ports.Pinnum=PINSEL_PIN_2;
    PINSEL_ConfigPin(&ports);

    ports.Pinnum=PINSEL_PIN_3;
    PINSEL_ConfigPin(&ports);

    ports.Pinnum=PINSEL_PIN_10;
    ports.Funcnum=PINSEL_FUNC_1;
    PINSEL_ConfigPin(&ports);

    GPIO_SetDir(PINSEL_PORT_0, SENSOR, INPUT);
    GPIO_IntCmd(PINSEL_PORT_0, SENSOR, FALL);
    GPIO_SetDir(PINSEL_PORT_1, ALARMA, OUTPUT);
    GPIO_SetDir(PINSEL_PORT_2, PIN0|PIN1|PIN2|PIN3, INPUT);
}

void cfg_exti(void)
{
    EXTI_InitTypeDef exti;
    exti.EXTI_Line=EXTI_EINT0;
    exti.EXTI_Mode=EXTI_MODE_EDGE_SENSITIVE;
    exti.EXTI_polarity=EXTI_POLARITY_LOW_ACTIVE_OR_FALLING_EDGE;
}

void cfg_systick(void)
{
    SYSTICK_InternalInit(SYSTICK_TIME);
    SYSTICK_IntCmd(ENABLE);
}

/*funciones de rutinas de interrupcion*/

void EINT3_IRQHandler(void)
{
    if(GPIO_GetIntStatus(PINSEL_PORT_0, SENSOR, FALL))
    {
        GPIO_ClearInt(PINSEL_PORT_0, SENSOR);
        SYSTICK_Cmd(ENABLE);
    }
}

void SYSTICK_IRQHandler(void)
{
    SYSTICK_ClearCounterFlag();
    treinta--;
}

void EINT0_IRQHandler(void)
{
    uint8_t codigo_correcto=0;
    EXTI_ClearEXTIFlag(EXTI_EINT0);
    intentos--;
    chequear_codigo(codigo_correcto);

    if(treinta>0 && intentos>0 && codigo_correcto)
    {
        GPIO_ClearValue(PINSEL_PORT_1, ALARMA);
        intentos=2;
        treinta=300;
        NVIC_DisableIRQ(SysTick_IRQn);
    }
    else if (treinta>0 && intentos==0 && !codigo_correcto)
    {
        GPIO_SetValue(PINSEL_PORT_1, ALARMA);
        NVIC_DidableIRQ(SysTick_IRQn);
    }
    else if(treinta==0 && intentos>0 && !codigo_correcto)
    {
        GPIO_SetValue(PINSEL_PORT_1, ALARMA);
        NVIC_DidableIRQ(SysTick_IRQn);
    }
}

/*funciones auxiliares*/
uint8_t chequear_codigo(uint8_t codigo);
{
    if((GPIO_ReadValue(PINSEL_PORT_0)^CODIGO)==0;)
    {
        return codigo=1;
    }
    else
    {
        return codigo=0;
    }

}
