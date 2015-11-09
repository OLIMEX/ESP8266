var Connections = require('./connections');
var Triggers    = require('./triggers');

const MESSAGES_TIMEOUT     = 60000;  // 1 minute
const MESSAGES_DEF_TIMEOUT =  5000;  // 5 seconds

/****************************************************************************
 * Message Queue
 ****************************************************************************/
function MessagesQueue(node, url, rows, cols, speed) {
	this._token = node;
	this._connection = null;
	
	this._queue = [];
	this._timeout = null;
	
	this._url = url;
	
	this._rows = rows;
	this._cols = cols;
	this._speed = speed;
	
	this._default = null;
	this._default_timeout = null;
	
	this._current = null;
	this._triggers = [];
	
	this.connect();
}

MessagesQueue.prototype.addTrigger = function (trigger) {
	this._triggers.push(trigger);
	return this;
}

MessagesQueue.prototype.removeTriggers = function () {
	// remove global registered triggers
	this._triggers.forEach(
		function (trigger) {
			Triggers.remove(trigger);
		}
	);
	
	// clear local references
	this._triggers = [];
	return this;
}

MessagesQueue.prototype.connect = function () {
	if (this._connection === null) {
		var connection = Connections.get(this._token);
		if (connection !== null) {
			this._connection = connection;
			this._token = connection.node.Token;
		}
	}
	
	return this;
};

MessagesQueue.prototype.url = function () {
	return this._url;
}

MessagesQueue.prototype.token = function () {
	return this._token;
};

MessagesQueue.prototype.rows = function (rows) {
	this._rows = rows;
	return this;
};

MessagesQueue.prototype.cols = function (cols) {
	this._cols = cols;
	return this;
};

MessagesQueue.prototype.speed = function (speed) {
	this._speed = speed;
	return this;
};

MessagesQueue.prototype.defaultMsg = function (msg) {
	this._default = msg;
	this.next();
};

MessagesQueue.prototype.display = function (message) {
	this._queue.push(message);
	console.log('MessagesQueue.length@'+this.token()+'['+this._queue.length+']');
	this.next();
	return this;
};

MessagesQueue.prototype.next = function() {
	if (this._timeout !== null) {
		return;
	}
	
	var self = this;
	if (this._queue.length == 0) {
		if (this._default === null) {
			return;
		}
		// display default message after timeout
		this._default_timeout = setTimeout(
			function () {
				console.log('MessagesQueue.displayDefault()');
				self.display(self._default);
			},
			MESSAGES_DEF_TIMEOUT
		);
		return;
	}
	
	this.connect();
	if (typeof this._connection != 'object' || !this._connection.connected) {
		// retry after timeout
		setTimeout(
			function () {
				self.next();
			},
			MESSAGES_TIMEOUT
		);
		return;
	}
	
	this.setTimeout();
	
	this._current = this._queue.shift();
	console.log('MessagesQueue.display@'+this.token()+'['+this._current.text+']');
	console.log('MessagesQueue.length@'+this.token()+'['+this._queue.length+']');
	
	this._connection.sendUTF(
		JSON.stringify(
			{
				URL: this._url,
				Method: 'POST',
				Data: {
					Text: this._current.text,
					R: typeof this._current.r == 'undefined' ?
						1
						:
						this._current.r
					,
					G: typeof this._current.g == 'undefined' ?
						1
						:
						this._current.g
					,
					B: typeof this._current.b == 'undefined' ?
						1
						:
						this._current.b
					,
					Speed: typeof this._current.speed == 'undefined' ? 
						this._speed
						:
						this._current.speed
					,
					cols: typeof this._current.cols == 'undefined' ?
						this._cols
						:
						this._current.cols
					,
					rows: typeof this._current.rows == 'undefined' ? 
						this._rows
						:
						this._current.rows
				}
			}
		)
	);
	
};

MessagesQueue.prototype.setTimeout = function () {
	var self = this;
	
	if (this._default_timeout !== null) {
		// reset default message timout
		clearTimeout(this._default_timeout);
		this._default_timeout = null;
	}
	
	if (this._timeout !== null) {
		clearTimeout(this._timeout);
	}
	
	this._timeout = setTimeout(
		// done after timeout
		function () {
			self.done();
		},
		MESSAGES_TIMEOUT
	);
};

MessagesQueue.prototype.done = function () {
	if (this._timeout !== null) {
		clearTimeout(this._timeout);
	}
	this._timeout = null;
	this.next();
};

MessagesQueue.prototype.busy = function () {
	if (this._current !== null) {
		if (this._current.text != this._default.text) {
			this._queue.unshift(this._current);
			console.log('MessagesQueue.busy("'+this._current.text+'")');
			console.log('MessagesQueue.length@'+this.token()+'['+this._queue.length+']');
		}
		this._current = null;
	}
	this.setTimeout();
};

module.exports = MessagesQueue;