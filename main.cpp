#include "mbed.h"
#include "DebouncedInterrupt.h"
#include "Encoder.h"

#define ENCODER_CALIBRATION_VALUE 20
 
Serial pc(SERIAL_TX, SERIAL_RX);
//Serial debug_serial(PA_9,PA_10);

//DebouncedInterrupt mybutton(USER_BUTTON); //for debug
DebouncedInterrupt mybutton(D7); //Jump platform port
void up_button();
 
//General Variables
DigitalOut  myled(LED1);
DigitalIn   modeconfig(D8, PullUp);
DigitalIn   prueba(D9, PullUp);
Timer       timer1;
Ticker      encoder_send;

//Jump platform related variables
volatile int            uptime = 0;
volatile int            downtime = 0;
volatile bool           press = false; 
int                     debounce_time = 50;

//Encoder related variables
TIM_Encoder_InitTypeDef encoder3;
TIM_HandleTypeDef timer3;
uint32_t enlapsedtime, read_time;
volatile signed char    steps, value_to_send;


float delay = 1.0; // 1 sec
 
void pressed()
{
    downtime = timer1.read_us();
    //timer1.reset();
    press = true;    
    mybutton.attach(NULL,IRQ_FALL,debounce_time);     
    mybutton.attach(&up_button,IRQ_RISE,debounce_time);
}
 
 
void up_button()
{
    //New event
    uptime = timer1.read_us();
    //timer1.reset();
    press = false;
    mybutton.attach(NULL,IRQ_RISE,debounce_time);
    mybutton.attach(&pressed,IRQ_FALL,debounce_time);
} 

void send_encoder_data()
{
     pc.putc(value_to_send);
     value_to_send = 0;    
}
 
int main()
{
    int deltatime, count;
    unsigned char my_char, aux;
    signed char steps;
    bool lastbuttonstate = false;
    uint32_t lastcount = 0xFFFF, aprox_position = 0xFFFF, virtual_position = 0xFFFF;
    
    //Initial Config
    if(modeconfig) {
        myled = 1;
        pc.baud(115200);    // Configuration for Encoder Mode
        encoder_send.attach_us(&send_encoder_data,1000);//
    } else {
        myled = 0;
        pc.baud(9600);      //Configuration for Jump Mode
    }
    pc.format(8,SerialBase::None,1);
    
    //debug_serial.baud(921600);
    //debug_serial.format(8,SerialBase::None,1);

    //EncoderInit(encoder3, timer3, TIM3, 0xffff, TIM_ENCODERMODE_TI12); //For better resolution
    EncoderInit(encoder3, timer3, TIM3, 0xffff, TIM_ENCODERMODE_TI1);
    mybutton.attach(&pressed,IRQ_FALL,debounce_time);
 
    timer1.start();
    while (1) {
        if(pc.readable()) { //receive data
            my_char = pc.getc();
            if(my_char == 'J') {
                pc.putc('J');
            } else if(my_char == 'E') { //Get Status
                //pc.putc('E');
                if(press) {
                    pc.printf("E%c",1);
                } else {
                    pc.printf("E%c",0);
                }
            } else if(my_char == 'V') { //Get version
                pc.putc('1');
                pc.putc('.');
                pc.putc('1');  
            } else if(my_char == 'a') { //Get debounce time
                pc.putc(debounce_time/10+'0'); 
            } else if(my_char == 'b') { //Set debounce time
                if(pc.readable()) {
                    aux = pc.getc();
                    if((aux > 0) && (aux < 200)) {
                        debounce_time = aux * 10;
                    }
                }
            }    
        }
        //Platform State
        if(lastbuttonstate != press) {
            if(press) {
                deltatime = (downtime - uptime)/8; //Quit resolution for ChronoJump Compatibility
                pc.printf("X%c%c%c%c",1,(deltatime&0xFF0000)>>16,(deltatime&0x00FF00)>>8,deltatime&0x0000FF);
            } else {
                deltatime = (uptime - downtime)/8; //Quit resolution for ChronoJump Compatibility
                pc.printf("X%c%c%c%c",0,(deltatime&0xFF0000)>>16,(deltatime&0x00FF00)>>8,deltatime&0x0000FF);
            }
            lastbuttonstate = press;    
        }
        
        // Encoder State
        int16_t count3 = 0;        
        count3 = TIM3->CNT;
        virtual_position = (0xFFFF - count3);   //Reverse motion - Alargar negativo - Acortar positivo
        count = virtual_position - lastcount;
        steps = count / ENCODER_CALIBRATION_VALUE;  
        aprox_position += (int32_t)(steps * ENCODER_CALIBRATION_VALUE);
        //debug_serial.printf("%d#%d#%d\r\n",count3,virtual_position,aprox_position);
        __disable_irq();
        value_to_send += steps;
        __enable_irq();
        lastcount += (steps * ENCODER_CALIBRATION_VALUE);
    }
}

