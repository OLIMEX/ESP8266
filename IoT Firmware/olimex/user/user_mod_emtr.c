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
#include "user_plug.h"
#include "user_switch1.h"
#include "user_switch2.h"
#include "modules/mod_emtr.h"

LOCAL struct {
	emtr_calibration_registers *calibration;
	emtr_frequency_registers   *frequency;
	emtr_event_registers       *event;
	emtr_output_registers      *output;
} emtr_registers = {NULL, NULL, NULL, NULL};

LOCAL uint8 emtr_filter_output    = 1;
LOCAL bool  emtr_single_wire_read = false;

LOCAL emtr_low_pass_output *last_event    = NULL;
LOCAL emtr_low_pass_output *current_event = NULL;

LOCAL emtr_mode emtr_current_mode     = EMTR_LOG;
LOCAL uint32    emtr_read_interval    = EMTR_DEFAULT_READ_INTERVAL;

LOCAL void emtr_start_read();

LOCAL const char ICACHE_FLASH_ATTR *emtr_mode_str(uint8 mode) {
	switch (mode) {
		case EMTR_LOG          : return "Log";
		case EMTR_CONFIGURE    : return "Configure";
		case EMTR_CALIBRATION  : return "Calibration";
	}
}

LOCAL void ICACHE_FLASH_ATTR emtr_read_format(char *response, uint32 interval) {
	if (last_event == NULL) {
		return;
	}
	
	char data_str[WEBSERVER_MAX_RESPONSE_LEN];
	json_data(
		response, MOD_EMTR, OK_STR,
		json_sprintf(
			data_str,
			"\"Address\" : \"0x%04X\", "
			"\"CounterActive\" : %d, "
			"\"CounterApparent\" : %d, "
			"\"ReadInterval\" : %d, "
			"\"Interval\" : %d, "
			"\"Filter\" : %d, "
			
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
			emtr_read_interval,
			interval,
			emtr_filter_output,
			
			(_sint64_)last_event->current_rms,
			(_sint64_)last_event->voltage_rms,
			(_sint64_)last_event->active_power,
			(_sint64_)last_event->reactive_power,
			(_sint64_)last_event->apparent_power,
			(_sint64_)last_event->power_factor,
			(_sint64_)last_event->line_frequency,
			(_sint64_)last_event->thermistor_voltage,
			(_sint64_)last_event->event_flag,
			(_sint64_)last_event->system_status
		),
		NULL
	);
}

LOCAL void ICACHE_FLASH_ATTR emtr_calibration_format(char *response) {
	if (emtr_registers.calibration == NULL || emtr_registers.frequency == NULL) {
		return;
	}
	
	char data_str[WEBSERVER_MAX_RESPONSE_LEN];
	json_data(
		response, MOD_EMTR, OK_STR,
		json_sprintf(
			data_str,
			"\"GainCurrentRMS\" : %d, "
			"\"GainVoltageRMS\" : %d, "
			"\"GainActivePower\" : %d, "
			"\"GainReactivePower\" : %d, "
			"\"GainLineFrequency\" : %d, "
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
			"\"LineFrequencyRef\" : %d, "
			"\"AccumulationInterval\" : %d",
			
			emtr_registers.calibration->gain_current_rms,
			emtr_registers.calibration->gain_voltage_rms,
			emtr_registers.calibration->gain_active_power,
			emtr_registers.calibration->gain_reactive_power,
			emtr_registers.frequency->gain_line_frequency,
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
			emtr_registers.frequency->line_frequency_ref,
			emtr_registers.calibration->accumulation_interval
		),
		NULL
	);
}

