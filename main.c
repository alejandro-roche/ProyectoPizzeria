#include <msp430.h> 
#include "grlib.h"
#include "Crystalfontz128x128_ST7735.h"
#include "HAL_MSP430G2_Crystalfontz128x128_ST7735.h"
#include <stdio.h>
#include "uart_STDIO.h"
#define xmin 16
#define xmax 112
#define precio_base 5
#define tiempo_base 10
#define qmax 3
#define jmax 3
#define pimax 3
#define pemax 3



//-------Maquina de estados------------//
enum{
    Principal,          //Seleccion del ingrediente
    Informacion,        //Precio ingrediente
    Aumentar_Anadir,    //Aumentar precio total y añadir ingrediente
    Decision,           //Decision de poner ingrediente o no
    Libre,              //Estado que calcula el tiempo de horno y da turno al cliente
    Horno,              //Se envia pizza al horno
    Espera,             //Estado de espera cuando ya se han hecho 2 pizzas y hay una en el horno
    Listo,              //Pizza lista
    Recibe_Pedido       //Estado de "Espera" de la cocina a la espera de que le llegue un pedido
};

char Estados=Principal;
char Cocina=Recibe_Pedido;

Graphics_Context g_sContext;

//---------Llamada a funciones----------//
void Set_Clock(char VEL);
int lee_ch(char canal);
void inicia_ADC(char canales);
void guarda_flash(char *dato,unsigned int direc);
void dibuja_pizza_vacia();
void dibuja_cantidades();

//---------Declaracion de variables-----//--------------//-------------------------------------------NOTA IMPORTANTE:---------------------------------//
int tms;                                                                               //Esta todo definido con variables, tanto la cantidad
int seg;                                                                               //de producto por pizza, como el numero de pedidos etc
int tseg;                                   //Variable que cuenta segundos             //Por tanto, todo es modificable según el criterio del usuario
int tseg2;                                  //Variable que cuenta segundos

char cadena[75];                            //Variable que se escribe por puerto serie
volatile char caracter=0;                   //Variable para lee por puerto serie

char cantidad_q=0;                          //Cantidad de ingrediente en cocina
char cantidad_j=2;                          //Cantidad de ingrediente en cocina
char cantidad_pi=3;                         //Cantidad de ingrediente en cocina
char cantidad_pe=4;                         //Cantidad de ingrediente en cocina

char cq=0;                                  //Cantidad de ingrediente en la pizza
char cj=0;                                  //Cantidad de ingrediente en la pizza
char cpi=0;                                 //Cantidad de ingrediente en la pizza
char cpe=0;                                 //Cantidad de ingrediente en la pizza
char cant[2];                               //Cantidad de ingrediente en la pizza que escribimos por pantalla

int precio=precio_base;                     //Precio total de la pizza
char precios[10]= {0,0,0,0,0,0,0,0,0,0};    //Vector de precios
char sump=0;                                //Suma de todos los precios
int pm=0;                                   //Precio medio de los pedidos
char j;                                     //Variable para movernos por el vector de precios[j]

char hueco=1;                               //Hueco de horno
char pedido=0;                              //Variable que se activa en el momento que se registra un pedido
char puestos=2;                             //Numero de puesto de trabajo disponibles

int k=0;                                    //Turno del cliente

int pedidos[10]={0,0,0,0,0,0,0,0,0,0};      //Vector que guarda el tiempo de cada pizza, depende de su tamaño podremos hacer mas o menos pedidos en el dia
char n;                                     //Variable para movernos a traves del vector pedidos[n]

int tiempo_espera=0;                        //Tiempo de espera en los puestos de trabajo
int tiempo_pantalla=0;                      //Tiempo de espera mostrado por pantalla
int tiempo_espera_pizza=0;                  //Tiempo que le queda a la pizza en el horno


//Selector de ingredientes
int x;                                      //Variable para moverte entre ingredientes
int ejex;                                   //variable que cambia a medida que movemos el joystick en el eje x
int pos=16;


