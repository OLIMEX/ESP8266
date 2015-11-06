
/****************************************************************************
 * Action
 ****************************************************************************/
function Action(name, callback) {
	this._name = name;
	
	if (typeof callback != 'function') {
		console.log('ACTIONS: [' + this._name + '] callback must be a function');
		this._callback = null;
	} else {
		this._callback = callback;
	}
	
	this._parameters = {};
}

Action.prototype.parameter = function (parameter, type, required) {
	this._parameters[parameter] = {
		type: type,
		required: required
	};
	
	return this;
};

Action.prototype.execute = function (parameters) {
	for (var index in this._parameters) {
		var current = this._parameters[index];
		var type = typeof parameters[index];
		if (current.required &&  type == 'undefined') {
			console.log('ACTIONS: [' + this._name + '] missing [' + index + '] parameter');
			return;
		}
		
		if (current.type != type && type != 'undefined') {
			console.log(
				'ACTIONS: [' + this._name + '] wrong [' + index + '] type! '+
				'Expected ['+current.type+'] got ['+type+']'
			);
			return;
		}
	}
	
	if (typeof this._callback != 'function') {
		console.log('ACTIONS: [' + this._name + '] wrong callback type');
	} else {
		this._callback(parameters);
	}
};

/****************************************************************************
 * Actions
 ****************************************************************************/
module.exports = {
	_poll: [],
	
	register: function (name, callback) {
		if (typeof this._poll[name] != 'undefined') {
			console.log('ACTIONS: Action ['+name+'] is already registered');
		} else {
			this._poll[name] = new Action(name, callback);
		}
		
		return this._poll[name];
	},
	
	get: function (name) {
		if (typeof this._poll[name] == 'undefined') {
			console.log('ACTIONS: Unknown action ['+name+']');
			return null;
		}
		return this._poll[name];
	},
	
	execute: function (name, parameters) {
		var action = this.get(name);
		if (action === null) {
			return;
		}
		
		action.execute(parameters);
	}
}
