#include "gpio.h"

int main(void) {
  led_init();
  button_init();
  while(1) {

    bool btn_state = get_button_state();
    if (btn_state) {
      led_on();
    } else {
      led_off();
    }
  }
  return 1;
}