int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;           // stop watchdog timer
    FCTL2 = FWKEY + FSSEL_2 + 34;

    //----------Timer------------------//

    Set_Clock(16);
    TA0CTL=TASSEL_2|ID_3| MC_3;         //SMCLK, DIV=8 (2MHz), UP/DOWN
    TA0CCTL0=CCIE;                      //CCIE=1
    TA0CCR0=49999;                      //periodo=100 000 -->50 ms

    //------ Pines de E/S involucrados---//
    P1SEL2 = BIT1 | BIT2;  //P1.1 RX, P1.2: TX
    P1SEL = BIT1 | BIT2;
    P1DIR = BIT0+ BIT2;
    P1OUT &=~BIT0;
    P2DIR &= ~BIT5;
    P2OUT |= BIT5;


    //------ Configuración de la USCI-A para modo UART:----------//

    UCA0CTL1 |= UCSWRST;                    // Reset
    UCA0CTL1 = UCSSEL_2 | UCSWRST;          //UCSSEL_2: SMCLK (16MHz) | Reset on
    UCA0CTL0 = 0;                           // 8bit, 1stop, sin paridad. NO NECESARIO
    UCA0BR0 = 139;                          // 16MHz/139=115108...
    UCA0CTL1 &= ~UCSWRST;                   /* Quita reset */
    IFG2 &= ~(UCA0RXIFG);                   /* Quita flag */
    IE2 |= UCA0RXIE;                        /* y habilita int. recepcion */


    dibuja_pizza_vacia();
    dibuja_cantidades();
    sprintf(cadena,"\n\rBienvenido a PizzaBuffet Arazar, escoja sus ingredientes\n\r");
    UARTprint(cadena);


    inicia_ADC(BIT0+BIT3);                  //Funcion para el joystick
    __bis_SR_register(GIE);                 // Habilita las interrupciones globalmente

    while(1)
    {
        LPM0;


        //-------------------Mostrar el precio medio---------------------------
        if(caracter == 'q'){

            caracter=0;
            if(k>0){
                sump=0;
                for(j=0;j<10;j++){
                    sump+=precios[j];
                }
                pm=sump/k;
            }
            else pm=0;
            sprintf(cadena,"\n\rEl precio medio de los pedidos es: %d\n\r",pm);
            UARTprint(cadena);
        }

        //--------------------Recargar ingredientes--------------------------

        if(cantidad_q==0 && caracter=='r')
        {
            cantidad_q=cantidad_q+2;
            sprintf(cadena,"\n\rSe ha recargado el queso\n\r");
            UARTprint(cadena);
        }
        else if(cantidad_j==0 && caracter=='r')
        {
            cantidad_j=cantidad_j+2;
            sprintf(cadena,"\n\rSe ha recargado el Jamon York\n\r");
            UARTprint(cadena);
        }
        else if(cantidad_pi==0 && caracter=='r')
        {
            cantidad_pi=cantidad_pi+2;
            sprintf(cadena,"\n\rSe ha recargado el pimiento\n\r");
            UARTprint(cadena);
        }
        else if(cantidad_pe==0 && caracter=='r')
        {
            cantidad_pe=cantidad_pe+2;
            sprintf(cadena,"\n\rSe ha recargado el peperonni\n\r");
            UARTprint(cadena);
        }

        //--------------Máquina:Modificador de pizza-------------------

        switch(Estados)
        {
        case Principal:

            ejex=lee_ch(0);                     //leemos el valor del joystick en el eje x

            dibuja_cantidades();

            //Cambiamos de color con la variable "x" según se modifica ejey
            if(ejex>=574) x+=32;
            else if(ejex>=450) x+=0;
            else if(ejex>=0) x-=32;


            //-------------------------Selector-------------------------------
            //Saturacion
            if(x>xmax) x=xmin;
            if(x<xmin) x=xmax;


            if(x!=pos)
            {
                //Borramos posicion anterior dibujando verde
                Graphics_setForegroundColor(&g_sContext,GRAPHICS_COLOR_LIGHT_GREEN);
                Graphics_fillCircle(&g_sContext,pos,32,3);

                //Dibujamos bola en la siguiente posición
                Graphics_setForegroundColor(&g_sContext,GRAPHICS_COLOR_WHITE);
                Graphics_fillCircle(&g_sContext,x,32,3);
                //Actualizamos variable
                pos=x;
            }

            //------------Se selecciona el ingrediente-------------

            if(!(P2IN&BIT5)) Estados=Informacion;
            else Estados=Principal;

            //-----------------Se realiza el pedido---------------------------

            if(puestos>=1 && caracter=='d')
            {

                caracter=0;

                pedidos[k]=tiempo_base+5+(cq+cj+cpi+cpe)*2;          //Tiempo base mas 5 de horno mas 2 segundos por ingrediente

                if(puestos>1)                                        //Caso que haya o una pizza en el horno o una en el horno y otra en un puesto de trabajo
                {
                    tiempo_espera=pedidos[k];
                }
                else
                {
                    tiempo_espera=pedidos[k]+pedidos[k-1];
                }

                tiempo_espera=tiempo_espera+tiempo_espera_pizza;    //Tiempo de espera en los puestos de trabajo mas el tiempo que le queda a la pizza en el horno
                                                                    //Para el caso que no haya ninguna pizza en el horno ni en los puestos de trabajo el tiempo_espera_pizza será nula

                puestos--;                                          //restamos un puesto de trabajo
                tiempo_pantalla=tiempo_espera;


                sprintf(cadena,"\n\rSu turno es el %d y la pizza va a tardar %d segundos\n\r",k,tiempo_espera);
                UARTprint(cadena);

                if(tiempo_pantalla==0)
                {
                    tseg2=0;
                }

                //Vaciamos la pizza para poder crear otra

                cq=0;
                cj=0;
                cpi=0;
                cpe=0;

                precios[k]=precio;
                guarda_flash(precios,0x1000);
                precio=precio_base;

                k++;                                                //Aumentamos el turno

                dibuja_pizza_vacia();
                dibuja_cantidades();
                sprintf(cadena,"\n\rBienvenido a PizzaBuffet Arazar, escoja sus ingredientes\n\r");
                UARTprint(cadena);

                pedido=1;                                           //Variable que activa la maquina de estados de la cocina


                if(hueco>0)                                         //Horno libre, metemos pizza
                {
                    Estados=Principal;                              //Editamos siguiente pizza
                }
                else if(puestos!=0)                                 //No queda hueco en el horno pero quedan aun puestos de trabajo
                {
                    sprintf(cadena,"\n\rEl horno esta ocupado,espere por favor \n\r");
                    UARTprint(cadena);
                    Estados=Principal;                              //Volvemos a editar siguiente pizza
                }
                else                                                //No quedan puestos de trabajo
                {
                    sprintf(cadena,"\n\rNo quedan mas puestos de trabajo, espere por favor \n\r");
                    UARTprint(cadena);
                    Estados=Espera;                                 //Vamos a un estado de espera donde solo podemos esperar a que se libere un puesto de trabajo
                }
            }

            break;

        case Espera:
            caracter=0;
            pedido=1;
            if(puestos>0)
            {
                Estados=Principal;
            }
            break;

        case Informacion:

            caracter=0;
            if(pos==16)
            {
                if(cq!=qmax)
                {
                    if(cantidad_q>0)
                    {
                        sprintf(cadena,"\n\rQuedan %d unidades de queso\n\rPrecio de la unidad 1 euros\n\r ",cantidad_q);
                        UARTprint(cadena);
                        Estados=Decision;
                    }
                    else
                    {
                        sprintf(cadena,"\n\rNo quedan unidades de este producto\n\r");
                        UARTprint(cadena);
                        Estados=Principal;
                    }
                }
                else
                {
                    sprintf(cadena,"\n\rHas alcanzado el limite de este producto\n\r");
                    UARTprint(cadena);
                    Estados=Principal;
                }
            }


            if(pos==48)
            {
                if(cj!=jmax)
                {
                    if(cantidad_j>0)
                    {
                        sprintf(cadena,"\n\rQuedan %d unidades de jamon York\n\rPrecio de la unidad 2 euros\n\r",cantidad_j);
                        UARTprint(cadena);
                        Estados=Decision;
                    }
                    else
                    {
                        sprintf(cadena,"\n\rNo quedan unidades de este producto\n\r");
                        UARTprint(cadena);
                        Estados=Principal;
                    }

                }
                else
                {
                    sprintf(cadena,"\n\rHas alcanzado el limite de este producto\n\r");
                    UARTprint(cadena);
                    Estados=Principal;
                }
            }


            if(pos==80)
            {
                if(cpi!=pimax)
                {
                    if(cantidad_pi>0)
                    {
                        sprintf(cadena,"\n\rQuedan %d unidades de pimientos\n\rPrecio de la unidad 1 euros\n\r",cantidad_pi);
                        UARTprint(cadena);
                        Estados=Decision;
                    }
                    else
                    {
                        sprintf(cadena,"\n\rNo quedan unidades de este producto\n\r");
                        UARTprint(cadena);
                        Estados=Principal;
                    }

                }
                else
                {
                    sprintf(cadena,"\n\rHas alcanzado el limite de este producto\n\r");
                    UARTprint(cadena);
                    Estados=Principal;
                }

            }

            if(pos==112)
            {
                if(cpe!=pemax)
                {
                    if(cantidad_pe>0)
                    {
                        sprintf(cadena,"\n\rQuedan %d unidades de peperonni\n\rPrecio de la unidad 3 euros\n\r",cantidad_pe);
                        UARTprint(cadena);
                        Estados=Decision;
                    }
                    else
                    {
                        sprintf(cadena,"\n\rNo quedan unidades de este producto\n\r");
                        UARTprint(cadena);
                        Estados=Principal;
                    }

                }
                else
                {
                    sprintf(cadena,"\n\rHas alcanzado el limite de este producto\n\r");
                    UARTprint(cadena);
                    Estados=Principal;
                }
            }


            break;

        case Decision:

            if(!(P2IN & BIT5))Estados=Aumentar_Anadir;
            else if(caracter=='c')
            {
                sprintf(cadena,"\n\rOperacion cancelada\n\r");
                UARTprint(cadena);
                caracter=0;
                Estados=Principal;
            }


            break;

        case Aumentar_Anadir:
            if(pos==16)
            {
                precio+=1;                             //Aumentamos precio
                cantidad_q-=1;                         //Cantidad del producto se reduce
                Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_YELLOW);
                Graphics_fillCircle(&g_sContext,51,65,8);
                cq++; //Cantidad de ingrediente en la pizza aumenta
                Estados=Principal;
                sprintf(cadena,"\n\rEl precio de su pedido es de %d euros.\n\r",precio);
                UARTprint(cadena);

            }
            if(pos==48)
            {
                precio+=2;
                cantidad_j-=1;
                Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_PINK);
                Graphics_fillCircle(&g_sContext,77,65,8);
                cj++;
                Estados=Principal;
                sprintf(cadena,"\n\rEl precio de su pedido es de %d euros.\n\r",precio);
                UARTprint(cadena);
            }
            if(pos==80)
            {
                precio+=1;
                cantidad_pi-=1;
                Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_GREEN);
                Graphics_fillCircle(&g_sContext,51,91,8);
                cpi++;
                Estados=Principal;
                sprintf(cadena,"\n\rEl precio de su pedido es de %d euros.\n\r",precio);
                UARTprint(cadena);
            }
            if(pos==112)
            {
                precio+=3;
                cantidad_pe-=1;
                Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_DARK_RED);
                Graphics_fillCircle(&g_sContext,77,91,8);
                cpe++;
                Estados=Principal;
                sprintf(cadena,"\n\rEl precio de su pedido es de %d euros.\n\r",precio);
                UARTprint(cadena);
            }


            break;

        }

        //----------------------Maquina:Cocina---------------------------

        switch(Cocina)
        {
        case Recibe_Pedido:
            //Esperamos que se reciba un pedido
            if(pedido==1)
            {
                pedido=0;
                hueco--;            //Horno ocupado
                tseg=0;             //Reiniciamos contador de segundos
                puestos++;          //Se actualiza el numero de puestos de trabajo
                Cocina=Horno;
            }
            break;
        case Horno:

            tiempo_espera_pizza=pedidos[n]-tseg;
            while(pedidos[n]!=0)    //Mientras haya una pizza registrada, el horno irá haciendo pizzas
            {
                //una vez transcurrido el tiempo de la pizza guardado en el vector
                if(tseg==pedidos[n])
                {
                    hueco++;        //se libera el hueco de la pizza
                    tseg=0;         //reiniciamos contador de segundos en caso de que detras venga otra pizza seguida
                    n++;            //Nos movemos al siguiente tiempo de pizza
                    Cocina=Listo;
                }
                break;
            }


            break;

        case Listo:
            sprintf(cadena,"\n\rLa pizza %d esta lista\n\r",n-1);
            UARTprint(cadena);
            Cocina=Recibe_Pedido;
            break;
        }
    }
}
//---------------------------Puerto Serie---------------------------
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR_HOOK(void)
{
    caracter=UCA0RXBUF; // Leo dato recibido
}
//---------------------------TIMERS---------------------------------
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Interrupcion_T0(void)
{
    tms++;
    seg++;

    if(tms>5)
    {
        tms=0;
        LPM0_EXIT;
    }

    if(seg>20)
    {
        tseg++;
        tseg2++;

        if(tiempo_pantalla>0)
        {
            tiempo_pantalla--;
        }
        else tiempo_pantalla=0;                                               //Mostramos en pantalla el tiempo de espera
        sprintf(cant,"%d",tiempo_pantalla);                                   //para el último pedido solicitado.
        Graphics_setFont(&g_sContext, &g_sFontCm12b);
        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_LIGHT_GREEN); // Color de fondo
        Graphics_fillRectangle(&g_sContext, &(Graphics_Rectangle){1, 115, 20, 127});
        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
        Graphics_drawString(&g_sContext,cant,3,2,115,OPAQUE_TEXT);

        seg=0;

    }
}

