var JSONPath   = require('advanced-json-path');
var Properties = require('./properties');
var Actions    = require('./actions');

/****************************************************************************
 * Triggers
 ****************************************************************************/
module.exports = {
	_poll: [],
	
	register: function (path, action, paramBuilder) {
		var actionName = '';
		if (typeof action == 'string') {
			actionName = action;
			action = Actions.get(actionName);
		} else {
			action = null;
		}
		
		if (action === null) {
			console.log('TRIGGERS: Action not found ['+actionName+']');
		}
		
		// console.log('TRIGGERS: Register '+path+' ['+actionName+']');
		var trigger = {
			path: path,
			actionName: actionName,
			action: action,
			paramBuilder: paramBuilder
		};
		
		this._poll.push(trigger);
		return trigger;
	},
	
	remove: function (trigger) {
		var i  = this._poll.indexOf(trigger);
		if (i >= 0) {
			this._poll.splice(i, 1);
		}
	},
	
	onRegisterDevice: function (node, deviceURL, action, paramBuilder) {
		var nodePath = node ?
			'[?($.EventNode.Name == "'+node+'" || $.EventNode.Token == "'+node+'")]'
			:
			''
		;
		
		var devicePath = deviceURL ? 
			'.EventData.Data.Devices[?(@.URL == "'+deviceURL+'" && @.Found == 1)]'
			:
			''
		;
		
		return this.register('$[?($.EventURL == "/devices")]' + nodePath + devicePath, action, paramBuilder);
	},
	
	onPropertyChange: function (node, device, property, cmp_opearor, value, action, paramBuilder) {
		var nodePath = node ?
			'[?($.EventNode.Name == "'+node+'" || $.EventNode.Token == "'+node+'")]'
			:
			''
		;
		
		var devicePath = device ? 
			'[?('+Properties.get(null, 'Device')+' == "'+device+'")]'
			:
			''
		;
		
		cmp_opearor = typeof cmp_opearor == 'string' ?
			cmp_opearor
			:
			' === '
		;
		
		var propertyPath = Properties.get(device, property);
		propertyPath = propertyPath ? 
			'[?({'+propertyPath+'}'+(value ? cmp_opearor+JSON.stringify(value) : ' !== false')+')]'
			:
			''
		;
		
		return this.register('$' + nodePath + devicePath + propertyPath, action, paramBuilder);
	},
	
	onConnectionClose: function (node, action, paramBuilder) {
		var nodePath = node ?
			'[?($.EventNode.Name == "'+node+'" || $.EventNode.Token == "'+node+'")]'
			:
			''
		;
		
		return this.register('$' + nodePath + '.EventData[?(@.Status == "Connection close")]', action, paramBuilder);
	},
	
	fire: function (data) {
		console.log('DATA: ' + JSON.stringify(data));
		this._poll.forEach(
			function (current, index, array) {
				if (JSONPath(data, current.path) && current.action != null) {
					console.log('TRIGGERS: fire(' + current.action._name + ')');
					current.action.execute(
						current.paramBuilder ?
							current.paramBuilder.execute(data)
							:
							{}
					);
				}
			}
		);
		console.log();
	}
};
