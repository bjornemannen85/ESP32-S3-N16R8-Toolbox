#include <stdio.h>
#include "sdkconfig.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "lwip/lwip_napt.h"

void app_main(void) {
    /* Link-time test only. No AP, no credentials, no NAT traffic. */
    void (*enable_napt)(u32_t, int) = ip_napt_enable;
    printf("NAT symbol link probe: %p; configured=%d\n", (void*)enable_napt, CONFIG_LWIP_IPV4_NAPT);
}
