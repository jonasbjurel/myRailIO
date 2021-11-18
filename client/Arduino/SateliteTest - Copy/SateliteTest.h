#ifndef GENJMRI_H
#define GENJMRI_H

//#include <MqttClient.h> After recovery
#include <ArduinoLog.h>
//#include <C:\Users\jonas\AppData\Local\Arduino15\packages\esp32\hardware\esp32\1.0.6\libraries\WiFi\src\WiFi.h>
//#include "WiFi.h"
//#include "esp_wps.h"
#include "esp_timer.h"
//#include <PubSubClient.h>
//#include <QList.h>
//#include <tinyxml2.h>
//#include <esp_task_wdt.h>
#include <Adafruit_NeoPixel.h>
//#include <cmath>
//#include "ESPTelnet.h"
//#include <SimpleCLI.h>
#include <limits>
//#include <cstddef>
#include "driver/rmt.h"
#include "driver/periph_ctrl.h"
#include "soc/rmt_reg.h"

static void IRAM_ATTR ws28xx_rmt_tx_adapter(const void *src, rmt_item32_t *dest, size_t src_size,
        size_t wanted_num, size_t *translated_size, size_t *item_num);

#endif /*GENJMRI_H*/
