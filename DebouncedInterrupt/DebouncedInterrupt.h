 
#ifndef DEBOUNCED_INTERRUPT_H
#define DEBOUNCED_INTERRUPT_H

#include <stdint.h>
#include "mbed.h"
#include "FunctionPointer.h"

/**
typedef enum {
    IRQ_NONE,
    IRQ_RISE,
    IRQ_FALL
} gpio_irq_event;
**/

/** A replacement for InterruptIn that debounces the interrupt
 *
 * Example:
 * @code
 *
 * #include "DebouncedInterrupt.h"
 *
 * DebouncedInterrupt up_button(p15);
 * 
 * void onUp()
 * {
 *    // Do Something
 * }
 * 
 * int main()
 * {
 *     up_button.attach(&onUp,IRQ_FALL, 100);
 *     while(1) {
 *         ...
 *     }
 * }
 * @endcode
 */
class DebouncedInterrupt {
private:
    unsigned int _debounce_us;
    InterruptIn *_in;
    DigitalIn *_din;
    gpio_irq_event _trigger;
    
    // Diagnostics
    volatile unsigned int _bounce_count;
    volatile unsigned int _last_bounce_count;
    
    void _onInterrupt(void);
    void _callback(void);
public:
    DebouncedInterrupt(PinName pin);
    ~DebouncedInterrupt();
    
    // Start monitoring the interupt and attach a callback
    void attach(void (*fptr)(void), const gpio_irq_event trigger, const uint32_t& debounce_ms=10);
    
    template<typename T>
    void attach(T* tptr, void (T::*mptr)(void), const gpio_irq_event trigger, const uint32_t& debounce_ms=10) {
        _fAttach.attach(tptr, mptr);
        _last_bounce_count = _bounce_count = 0;
        _debounce_us = 1000*debounce_ms;
        _trigger = trigger;
        
        switch(trigger)
        {
            case IRQ_RISE:
                _in->rise(tptr, &DebouncedInterrupt::_onInterrupt);
                break;
            case IRQ_FALL:
                _in->fall(tptr, &DebouncedInterrupt::_onInterrupt);
                break;
            case IRQ_NONE:
                reset(); // Unexpected. Clear callbacks.
                break;
        }
    }
   
    // Stop monitoring the interrupt
    void reset();
    
    
    /*
    * Get number of bounces 
    * @return: bounce count
    */
    unsigned int get_bounce();
protected:
//    https://github.com/mbedmicro/mbed/blob/master/libraries/mbed/api/FunctionPointer.h
    FunctionPointer _fAttach;
};
#endif