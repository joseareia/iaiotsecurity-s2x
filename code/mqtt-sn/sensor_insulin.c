/*
 * @file sensor_breathing.c
 * @author joseareia
 * @date 15/09/2022
 */

#define _DEFAULT_SOURCE

#include "contiki.h"
#include "lib/random.h"
#include "clock.h"
#include "sys/ctimer.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "mqtt_sn.h"
#include "dev/leds.h"
#include "net/rime/rime.h"
#include "net/ip/uip.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static uint16_t udp_port = 1884;
static uint16_t keep_alive = 5;
static uint16_t broker_address[] = {0xaaaa, 0, 0, 0, 0, 0, 0, 0x1};
static struct   etimer time_poll;
// static uint16_t tick_process = 0;
static char pub_test[20];
static char device_id[17];
static char topic_hw[25];
static char *topics_mqtt[] = {"/insulin", topic_hw};
// static char     *will_topic = "/6lowpan_node/offline";
// static char     *will_message = "O dispositivo esta offline";
// This topics will run so much faster than others

mqtt_sn_con_t mqtt_sn_connection;

void mqtt_sn_callback(char *topic, char *message){
    printf("\nMessage received:");
    printf("\nTopic:%s Message:%s",topic,message);
}

void init_broker(void) {
    char *all_topics[ss(topics_mqtt)+1];
    sprintf(device_id,"%02X%02X%02X%02X%02X%02X%02X%02X",
        linkaddr_node_addr.u8[0],linkaddr_node_addr.u8[1],
        linkaddr_node_addr.u8[2],linkaddr_node_addr.u8[3],
        linkaddr_node_addr.u8[4],linkaddr_node_addr.u8[5],
        linkaddr_node_addr.u8[6],linkaddr_node_addr.u8[7]);
    sprintf(topic_hw, "/insulin_device_%02X%02X",linkaddr_node_addr.u8[6],linkaddr_node_addr.u8[7]);

    mqtt_sn_connection.client_id     = device_id;
    mqtt_sn_connection.udp_port      = udp_port;
    mqtt_sn_connection.ipv6_broker   = broker_address;
    mqtt_sn_connection.keep_alive    = keep_alive;
    //mqtt_sn_connection.will_topic    = will_topic;   // Configure as 0x00 if you don't want to use
    //mqtt_sn_connection.will_message  = will_message; // Configure as 0x00 if you don't want to use
    mqtt_sn_connection.will_topic    = 0x00;
    mqtt_sn_connection.will_message  = 0x00;

    mqtt_sn_init();   // Inicializa alocação de eventos e a principal PROCESS_THREAD do MQTT-SN

    size_t i;
    for(i = 0; i < ss(topics_mqtt); i++) {
        all_topics[i] = topics_mqtt[i];
    }
    all_topics[i] = topic_hw;

    mqtt_sn_create_sck(mqtt_sn_connection,
        all_topics,
        ss(all_topics),
        mqtt_sn_callback);

    mqtt_sn_sub(topic_hw,0);
}

/*---------------------------------------------------------------------------*/
PROCESS(init_system_process, "[Contiki-OS] Initializing OS");
AUTOSTART_PROCESSES(&init_system_process);
/*---------------------------------------------------------------------------*/

PROCESS_THREAD(init_system_process, ev, data) {
    PROCESS_BEGIN();

    debug_os("Initializing the MQTT_SN_DEMO");

    init_broker();

    etimer_set(&time_poll, CLOCK_SECOND);

    srand(CLOCK_SECOND);

    int blood_sugar = 0;

    while(1) {
        PROCESS_WAIT_EVENT();

        blood_sugar = rand() % 300 + 50;

        sprintf(pub_test, "Blood sugar is: %d mg/dL", blood_sugar);

        mqtt_sn_pub(topic_hw, pub_test, true, 0);
        mqtt_sn_pub("/insulin", pub_test, true, 0);

        // debug_os("State MQTT:%s",mqtt_sn_check_status_string());
        if (etimer_expired(&time_poll))
            etimer_reset(&time_poll);
    }
    PROCESS_END();
}
