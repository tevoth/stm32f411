#include "gpio.h"
#include "button.h"
#include "system_init.h"

int main(void) {
  system_init();
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
  return 0;
}
