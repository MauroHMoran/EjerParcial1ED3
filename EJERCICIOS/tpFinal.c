#ifdef _USE_CMSIS
#include "LPC17xx.h"
#endif

#ifdef __USE_MCUEXPRESSO
#include <cr_section_macros.h>
#endif

#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_exti.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_timer.h"

#include  <math.h> 

#define ALTO 1
#define BAJO 0

#define ENABLE 1
#define DISABLE 0

#define OUTPUT 1
#define INPUT 0

#define RISE 1
#define FALL 0

#define TIMER_VALUE 1000    /*el contador aumenta cada 1ms*/
#define CHANNEL0 0
#define CHANNEL1 1

/*macros*/
#define LED1 ((uint32_t)(1<<18))
#define LED2 ((uint32_t)(1<<19))
#define LED3 ((uint32_t)(1<<20))

uint8_t PrimerCap[10]={0,0,0,0,0,0,0,0,0,0};
uint8_t SegundoCap[10]={0,0,0,0,0,0,0,0,0,0};
uint8_t Contador1=0;
uint8_t Contador2=0;
uint8_t Promedio1=0;
uint8_t Promerio2=0;

float fdp=0;

/*configuration functions*/

void cfg_ports(void)
{
    PINSEL_CFG_Type port;

    /*AD0[7] en P0.2*/
    port.Portnum=PINSEL_PORT_0;
    port.Pinnum=PINSEL_PIN_2;
    port.Funcnum=PINSEL_FUNC_2;
    port.Pinmode=PINSEL_PINMODE_PULLUP;
    port.OpenDrain=PINSEL_PINMODE_NORMAL;
    PINSEL_ConfigPin(&port);

    /*AD0[6] en P0.3*/
    port.Pinnum=PINSEL_PIN_3;
    PINSEL_ConfigPin(&port);

    /*salida para leds en P0.18-20*/
    port.Pinnum=PINSEL_PIN_18;
    port.Funcnum=PINSEL_FUNC_0;
    PINSEL_ConfigPin(&port)
    port.Pinnum=PINSEL_PIN_19;
    PINSEL_ConfigPin(&port);
    port.Pinnum=PINSEL_PIN_20;
    PINSEL_ConfigPin(&port);
    GPIO_SetDir(PINSEL_PORT_0, LED1|LED2|LED3, OUTPUT);

    /*cap0[0] en P1.26*/
    port.Portnum=PINSEL_PORT_1;
    port.Pinnum=PINSEL_PIN_26;
    port.Funcnum=PINSEL_FUNC_3;
    PINSEL_ConfigPin(&port);

    /*cap0[1] en P1.27*/
    port.Pinnum=PINSEL_PIN_27;
    PINSEL_ConfigPin(&port);
}

/*matches timer configuration
*CONSIDERAR: El tiempo que tarda en:
*   1) Interrumpir y salvar el contexto (tener en cuenta para el 2do match)
*   2) El tiempo que va a tardar en la rutina de interrupcion (en ambos matchs)
*   3) Como pasamos por diodos la señal, solo vamos a tener un rising edge en 20ms
*/
void cfg_tmr_cap(void)
{
    TIM_TIMERCFG_Type tmr;
    tmr.PrescaleOption=TIM_PRESCALE_USVAL;
    tmr.PrescaleValue=TIMER_VALUE;
    
    TIM_CAPTURECFG_Type cap;
    cap.CaptureChannel=0;
    cap.RisingEdge=ENABLE;
    cap.FallingEdge=DISABLE;
    cap.IntOnCaption=ENABLE;
    TIM_ConfigCapture(LPC_TIM0, &cap);
    cap.CaptureChannel=1;
    TIM_ConfigCapture(LPC_TIM0, &cap);
    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &tmr);

    TIM_MATCHCFG_Type match;
    match.MatchChannel=CHANNEL0;
    match.IntOnMatch=ENABLE;
    match.ResetOnMatch=ENABLE;
    match.StopOnMatch=DISABLE;
    match.ExtMathOutputType=TIM_EXTMATCH_NOTHING;
    match.MatchValue=210;       //interrumpe cada 210ms (mas de 10 periodos de señal)
    TIM_ConfigMatch(LPC_TIM1, &match);
    TIM_Init(LPC_TIM1, TIM_TIMER_MODE, &tmr);

    TIM_Cmd(LPC_TIM0, ENABLE);
    TIM_Cmd(LPC_TIM1, ENABLE);
    NVIC_EnableIRQ(TIMER1_IRQn);
    NVIC_EnableIRQ(TIMER0_IRQn);
}

/*IRQ functions*/
void TIMER0_IRQHandler(void)
{
    if(TIM_GetIntCaptureStatus(LPC_TIM0, TIM_CR0_INT))
    {
        if(Contador1<10)
        {   
            PrimerCap[Contador1]=TIM_GetCaptureValue(LPC_TIM0, CHANNEL0);
            Contador1++;
        }
        else if (Contador1==10)
        {
            Contador1=0;
        }
    }

    else if(TIM_GetIntCaptureStatus(LPC_TIM0, TIM_CR1_INT))
    {
        if(Contador2<10)
        {
            SegundoCap[Contador2]=TIM_GetCaptureValue(LP_TIM0, CHANNEL1);
            Contador2++;
            TIM_ResetCounter(LPC_TIM0);
        }
        else if(Contador2==10)
        {
            Contador2=0;
            TIM_ResetCounter(LPC_TIM0);
        }
    }
    TIM_ClearIntCapturePending(LPC_TIM0, TIM_CR0_INT|TIM_CR1_INT);
}

/*cada cierto tiempo (al menos 210ms (mas de10 perdiodos de señal)) interrumpir para hacer promedio
* y limpiar los valores de los arrays
*/

void TIMER1_IRQHandler(void)
{   
    NVIC_DisableIRQ(TIMER0_IRQn);
    average_times();
    fdp_calculate();
    show_led();

    TIM_ClearIntPending(LPC_TIM1, TIM_MR0_INT);
    clear_capture_arrays();
    TIM_ResetCounter(LPC_TIM0);
    NVIC_EnableIRQ(TIMER0_IRQn);
}


/*funciones auxiliares*/
void clear_capture_arrays(void)
{
    for (uint8_t i=0; i<10, i++)
    {
        PrimerCap[i]=0;
        SegundoCap[i]=0;
    }
}

void average_times(void)
{
    for (uint8_t i=0; i<10, i++)
    {
        Promedio1=PrimerCap[i] 
        Promedio2=SegundoCap[i];
    }
    Promedio1/=10;
    Promedio2/=10;
}

void fdp_calculate(void)
{
    fdp=cos(360*(Promedio1-Promedio2)/20);
}

void show_led(void)
{
    if(fdp>0.75 && fdp<1)
    {
        GPIO_ClearValue(PINSEL_PORT_0, LED1|LED2|LED3);
        GPIO_SetValue(PINSEL_PORT_0,LED1);
    }
    else if (fdp<0.75 && fdp>0.25)
    {
        GPIO_ClearValue(PINSEL_PORT_0, LED1|LED2|LED3);
        GPIO_SetValue(PINSEL_PORT_0, LED2);
    }
    else if (fdp<0.25)
    {
        GPIO_ClearValue(PINSEL_PORT_0, LED1|LED2|LED3);
        GPIO_SetValue(PINSEL_PORT_0, LED3);
    }
}