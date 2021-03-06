#ifndef _LEDS_H_
#define _LEDS_H_

void led_manager_setup(const raat_devices_struct& devices, const raat_params_struct& params);
void led_manager_set_completed_bars(uint8_t set);

void led_manager_set_led_pressed(uint8_t led, RGBParam<uint8_t> * pColourParam, int8_t led_bank=-1);

void led_manager_set_led_blink(uint8_t nblinks, uint8_t led, bool invert_blink, bool post_blink_state, RGBParam<uint8_t> * pColourParam, int8_t led_bank=-1);
void led_manager_set_bank_blink(uint8_t nblinks, bool invert_blink, bool post_blink_state, RGBParam<uint8_t> * pColourParam, int8_t led_bank=-1);

void led_manager_service();

#endif