void Set_Clock(char VEL)
{
    BCSCTL2 = SELM_0 | DIVM_0 | DIVS_0;
    switch(VEL){
    case 1:
        if (CALBC1_1MHZ != 0xFF) {
            DCOCTL = 0x00;
            BCSCTL1 = CALBC1_1MHZ;      /* Set DCO to 1MHz */
            DCOCTL = CALDCO_1MHZ;
        }
        break;
    case 8:

        if (CALBC1_8MHZ != 0xFF) {
            __delay_cycles(100000);
            DCOCTL = 0x00;
            BCSCTL1 = CALBC1_8MHZ;      /* Set DCO to 8MHz */
            DCOCTL = CALDCO_8MHZ;
        }
        break;
    case 12:
        if (CALBC1_12MHZ != 0xFF) {
            __delay_cycles(100000);
            DCOCTL = 0x00;
            BCSCTL1 = CALBC1_12MHZ;     /* Set DCO to 12MHz */
            DCOCTL = CALDCO_12MHZ;
        }
        break;
    case 16:
        if (CALBC1_16MHZ != 0xFF) {
            __delay_cycles(100000);
            DCOCTL = 0x00;
            BCSCTL1 = CALBC1_16MHZ;     /* Set DCO to 16MHz */
            DCOCTL = CALDCO_16MHZ;
        }
        break;
    default:
        if (CALBC1_1MHZ != 0xFF) {
            DCOCTL = 0x00;
            BCSCTL1 = CALBC1_1MHZ;      /* Set DCO to 1MHz */
            DCOCTL = CALDCO_1MHZ;
        }
        break;
    }
    BCSCTL1 |= XT2OFF | DIVA_0;
    BCSCTL3 = XT2S_0 | LFXT1S_2 | XCAP_1;
}

