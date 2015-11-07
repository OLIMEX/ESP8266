var JSONPath = require('advanced-json-path');

/****************************************************************************
 * Properties
 ****************************************************************************/
module.exports = Properties = {
	_poll: [],
	
	register: function (device, property, path) {
		if (typeof device != 'string') {
			device = '*';
		}
		
		if (typeof this._poll[device] == 'undefined') {
			this._poll[device] = [];
		}
		
		this._poll[device][property] = path;
		
		return this;
	},
	
	get: function (device, property) {
		if (typeof device != 'string') {
			device = '*';
		}
		
		var path = JSONPath(this._poll, '$["'+device+'"]["'+property+'"]');
		if (path) {
			return path;
		}
		
		return JSONPath(this._poll, '$["*"]["'+property+'"]');
	},
	
	value: function (property, data) {
		var device = JSONPath(
			data, 
			Properties.get(null, 'Device')
		);
		
		var path = Properties.get(device, property);
		if (path === false) {
			return null;
		}
		
		return JSONPath(data, path);
	}
};

/****************************************************************************
 * Register some properties
 ****************************************************************************/
Properties.
	register(null,         'Device',         '$.EventData.Device').
	register(null,         'Status',         '$.EventData.Status').
	register(null,         'Error',          '$.EventData.Error').
	register(null,         'I2C_Address',    '$.EventData.I2C_Address').
	register(null,         'NodeIP',         '$.EventNode.IP').
	register(null,         'NodeName',       '$.EventNode.Name').
	register(null,         'NodeToken',      '$.EventNode.Token').
	
	register('ESP8266',    'Relay',          '$.EventData.Data.Relay').
	register('ESP8266',    'Button',         '$.EventData.Data.Button').
	register('ESP8266',    'ADCValue',       '$.EventData.Data.ADC.Value').
	register('ESP8266',    'BatteryState',   '$.EventData.Data.Battery.State').
	register('ESP8266',    'BatteryPercent', '$.EventData.Data.Battery.Percent').
	
	register('MOD-TC-MK2', 'Temperature',    '$.EventData.Data.Temperature').
	
	register('MOD-FINGER', 'FingerID',       '$.EventData.Match.ID').
	register('MOD-FINGER', 'FingerScore',    '$.EventData.Match.Score')
;
