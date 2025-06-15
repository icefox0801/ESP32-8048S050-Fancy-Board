#include "dashboard_config.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
  printf("Dashboard main started!\n");
  // Initialize hardware, display, etc. here
  while (1)
  {
    printf("Dashboard running...\n");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