//-------------------------Joystick--------------------//
#pragma vector=ADC10_VECTOR
__interrupt void ConvertidorAD(void)
{
    LPM0_EXIT; //Despierta al micro al final de la conversión
}

int lee_ch(char canal){
    ADC10CTL0 &= ~ENC;                  //deshabilita el ADC
    ADC10CTL1&=(0x0fff);                //Borra canal anterior
    ADC10CTL1|=canal<<12;               //selecciona nuevo canal
    ADC10CTL0|= ENC;                    //Habilita el ADC
    ADC10CTL0|=ADC10SC;                 //Empieza la conversión
    LPM0;                               //Espera fin en modo LPM0
    return(ADC10MEM);                   //Devuelve valor leido
}

void inicia_ADC(char canales){
    ADC10CTL0 &= ~ENC;                                 //deshabilita ADC
    ADC10CTL0 = ADC10ON | ADC10SHT_3 | SREF_0|ADC10IE; //enciende ADC, S/H lento, REF:VCC, con INT
    ADC10CTL1 = CONSEQ_0 | ADC10SSEL_0 | ADC10DIV_0 | SHS_0 | INCH_0;
    //Modo simple, reloj ADC, sin subdivision, Disparo soft, Canal 0
    ADC10AE0 = canales;                                //habilita los canales indicados
    ADC10CTL0 |= ENC;                                  //Habilita el ADC
}

