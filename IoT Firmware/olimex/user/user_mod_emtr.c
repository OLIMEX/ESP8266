#include "user_config.h"
#if MOD_EMTR_ENABLE

#include "ets_sys.h"
#include "stdout.h"
#include "osapi.h"
#include "mem.h"

#include "json/jsonparse.h"

#include "user_misc.h"
#include "user_json.h"
#include "user_events.h"
#include "user_webserver.h"
#include "user_config.h"
#include "user_devices.h"

#include "user_mod_emtr.h"
#include "modules/mod_emtr.h"

LOCAL struct {
	emtr_calibration_registers *calibration;
	emtr_event_registers       *event;
} emtr_registers = {NULL, NULL};

LOCAL emtr_mode emtr_current_mode  = EMTR_LOG;
LOCAL uint32    emtr_read_interval = 1000;

LOCAL const char ICACHE_FLASH_ATTR *emtr_mode_str(uint8 mode) {
	switch (mode) {
		case EMTR_LOG          : return "Log";
		case EMTR_CONFIGURE    : return "Configure";
		case EMTR_CALIBRATION  : return "Calibration";
	}
}

LOCAL void ICACHE_FLASH_ATTR emtr_calibration_done(emtr_packet *packet) {
	if (device_get_uart() != UART_EMTR) {
#if EMTR_DEBUG
		debug("EMTR: %s\n", DEVICE_NOT_FOUND);
#endif
		return;
	}
	
	if (emtr_registers.calibration == NULL) {
		emtr_registers.calibration = (emtr_calibration_registers *)os_zalloc(sizeof(emtr_calibration_registers));
	}
	
	emtr_parse_calibration(packet, emtr_registers.calibration);
}

LOCAL void ICACHE_FLASH_ATTR emtr_event_done(emtr_packet *packet) {
	if (device_get_uart() != UART_EMTR) {
#if EMTR_DEBUG
		debug("EMTR: %s\n", DEVICE_NOT_FOUND);
#endif
		return;
	}
	
	if (emtr_registers.event == NULL) {
		emtr_registers.event = (emtr_event_registers *)os_zalloc(sizeof(emtr_event_registers));
	}
	
	emtr_parse_event(packet, emtr_registers.event);
}

LOCAL void emtr_start_read();

LOCAL void ICACHE_FLASH_ATTR emtr_timeout() {
	char response[WEBSERVER_MAX_VALUE];
	json_error(response, MOD_EMTR, TIMEOUT, NULL);
	user_event_raise(EMTR_URL, response);
	
	emtr_clear_timeout();
	emtr_start_read();
}

LOCAL void ICACHE_FLASH_ATTR emtr_read_done(emtr_packet *packet) {
	LOCAL emtr_output_registers *registers = NULL;
	LOCAL uint32 time = 0;
	LOCAL uint32 interval = 0;
	LOCAL uint32 now = 0;
	
	now = system_get_time();
	interval = (time != 0) ? 
		(now - time) / 1000
		: 
		0
	;
	time = now;
	
	if (registers == NULL) {
		registers = (emtr_output_registers *)os_zalloc(sizeof(emtr_output_registers));
	}
	
	emtr_parse_output(packet, registers);
	
	char event_str[20];
	os_memset(event_str, 0, sizeof(event_str));
	
	char response[WEBSERVER_MAX_RESPONSE_LEN];
	char data_str[WEBSERVER_MAX_RESPONSE_LEN];
	json_data(
		response, MOD_EMTR, OK_STR,
		json_sprintf(
			data_str,
			"\"Address\" : \"0x%04X\", "
			"\"CounterActive\" : %d, "
			"\"CounterApparent\" : %d, "
			"\"Interval\" : %d, "
			
			"\"CurrentRMS\" : %d, "
			"\"VoltageRMS\" : %d, "
			"\"ActivePower\" : %d, "
			"\"ReactivePower\" : %d, "
			"\"ApparentPower\" : %d, "
			"\"PowerFactor\" : %d, "
			"\"LineFrequency\" : %d, "
			"\"ThermistorVoltage\" : %d, "
			"\"EventFlag\" : %d, "
			"\"SystemStatus\" : \"0x%04X\"",
			emtr_address(),
			emtr_counter_active(),
			emtr_counter_apparent(),
			interval,
			
			registers->current_rms,
			registers->voltage_rms,
			registers->active_power,
			registers->reactive_power,
			registers->apparent_power,
			registers->power_factor,
			registers->line_frequency,
			registers->thermistor_voltage,
			registers->event_flag,
			registers->system_status
		),
		NULL
	);
	
	emtr_counter_add(
		registers->active_power * interval / 1000, 
		registers->apparent_power * interval / 1000
	);
	
	user_event_raise(EMTR_URL, response);
	emtr_start_read();
	
	if (registers->event_flag != 0) {
		emtr_clear_event(registers->event_flag, NULL);
	}
}

