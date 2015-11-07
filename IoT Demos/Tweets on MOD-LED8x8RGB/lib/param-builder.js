var Properties = require('./properties');

/****************************************************************************
 * ParamBuilder
 ****************************************************************************/
function ParamBuilder() {
	this._parameters = {};
}

ParamBuilder.prototype.parameter = function (name, expression) {
	this._parameters[name] = expression;
	return this;
};

ParamBuilder.prototype.eval = function (expression, context) {
	if (typeof expression != 'string') {
		return expression;
	}
	
	var split = expression.split(/\[([^\[\]]+)\]/);
	for (var i in split) {
		if ((i % 2) == 1) {
			split[i] = Properties.value(split[i], context);
		}
	}
	
	var i = 0;
	while (i < split.length) {
		if (split[i] === '') {
			split.splice(i, 1);
		} else {
			i++;
		}
	}
	
	return split.length == 1 ?
		split[0]
		:
		split.join('')
	;
};

ParamBuilder.prototype.execute = function (context) {
	var data = {};
	for (index in this._parameters) {
		var current = this._parameters[index];
		data[index] = this.eval(current, context);
	}
	
	return data;
};

module.exports = function () {
	return new ParamBuilder();
};