//-------------------Edición Flash--------------------------//

void guarda_flash(char *dato, unsigned int direc){
    unsigned int i;
    char *  Puntero = (char *) direc;       // Apunta a la direccion
    if(direc>=0x1000 && direc<0x10C0){      // Comprueba area permitida
        FCTL1 = FWKEY + ERASE;              // activa  Erase
        FCTL3 = FWKEY;                      // Borra Lock (pone a 0)
        *Puntero = 0;                       // Escribe algo para borrar el segmento
        FCTL1 = FWKEY + WRT;                // Activa WRT y borra ERASE
        for(i=0;i<10;i++){
            Puntero[i]= dato[i];            // Escribe el dato
        }
        FCTL1 = FWKEY;                      // Borra bit WRT
        FCTL3 = FWKEY + LOCK;               // activa LOCK
    }
}

//---------------Funciones para dibujar--------------------//
void dibuja_pizza_vacia(){

    //----------Ingredientes-----------//
    //Queso
    Graphics_Rectangle rect1 = {4,4,28,28};
    Graphics_Rectangle int_rect1 = {5,5,27,27};

    //Jamon York
    Graphics_Rectangle rect2 = {36,4,60,28};
    Graphics_Rectangle int_rect2 = {37,5,59,27};

    //Pimientos
    Graphics_Rectangle rect3 = {68,4,92,28};
    Graphics_Rectangle int_rect3 = {69,5,91,27};

    //Pepperoni
    Graphics_Rectangle rect4 = {100,4,124,28};
    Graphics_Rectangle int_rect4 = {101,5,123,27};
    Crystalfontz128x128_Init();
    Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_UP);

    //FONDO VERDE
    Graphics_initContext(&g_sContext, &g_sCrystalfontz128x128);
    Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_LIGHT_GREEN);
    Graphics_clearDisplay(&g_sContext);

    //Ingrediente: Queso
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
    Graphics_drawRectangle(&g_sContext,&rect1);
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_YELLOW);
    Graphics_fillRectangle(&g_sContext,&int_rect1);

    //Ingrediente: Jamon York
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
    Graphics_drawRectangle(&g_sContext,&rect2);
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_PINK);
    Graphics_fillRectangle(&g_sContext,&int_rect2);

    //Ingrediente: Pimientos
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
    Graphics_drawRectangle(&g_sContext,&rect3);
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_DARK_GREEN);
    Graphics_fillRectangle(&g_sContext,&int_rect3);

    //Ingrediente: Peperonni
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
    Graphics_drawRectangle(&g_sContext,&rect4);
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_DARK_RED);
    Graphics_fillRectangle(&g_sContext,&int_rect4);

    //Masa pizza con tomate
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_YELLOW);
    Graphics_fillCircle(&g_sContext,64,78,38);
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_RED);
    Graphics_fillCircle(&g_sContext,64,78,34);

    //------------Cantidad ingredientes-----------//

    //Queso
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_YELLOW);
    Graphics_fillCircle(&g_sContext,51,65,8);


    //Jamon york
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_PINK);
    Graphics_fillCircle(&g_sContext,77,65,8);

    //Pimientos
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_GREEN);
    Graphics_fillCircle(&g_sContext,51,91,8);

    //Peperonni
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_DARK_RED);
    Graphics_fillCircle(&g_sContext,77,91,8);



    //Selector de ingredientes
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    Graphics_fillCircle(&g_sContext,xmin,32,3);
}

void dibuja_cantidades(){
    //Cantidad Queso
    Graphics_setFont(&g_sContext, &g_sFontCm16b);
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
    sprintf(cant,"%d",cq);
    Graphics_drawString(&g_sContext,cant,5,48,59,TRANSPARENT_TEXT);
    //Cantidad Jamon York
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
    sprintf(cant,"%d",cj);
    Graphics_drawString(&g_sContext,cant,5,74,59,TRANSPARENT_TEXT);
    //Cantidad Pimientos
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
    sprintf(cant,"%d",cpi);
    Graphics_drawString(&g_sContext,cant,5,48,85,TRANSPARENT_TEXT);
    //Cantidad Peperonni
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
    sprintf(cant,"%d",cpe);
    Graphics_drawString(&g_sContext,cant,5,74,85,TRANSPARENT_TEXT);
}