LOCAL void ICACHE_FLASH_ATTR emtr_calibration_read() {
	if (device_get_uart() != UART_EMTR) {
#if EMTR_DEBUG
		debug("EMTR: %s\n", DEVICE_NOT_FOUND);
#endif
		return;
	}
	setTimeout(emtr_get_calibration, emtr_calibration_done, 100);
}

LOCAL void ICACHE_FLASH_ATTR emtr_events_read() {
	if (device_get_uart() != UART_EMTR) {
#if EMTR_DEBUG
		debug("EMTR: %s\n", DEVICE_NOT_FOUND);
#endif
		return;
	}
	setTimeout(emtr_get_event, emtr_event_done, 100);
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
	emtr_read_timer = setTimeout(emtr_get_output, emtr_read_done, emtr_read_interval);
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
	
	if (emtr_registers.calibration == NULL) {
		emtr_registers.calibration = (emtr_calibration_registers *)os_zalloc(sizeof(emtr_calibration_registers));
	}
	
	if (emtr_registers.event == NULL) {
		emtr_registers.event = (emtr_event_registers *)os_zalloc(sizeof(emtr_event_registers));
	}
		
	struct jsonparse_state parser;
	int type;
	bool set_counter = false;
	emtr_mode mode = emtr_current_mode;
	_uint64_ counter_active = emtr_counter_active();
	_uint64_ counter_apparent = emtr_counter_apparent();
	
	if (method == POST && data != NULL && data_len != 0) {
		jsonparse_setup(&parser, data, data_len);
		
		while ((type = jsonparse_next(&parser)) != 0) {
			if (type == JSON_TYPE_PAIR_NAME) {
				if (jsonparse_strcmp_value(&parser, "Mode") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					if (jsonparse_strcmp_value(&parser, "Log") == 0) {
						emtr_current_mode = EMTR_LOG;
					} else if (jsonparse_strcmp_value(&parser, "Configure") == 0) {
						emtr_current_mode = EMTR_CONFIGURE;
					} else if (jsonparse_strcmp_value(&parser, "Calibration") == 0) {
						emtr_current_mode = EMTR_CALIBRATION;
					}
				} else if (jsonparse_strcmp_value(&parser, "ReadInterval") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					emtr_read_interval = jsonparse_get_value_as_int(&parser);
				} else if (jsonparse_strcmp_value(&parser, "CounterActive") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					counter_active = jsonparse_get_value_as_int(&parser);
					set_counter = true;
				} else if (jsonparse_strcmp_value(&parser, "CounterApparent") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					counter_apparent = jsonparse_get_value_as_int(&parser);
					set_counter = true;
				}
				
				if (mode == EMTR_CONFIGURE) {
					if (jsonparse_strcmp_value(&parser, "OverCurrentLimit") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						emtr_registers.event->over_current_limit = jsonparse_get_value_as_int(&parser);
					} else if (jsonparse_strcmp_value(&parser, "OverPowerLimit") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						emtr_registers.event->over_power_limit = jsonparse_get_value_as_int(&parser);
					} else if (jsonparse_strcmp_value(&parser, "OverFrequencyLimit") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						emtr_registers.event->over_frequency_limit = jsonparse_get_value_as_int(&parser);
					} else if (jsonparse_strcmp_value(&parser, "UnderFrequencyLimit") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						emtr_registers.event->under_frequency_limit = jsonparse_get_value_as_int(&parser);
					} else if (jsonparse_strcmp_value(&parser, "OverTemperatureLimit") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						emtr_registers.event->over_temperature_limit = jsonparse_get_value_as_int(&parser);
					} else if (jsonparse_strcmp_value(&parser, "UnderTemperatureLimit") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						emtr_registers.event->under_temperature_limit = jsonparse_get_value_as_int(&parser);
					} else if (jsonparse_strcmp_value(&parser, "VoltageSagLimit") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						emtr_registers.event->voltage_sag_limit = jsonparse_get_value_as_int(&parser);
					} else if (jsonparse_strcmp_value(&parser, "VoltageSurgeLimit") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						emtr_registers.event->voltage_surge_limit = jsonparse_get_value_as_int(&parser);
					} else if (jsonparse_strcmp_value(&parser, "OverCurrentHold") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						emtr_registers.event->over_current_hold = jsonparse_get_value_as_int(&parser);
					} else if (jsonparse_strcmp_value(&parser, "OverPowerHold") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						emtr_registers.event->over_power_hold = jsonparse_get_value_as_int(&parser);
					} else if (jsonparse_strcmp_value(&parser, "OverFrequencyHold") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						emtr_registers.event->over_frequency_hold = jsonparse_get_value_as_int(&parser);
					} else if (jsonparse_strcmp_value(&parser, "UnderFrequencyHold") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						emtr_registers.event->under_frequency_hold = jsonparse_get_value_as_int(&parser);
					} else if (jsonparse_strcmp_value(&parser, "OverTemperatureHold") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						emtr_registers.event->over_temperature_hold = jsonparse_get_value_as_int(&parser);
					} else if (jsonparse_strcmp_value(&parser, "UnderTemperatureHold") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						emtr_registers.event->under_temperature_hold = jsonparse_get_value_as_int(&parser);
					} else if (jsonparse_strcmp_value(&parser, "EventEnable") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						emtr_registers.event->event_enable = jsonparse_get_value_as_int(&parser);
					} else if (jsonparse_strcmp_value(&parser, "EventMaskCritical") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						emtr_registers.event->event_mask_critical = jsonparse_get_value_as_int(&parser);
					} else if (jsonparse_strcmp_value(&parser, "EventMaskStandard") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						emtr_registers.event->event_mask_standard = jsonparse_get_value_as_int(&parser);
					} else if (jsonparse_strcmp_value(&parser, "EventTest") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						emtr_registers.event->event_test = jsonparse_get_value_as_int(&parser);
					} else if (jsonparse_strcmp_value(&parser, "EventClear") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						emtr_registers.event->event_clear = jsonparse_get_value_as_int(&parser);
					}
				}
			}
		}
		
		emtr_set_event(emtr_registers.event, NULL);
		if (set_counter) {
			emtr_set_counter(counter_active, counter_apparent, NULL);
		}
	}
	
	char data_str[WEBSERVER_MAX_RESPONSE_LEN];
	if (emtr_current_mode == EMTR_CALIBRATION) {
		json_data(
			response, MOD_EMTR, OK_STR,
			json_sprintf(
				data_str,
				"\"Address\" : \"0x%04X\", "
				"\"Mode\" : \"%s\", "
				"\"CounterActive\" : %d, "
				"\"CounterApparent\" : %d, "
				"\"ReadInterval\" : %d, "
				
				"\"GainCurrentRMS\" : %d, "
				"\"GainVoltageRMS\" : %d, "
				"\"GainActivePower\" : %d, "
				"\"GainReactivePower\" : %d, "
				"\"OffsetCurrentRMS\" : %d, "
				"\"OffsetActivePower\" : %d, "
				"\"OffsetReactivePower\" : %d, "
				"\"DCOffsetCurrent\" : %d, "
				"\"PhaseCompensation\" : %d, "
				"\"ApparentPowerDivisor\" : %d, "
				"\"SystemConfiguration\" : \"0x%08X\", "
				"\"DIOConfiguration\" : \"0x%04X\", "
				"\"Range\" : \"0x%08X\", "
				
				"\"CalibrationCurrent\" : %d, "
				"\"CalibrationVoltage\" : %d, "
				"\"CalibrationActivePower\" : %d, "
				"\"CalibrationReactivePower\" : %d, "
				"\"AccumulationInterval\" : %d",
				emtr_address(),
				emtr_mode_str(emtr_current_mode),
				emtr_counter_active(),
				emtr_counter_apparent(),
				emtr_read_interval,
				
				emtr_registers.calibration->gain_current_rms,
				emtr_registers.calibration->gain_voltage_rms,
				emtr_registers.calibration->gain_active_power,
				emtr_registers.calibration->gain_reactive_power,
				emtr_registers.calibration->offset_current_rms,
				emtr_registers.calibration->offset_active_power,
				emtr_registers.calibration->offset_reactive_power,
				emtr_registers.calibration->dc_offset_current,
				emtr_registers.calibration->phase_compensation,
				emtr_registers.calibration->apparent_power_divisor,
				emtr_registers.calibration->system_configuration,
				emtr_registers.calibration->dio_configuration,
				emtr_registers.calibration->range,
				
				emtr_registers.calibration->calibration_current,
				emtr_registers.calibration->calibration_voltage,
				emtr_registers.calibration->calibration_active_power,
				emtr_registers.calibration->calibration_reactive_power,
				emtr_registers.calibration->accumulation_interval
			),
			NULL
		);
		setTimeout(emtr_calibration_read, NULL, 1500);
	} else if (emtr_current_mode == EMTR_CONFIGURE) {
		json_data(
			response, MOD_EMTR, OK_STR,
			json_sprintf(
				data_str,
				"\"Address\" : \"0x%04X\", "
				"\"Mode\" : \"%s\", "
				"\"CounterActive\" : %d, "
				"\"CounterApparent\" : %d, "
				"\"ReadInterval\" : %d, "
				
				"\"OverCurrentLimit\" : %d, "
				"\"OverPowerLimit\" : %d, "
				"\"OverFrequencyLimit\" : %d, "
				"\"UnderFrequencyLimit\" : %d, "
				"\"OverTemperatureLimit\" : %d, "
				"\"UnderTemperatureLimit\" : %d, "
				"\"VoltageSagLimit\" : %d, "
				"\"VoltageSurgeLimit\" : %d, "
				"\"OverCurrentHold\" : %d, "
				"\"OverPowerHold\" : %d, "
				"\"OverFrequencyHold\" : %d, "
				"\"UnderFrequencyHold\" : %d, "
				"\"OverTemperatureHold\" : %d, "
				"\"UnderTemperatureHold\" : %d, "
				"\"EventEnable\" : %d, "
				"\"EventMaskCritical\" : %d, "
				"\"EventMaskStandard\" : %d, "
				"\"EventTest\" : %d, "
				"\"EventClear\" : %d",
				emtr_address(),
				emtr_mode_str(emtr_current_mode),
				emtr_counter_active(),
				emtr_counter_apparent(),
				emtr_read_interval,
				
				emtr_registers.event->over_current_limit,
				emtr_registers.event->over_power_limit,
				emtr_registers.event->over_frequency_limit,
				emtr_registers.event->under_frequency_limit,
				emtr_registers.event->over_temperature_limit,
				emtr_registers.event->under_temperature_limit,
				emtr_registers.event->voltage_sag_limit,
				emtr_registers.event->voltage_surge_limit,
				emtr_registers.event->over_current_hold,
				emtr_registers.event->over_power_hold,
				emtr_registers.event->over_frequency_hold,
				emtr_registers.event->under_frequency_hold,
				emtr_registers.event->over_temperature_hold,
				emtr_registers.event->under_temperature_hold,
				emtr_registers.event->event_enable,
				emtr_registers.event->event_mask_critical,
				emtr_registers.event->event_mask_standard,
				emtr_registers.event->event_test,
				emtr_registers.event->event_clear
			),
			NULL
		);
		setTimeout(emtr_events_read, NULL, 1500);
	} else {
		json_data(
			response, MOD_EMTR, OK_STR,
			json_sprintf(
				data_str,
				"\"Address\" : \"0x%04X\", "
				"\"Mode\" : \"%s\", "
				"\"CounterActive\" : %d, "
				"\"CounterApparent\" : %d, "
				"\"ReadInterval\" : %d",
				emtr_address(),
				emtr_mode_str(emtr_current_mode),
				emtr_counter_active(),
				emtr_counter_apparent(),
				emtr_read_interval
			),
			NULL
		);
	}
	
	emtr_start_read();
}

void ICACHE_FLASH_ATTR mod_emtr_init() {
	emtr_set_timeout_callback(emtr_timeout);
	webserver_register_handler_callback(EMTR_URL, emtr_handler);
	device_register(UART, 0, EMTR_URL, emtr_init, emtr_down);
	
	setTimeout(emtr_calibration_read, NULL, 1500);
	setTimeout(emtr_events_read, NULL, 2000);
	setTimeout(emtr_start_read, NULL, 2500);
}
#endif
