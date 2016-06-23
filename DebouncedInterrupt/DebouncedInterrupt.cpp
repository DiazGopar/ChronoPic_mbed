/**
* DebouncedInterrupt.cpp
**/
#include "DebouncedInterrupt.h"

Timeout timeout;

DebouncedInterrupt::DebouncedInterrupt(PinName pin)
{

    _in  = new InterruptIn(pin);
    _din = new DigitalIn(pin);
    //_in->mode(PullUp);
    //_din->mode(PullUp);

}

DebouncedInterrupt::~DebouncedInterrupt()
{
    delete _in;
    delete _din;
}

void DebouncedInterrupt::attach(void (*fptr)(void), const gpio_irq_event trigger, const unsigned int& debounce_ms)
{
    if(fptr) {
        _fAttach.attach(fptr);
        _last_bounce_count = _bounce_count = 0;
        _debounce_us = 1000*debounce_ms;
        _trigger = trigger;
        
        switch(trigger)
        {
            case IRQ_RISE:
                _in->rise(this, &DebouncedInterrupt::_onInterrupt);
                break;
            case IRQ_FALL:
                _in->fall(this, &DebouncedInterrupt::_onInterrupt);
                break;
            case IRQ_NONE:
                reset(); // Unexpected. Clear callbacks.
                break;
        }
    } else {
        reset();
    }
    
   //Workaround: https://developer.mbed.org/questions/5753/PullUpPullDown-on-nucleos-with-Interrupt/
   _in->mode(PullUp); //Allways re(set) Pullup mode
}

void DebouncedInterrupt::reset()
{
    timeout.detach();
}

unsigned int DebouncedInterrupt::get_bounce()
{
    return _last_bounce_count;
}

void DebouncedInterrupt::_callback()
{
    _last_bounce_count = _bounce_count;
    _bounce_count = 0;
    if(_din->read() == (_trigger==IRQ_RISE)) {
        _fAttach.call();
    }
}

void DebouncedInterrupt::_onInterrupt()
{
    _bounce_count++;
    timeout.attach_us(this, &DebouncedInterrupt::_callback, _debounce_us);
}
