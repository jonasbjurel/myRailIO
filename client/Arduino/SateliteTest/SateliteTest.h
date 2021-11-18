#ifndef GENJMRI_H
#define GENJMRI_H

#include "Satelite.h"

#define LINK_ADDRESS    0
#define LINK_TX_PIN     25
#define LINK_RX_PIN     14
#define RMT_TX_CHAN     2
#define RMT_RX_CHAN     1
#define RMT_TX_MEMBANK  2
#define RMT_RX_MEMBANK  1
#define SCAN_PRIO       20
#define SCAN_CORE       1
#define SCAN_INTERVAL   5


//void newSensorVal(satelite* satHandle_p, uint8_t linkAddr_p, uint8_t satAddr_p, uint8_t senseAddr_p, bool state_p);
//void newSatState(satelite* satHandle_p, uint8_t linkAddr_p, uint8_t satAddr_p, uint8_t senseAddr_p, satOpState_t opState_p);



#endif /*GENJMRI_H*/
