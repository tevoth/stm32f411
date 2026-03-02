# shared/adc

Shared ADC1 helper module used by ADXL logging projects.

## Files

- `inc/adc.h`: ADC helper API
- `src/adc.c`: ADC1 setup/start/read helpers

## Notes

- Configures `PA1` as ADC1 channel input.
- Uses timeout-based polling to avoid deadlock on missing EOC.

## Current Consumers

- `cmsis_adxl_sdcard`
- `cmsis_adxl_sdcard_fatfs`