LOCAL void ICACHE_FLASH_ATTR emtr_event_format(char *response) {
	if (emtr_registers.event == NULL) {
		return;
	}
	
	char data_str[WEBSERVER_MAX_RESPONSE_LEN];
	json_data(
		response, MOD_EMTR, OK_STR,
		json_sprintf(
			data_str,
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
}

LOCAL void ICACHE_FLASH_ATTR emtr_frequency_done(emtr_packet *packet) {
	if (device_get_uart() != UART_EMTR) {
#if EMTR_DEBUG
		debug("EMTR: %s\n", DEVICE_NOT_FOUND);
#endif
		return;
	}
	
	if (emtr_registers.frequency == NULL) {
		emtr_registers.frequency = (emtr_frequency_registers *)os_zalloc(sizeof(emtr_frequency_registers));
	}
	
	emtr_parse_frequency(packet, emtr_registers.frequency);
	char response[WEBSERVER_MAX_RESPONSE_LEN];
	emtr_calibration_format(response);
	user_event_raise(EMTR_URL, response);
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
	setTimeout(emtr_get_frequency, emtr_frequency_done, 100);
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
	
	char response[WEBSERVER_MAX_RESPONSE_LEN];
	emtr_event_format(response);
	user_event_raise(EMTR_URL, response);
}

LOCAL void ICACHE_FLASH_ATTR emtr_timeout() {
	char response[WEBSERVER_MAX_VALUE];
	
	debug("EMTR: %s\n", TIMEOUT);
	
	json_error(response, MOD_EMTR, TIMEOUT, NULL);
	user_event_raise(EMTR_URL, response);
	
	emtr_clear_timeout();
	emtr_start_read();
}

LOCAL bool ICACHE_FLASH_ATTR emtr_over_treshold(float old, float new, float treshold) {
	return (
		new - old > treshold ||
		old - new > treshold
	);
}

LOCAL float ICACHE_FLASH_ATTR emtr_low_pass(float old, float new, float factor, float treshold) {
	if (emtr_over_treshold(old, new, treshold)) {
		return new;
	}
	return (old + (new - old) / factor);
}

LOCAL void ICACHE_FLASH_ATTR emtr_read_event(uint32 interval) {
	bool event = false;
	
	if (last_event == NULL) {
		current_event = (emtr_low_pass_output *)os_zalloc(sizeof(emtr_low_pass_output));
		last_event    = (emtr_low_pass_output *)os_zalloc(sizeof(emtr_low_pass_output));
		event = true;
	} else {
		event = (emtr_filter_output == 0);
	}
	
	if (event) {
		current_event->current_rms    = emtr_registers.output->current_rms;
		current_event->voltage_rms    = emtr_registers.output->voltage_rms;
		current_event->active_power   = emtr_registers.output->active_power;
		current_event->reactive_power = emtr_registers.output->reactive_power;
		current_event->apparent_power = emtr_registers.output->apparent_power;
		current_event->power_factor   = emtr_registers.output->power_factor;
		current_event->line_frequency = emtr_registers.output->line_frequency;
		current_event->event_flag     = emtr_registers.output->event_flag;
		current_event->system_status  = emtr_registers.output->system_status;
	} else {
		current_event->current_rms    = emtr_low_pass(current_event->current_rms,    emtr_registers.output->current_rms,     4,  10);
		current_event->voltage_rms    = emtr_low_pass(current_event->voltage_rms,    emtr_registers.output->voltage_rms,     4,   5);
		current_event->active_power   = emtr_low_pass(current_event->active_power,   emtr_registers.output->active_power,    2,   2);
		current_event->reactive_power = emtr_low_pass(current_event->reactive_power, emtr_registers.output->reactive_power,  2,   2);
		current_event->apparent_power = emtr_low_pass(current_event->apparent_power, emtr_registers.output->apparent_power,  2,   2);
		current_event->power_factor   = emtr_low_pass(current_event->power_factor,   emtr_registers.output->power_factor,    8, 150);
		current_event->line_frequency = emtr_low_pass(current_event->line_frequency, emtr_registers.output->line_frequency,  8, 100);
		current_event->event_flag     = emtr_registers.output->event_flag;
		current_event->system_status  = emtr_registers.output->system_status;
		
		event = event || emtr_over_treshold(last_event->current_rms,    current_event->current_rms,     10);
		event = event || emtr_over_treshold(last_event->voltage_rms,    current_event->voltage_rms,      5);
		event = event || emtr_over_treshold(last_event->active_power,   current_event->active_power,     2);
		event = event || emtr_over_treshold(last_event->reactive_power, current_event->reactive_power,   2);
		event = event || emtr_over_treshold(last_event->apparent_power, current_event->apparent_power,   2);
		event = event || emtr_over_treshold(last_event->power_factor,   current_event->power_factor,   150);
		event = event || emtr_over_treshold(last_event->line_frequency, current_event->line_frequency, 100);
		event = event || (last_event->event_flag    != current_event->event_flag);
		event = event || (last_event->system_status != current_event->system_status);
	}
	
	if (!event) {
		return;
	}
	
	*last_event = *current_event;
	
	char response[WEBSERVER_MAX_RESPONSE_LEN];
	emtr_read_format(response, interval);
	user_event_raise(EMTR_URL, response);
}

LOCAL void ICACHE_FLASH_ATTR emtr_read_done(emtr_packet *packet) {
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
	
	if (emtr_registers.output == NULL) {
		emtr_registers.output = (emtr_output_registers *)os_zalloc(sizeof(emtr_output_registers));
	}
	
	if (emtr_single_wire_read) {
		emtr_parse_single_wire(packet, emtr_registers.output);
	} else {
		emtr_parse_output(packet, emtr_registers.output);
	}
	
	emtr_counter_add(
		emtr_registers.output->active_power   * interval / 1000, 
		emtr_registers.output->apparent_power * interval / 1000
	);
	emtr_read_event(interval);
	emtr_start_read();
	if (emtr_registers.output->event_flag != 0) {
		emtr_clear_event(emtr_registers.output->event_flag, NULL);
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
// FIXME initial system_configuration
LOCAL uint32  system_configuration = 0x03000000;
	
	if (device_get_uart() != UART_EMTR) {
#if EMTR_DEBUG
		debug("EMTR: %s\n", DEVICE_NOT_FOUND);
#endif
		return;
	}
	
	clearTimeout(emtr_read_timer);
	if (emtr_single_wire_read) {
		emtr_single_wire_start(system_configuration, emtr_read_done);
		system_configuration = system_configuration | 0x00000100;
	} else {
		emtr_read_timer = setTimeout(emtr_get_output, emtr_read_done, emtr_read_interval);
	}
}

LOCAL void ICACHE_FLASH_ATTR emtr_set_calibration_done(emtr_packet *packet) {
	emtr_set_frequency(emtr_registers.frequency, NULL);
}

LOCAL void ICACHE_FLASH_ATTR emtr_reset(emtr_packet *packet) {
#if DEVICE == PLUG
	plug_down();
	setTimeout(plug_init, NULL, EMTR_RESET_TIMEOUT);
#endif
#if DEVICE == SWITCH1
	switch1_down();
	setTimeout(switch1_init, NULL, EMTR_RESET_TIMEOUT);
#endif
#if DEVICE == SWITCH2
	switch2_down();
	setTimeout(switch2_init, NULL, EMTR_RESET_TIMEOUT);
#endif
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
	
	if (emtr_registers.output == NULL) {
		emtr_registers.output = (emtr_output_registers *)os_zalloc(sizeof(emtr_output_registers));
	}
	
	struct jsonparse_state parser;
	int type;
	
	bool set_counter = false;
	bool set_configure = false;
	bool set_calibration = false;
	bool calc_calibration = false;
	
	emtr_mode mode = emtr_current_mode;
	emtr_mode response_format = EMTR_LOG;
	
	_uint64_ counter_active = emtr_counter_active();
	_uint64_ counter_apparent = emtr_counter_apparent();
	
	if (method == POST && data != NULL && data_len != 0) {
		jsonparse_setup(&parser, data, data_len);
		
		while ((type = jsonparse_next(&parser)) != 0) {
			if (type == JSON_TYPE_PAIR_NAME) {
				if (jsonparse_strcmp_value(&parser, "Return") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					if (jsonparse_strcmp_value(&parser, "Log") == 0) {
						response_format = EMTR_LOG;
					} else if (jsonparse_strcmp_value(&parser, "Configure") == 0) {
						response_format = EMTR_CONFIGURE;
					} else if (jsonparse_strcmp_value(&parser, "Calibration") == 0) {
						response_format = EMTR_CALIBRATION;
					}
				} else if (jsonparse_strcmp_value(&parser, "Mode") == 0) {
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
					if (emtr_read_interval < EMTR_MIN_READ_INTERVAL) {
						emtr_read_interval = EMTR_MIN_READ_INTERVAL;
					}
					if (emtr_read_interval > EMTR_MAX_READ_INTERVAL) {
						emtr_read_interval = EMTR_MAX_READ_INTERVAL;
					}
				} else if (jsonparse_strcmp_value(&parser, "Filter") == 0) {
					jsonparse_next(&parser);
					jsonparse_next(&parser);
					emtr_filter_output = jsonparse_get_value_as_int(&parser);
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
						set_configure = true;
					} else if (jsonparse_strcmp_value(&parser, "OverPowerLimit") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						emtr_registers.event->over_power_limit = jsonparse_get_value_as_int(&parser);
						set_configure = true;
					} else if (jsonparse_strcmp_value(&parser, "OverFrequencyLimit") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						emtr_registers.event->over_frequency_limit = jsonparse_get_value_as_int(&parser);
						set_configure = true;
					} else if (jsonparse_strcmp_value(&parser, "UnderFrequencyLimit") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						emtr_registers.event->under_frequency_limit = jsonparse_get_value_as_int(&parser);
						set_configure = true;
					} else if (jsonparse_strcmp_value(&parser, "OverTemperatureLimit") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						emtr_registers.event->over_temperature_limit = jsonparse_get_value_as_int(&parser);
						set_configure = true;
					} else if (jsonparse_strcmp_value(&parser, "UnderTemperatureLimit") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						emtr_registers.event->under_temperature_limit = jsonparse_get_value_as_int(&parser);
						set_configure = true;
					} else if (jsonparse_strcmp_value(&parser, "VoltageSagLimit") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						emtr_registers.event->voltage_sag_limit = jsonparse_get_value_as_int(&parser);
						set_configure = true;
					} else if (jsonparse_strcmp_value(&parser, "VoltageSurgeLimit") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						emtr_registers.event->voltage_surge_limit = jsonparse_get_value_as_int(&parser);
						set_configure = true;
					} else if (jsonparse_strcmp_value(&parser, "OverCurrentHold") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						emtr_registers.event->over_current_hold = jsonparse_get_value_as_int(&parser);
						set_configure = true;
					} else if (jsonparse_strcmp_value(&parser, "OverPowerHold") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						emtr_registers.event->over_power_hold = jsonparse_get_value_as_int(&parser);
						set_configure = true;
					} else if (jsonparse_strcmp_value(&parser, "OverFrequencyHold") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						emtr_registers.event->over_frequency_hold = jsonparse_get_value_as_int(&parser);
						set_configure = true;
					} else if (jsonparse_strcmp_value(&parser, "UnderFrequencyHold") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						emtr_registers.event->under_frequency_hold = jsonparse_get_value_as_int(&parser);
						set_configure = true;
					} else if (jsonparse_strcmp_value(&parser, "OverTemperatureHold") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						emtr_registers.event->over_temperature_hold = jsonparse_get_value_as_int(&parser);
						set_configure = true;
					} else if (jsonparse_strcmp_value(&parser, "UnderTemperatureHold") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						emtr_registers.event->under_temperature_hold = jsonparse_get_value_as_int(&parser);
						set_configure = true;
					} else if (jsonparse_strcmp_value(&parser, "EventEnable") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						emtr_registers.event->event_enable = jsonparse_get_value_as_int(&parser);
						set_configure = true;
					} else if (jsonparse_strcmp_value(&parser, "EventMaskCritical") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						emtr_registers.event->event_mask_critical = jsonparse_get_value_as_int(&parser);
						set_configure = true;
					} else if (jsonparse_strcmp_value(&parser, "EventMaskStandard") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						emtr_registers.event->event_mask_standard = jsonparse_get_value_as_int(&parser);
						set_configure = true;
					} else if (jsonparse_strcmp_value(&parser, "EventTest") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						emtr_registers.event->event_test = jsonparse_get_value_as_int(&parser);
						set_configure = true;
					} else if (jsonparse_strcmp_value(&parser, "EventClear") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						emtr_registers.event->event_clear = jsonparse_get_value_as_int(&parser);
						set_configure = true;
					}
				} else if (mode == EMTR_CALIBRATION) {
					if (jsonparse_strcmp_value(&parser, "CalcCalibration") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						calc_calibration = jsonparse_get_value_as_int(&parser);
					} else if (jsonparse_strcmp_value(&parser, "GainCurrentRMS") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						uint32 value = jsonparse_get_value_as_int(&parser);
						if (value) {
							emtr_registers.calibration->gain_current_rms = value;
							set_calibration = true;
						}
					} else if (jsonparse_strcmp_value(&parser, "GainVoltageRMS") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						uint32 value = jsonparse_get_value_as_int(&parser);
						if (value) {
							emtr_registers.calibration->gain_voltage_rms = value;
							set_calibration = true;
						}
					} else if (jsonparse_strcmp_value(&parser, "GainActivePower") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						uint32 value = jsonparse_get_value_as_int(&parser);
						if (value) {
							emtr_registers.calibration->gain_active_power = value;
							set_calibration = true;
						}
					} else if (jsonparse_strcmp_value(&parser, "GainReactivePower") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						int value = jsonparse_get_value_as_sint(&parser);
						if (value) {
							if (value < 0) {
								emtr_relay_set(EMTR_RELAYS_COUNT - 1, 1);
							}
							emtr_registers.calibration->gain_reactive_power = abs(value);
							set_calibration = true;
						}
					} else if (jsonparse_strcmp_value(&parser, "OffsetCurrentRMS") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						sint32 value = jsonparse_get_value_as_sint(&parser);
						emtr_registers.calibration->offset_current_rms = value;
						set_calibration = true;
					} else if (jsonparse_strcmp_value(&parser, "OffsetActivePower") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						sint32 value = jsonparse_get_value_as_sint(&parser);
						emtr_registers.calibration->offset_active_power = value;
						set_calibration = true;
					} else if (jsonparse_strcmp_value(&parser, "OffsetReactivePower") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						sint32 value = jsonparse_get_value_as_sint(&parser);
						emtr_registers.calibration->offset_reactive_power = value;
						set_calibration = true;
					} else if (jsonparse_strcmp_value(&parser, "DCOffsetCurrent") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						sint16 value = jsonparse_get_value_as_sint(&parser);
						emtr_registers.calibration->dc_offset_current = value;
						set_calibration = true;
					} else if (jsonparse_strcmp_value(&parser, "PhaseCompensation") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						sint16 value = jsonparse_get_value_as_sint(&parser);
						emtr_registers.calibration->phase_compensation = value;
						set_calibration = true;
					} else if (jsonparse_strcmp_value(&parser, "ApparentPowerDivisor") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						uint16 value = jsonparse_get_value_as_int(&parser);
						emtr_registers.calibration->apparent_power_divisor = value;
						set_calibration = true;
					} else if (jsonparse_strcmp_value(&parser, "SystemConfiguration") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						uint32 value = jsonparse_get_value_as_int(&parser);
						emtr_registers.calibration->system_configuration = value;
						set_calibration = true;
					} else if (jsonparse_strcmp_value(&parser, "DIOConfiguration") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						uint32 value = jsonparse_get_value_as_int(&parser);
						if (value) {
							emtr_registers.calibration->dio_configuration = value;
							set_calibration = true;
						}
					} else if (jsonparse_strcmp_value(&parser, "Range") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						uint32 value = jsonparse_get_value_as_int(&parser);
						if (value) {
							emtr_registers.calibration->range = value;
							set_calibration = true;
						}
					} else if (jsonparse_strcmp_value(&parser, "AccumulationInterval") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						uint32 value = jsonparse_get_value_as_int(&parser);
						if (value) {
							emtr_registers.calibration->accumulation_interval = value;
							set_calibration = true;
						}
					} else if (jsonparse_strcmp_value(&parser, "CalibrationVoltage") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						uint16 value = jsonparse_get_value_as_int(&parser);
						if (value) {
							emtr_registers.calibration->calibration_voltage = value;
							if (calc_calibration) {
								emtr_calibration_calc(
									emtr_registers.output,			
									emtr_registers.calibration,
									0
								);
							}
							set_calibration = true;
						}
					} else if (jsonparse_strcmp_value(&parser, "CalibrationCurrent") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						uint32 value = jsonparse_get_value_as_int(&parser);
						if (value) {
							emtr_registers.calibration->calibration_current = value;
							if (calc_calibration) {
								emtr_calibration_calc(
									emtr_registers.output,
									emtr_registers.calibration,
									8
								);
							}
							set_calibration = true;
						}
					} else if (jsonparse_strcmp_value(&parser, "CalibrationActivePower") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						uint32 value = jsonparse_get_value_as_int(&parser);
						if (value) {
							emtr_registers.calibration->calibration_active_power = value;
							if (calc_calibration) {
								emtr_calibration_calc(
									emtr_registers.output,
									emtr_registers.calibration,
									16
								);
							}
							set_calibration = true;
						}
					} else if (jsonparse_strcmp_value(&parser, "CalibrationReactivePower") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						uint32 value = jsonparse_get_value_as_int(&parser);
						if (value) {
							emtr_registers.calibration->calibration_reactive_power = value;
							if (calc_calibration) {
								emtr_calibration_reactive_power(
									emtr_registers.output,
									emtr_registers.calibration
								);
							}
							set_calibration = true;
						}
					} else if (jsonparse_strcmp_value(&parser, "LineFrequencyRef") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						uint16 value = jsonparse_get_value_as_int(&parser);
						if (value) {
							emtr_registers.frequency->line_frequency_ref = value;
							if (calc_calibration) {
								emtr_calibration_line_freqensy(
									emtr_registers.output,
									emtr_registers.frequency
								);
							}
							set_calibration = true;
						}
					} else if (jsonparse_strcmp_value(&parser, "GainLineFrequency") == 0) {
						jsonparse_next(&parser);
						jsonparse_next(&parser);
						uint16 value = jsonparse_get_value_as_int(&parser);
						if (value) {
							emtr_registers.frequency->gain_line_frequency = value;
							set_calibration = true;
						}
					}
				}
			}
		}
		
		if (set_configure) {
			emtr_set_event(emtr_registers.event, NULL);
		}
		
		if (set_counter) {
			emtr_set_counter(counter_active, counter_apparent, emtr_reset);
		}
		
		if (set_calibration) {
			emtr_set_calibration(emtr_registers.calibration, emtr_set_calibration_done);
		}
	}
	
	if (mode != emtr_current_mode) {
		response_format = emtr_current_mode;
	}
	
	if (response_format == EMTR_CONFIGURE) {
		emtr_event_format(response);
	} else if (response_format == EMTR_CALIBRATION) {
		emtr_calibration_format(response);
	} else {
		emtr_read_format(response, emtr_read_interval);
	}
	
	if (emtr_current_mode == EMTR_CONFIGURE) {
		emtr_events_read();
	} else if (emtr_current_mode == EMTR_CALIBRATION) {
		emtr_calibration_read();
	} else {
		emtr_start_read();
	}
}

void ICACHE_FLASH_ATTR mod_emtr_init() {
	emtr_set_timeout_callback(emtr_timeout);
	webserver_register_handler_callback(EMTR_URL, emtr_handler);
	device_register(UART, 0, MOD_EMTR, EMTR_URL, emtr_init, emtr_down);
	
	setTimeout(emtr_calibration_read, NULL, 1500);
	setTimeout(emtr_events_read, NULL, 2000);
	setTimeout(emtr_start_read, NULL, 2500);
}
#endif
