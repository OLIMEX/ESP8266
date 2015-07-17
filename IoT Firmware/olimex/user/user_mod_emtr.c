#include "ets_sys.h"
#include "stdout.h"
#include "osapi.h"
#include "mem.h"

#include "json/jsonparse.h"

#include "user_json.h"
#include "user_events.h"
#include "user_webserver.h"
#include "user_config.h"
#include "user_devices.h"

#include "user_mod_emtr.h"
#include "modules/mod_emtr.h"

LOCAL void emtr_start_read();

LOCAL void ICACHE_FLASH_ATTR emtr_timeout(emtr_packet *packet) {
	char response[WEBSERVER_MAX_VALUE];
	json_error(response, MOD_EMTR, TIMEOUT, NULL);
	user_event_raise(EMTR_URL, response);
	
	emtr_start_read();
}

LOCAL void ICACHE_FLASH_ATTR emtr_read_done(emtr_packet *packet) {
	LOCAL emtr_registers *registers = NULL;
	
	if (registers == NULL) {
		registers = (emtr_registers *)os_zalloc(sizeof(emtr_registers));
	}
	
	emtr_all_parse(packet, registers);
	
	char event_str[20];
	os_memset(event_str, 0, sizeof(event_str));
	char status_str[20];
	os_memset(status_str, 0, sizeof(status_str));
	
	char response[WEBSERVER_MAX_RESPONSE_LEN];
	char data_str[WEBSERVER_MAX_RESPONSE_LEN];
	json_data(
		response, MOD_EMTR, OK_STR,
		json_sprintf(
			data_str,
			"\"Address\" : \"0x%04X\", "
			"\"CurrentRMS\" : %d, "
			"\"VoltageRMS\" : %d, "
			"\"ActivePower\" : %d, "
			"\"ReactivePower\" : %d, "
			"\"ApparentPower\" : %d, "
			"\"PowerFactor\" : %d, "
			"\"LineFrequency\" : %d, "
			"\"ThermistorVoltage\" : %d, "
			"\"EventFlag\" : \"0b%s\", "
			"\"SystemStatus\" : \"0b%s\"",
			emtr_address(),
			registers->current_rms,
			registers->voltage_rms,
			registers->active_power,
			registers->reactive_power,
			registers->apparent_power,
			registers->power_factor,
			registers->line_frequency,
			registers->thermistor_voltage,
			itob(registers->event_flag, event_str, 16),
			itob(registers->system_status, status_str, 16)
		),
		NULL
	);
	
	user_event_raise(EMTR_URL, response);
	emtr_start_read();
}

LOCAL void ICACHE_FLASH_ATTR emtr_start_read() {
LOCAL uint32  emtr_read_timer = 0;
	if (device_get_uart() != UART_EMTR) {
#if EMTR_DEBUG
		debug("EMTR: %s\n", DEVICE_NOT_FOUND);
#endif
		return;
	}
	
	clearTimeout(emtr_read_timer);
	emtr_read_timer = setTimeout(emtr_get_all, emtr_read_done, 10);
}


void ICACHE_FLASH_ATTR emtr_handler(
	struct espconn *pConnection, 
	request_method method, 
	char *url, 
	char *data, 
	uint16 data_len, 
	uint32 content_len, 
	char *response,
	uint16 response_len
) {
	if (device_get_uart() != UART_EMTR) {
		json_error(response, MOD_EMTR, DEVICE_NOT_FOUND, NULL);
		return;
	}
	
	char data_str[WEBSERVER_MAX_VALUE];
	json_data(
		response, MOD_EMTR, OK_STR,
		json_sprintf(
			data_str,
			"\"Address\" : \"0x%04X\"",
			emtr_address()
		),
		NULL
	);
}

void ICACHE_FLASH_ATTR mod_emtr_init() {
	emtr_set_timeout_callback(emtr_timeout);
	webserver_register_handler_callback(EMTR_URL, emtr_handler);
	device_register(UART, 0, EMTR_URL, emtr_init);
	
	setTimeout(emtr_start_read, NULL, 2000);
}
