#include "gpio.h"
#include "button.h"

int main(void) {
  led_init();
  button_init();
  while(1) {
    bool btn_state = button_get_state_debounced();
    if (btn_state) {
      led_on();
    } else {
      led_off();
    }
  }
  return 1;
}
