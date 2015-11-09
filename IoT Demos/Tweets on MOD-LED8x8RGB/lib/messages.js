var Connections   = require('./connections');
var Triggers      = require('./triggers');
var Actions       = require('./actions');
var ParamBuilder  = require('./param-builder');
var MessagesQueue = require('./messages-queue');

/****************************************************************************
 * Messages
 ****************************************************************************/
module.exports = Messages = {
	_queues: [],
	
	init: function (node, device, url, rows, cols, speed) {
		this.remove(node, url);
		
		var queue = new MessagesQueue(node, url, rows, cols, speed);
		
		queue.
			addTrigger(
				Triggers.onPropertyChange(
					node, device, 'Error', null, 'Busy', 
					'Messages.busy', 
					ParamBuilder().
						parameter('node', '[NodeToken]')
				)
			).
			
			addTrigger(
				Triggers.onPropertyChange(
					node, device, 'Status', null, 'OK', 
					'Messages.setTimeout', 
					ParamBuilder().
						parameter('node', '[NodeToken]')
				)
			).
			
			addTrigger(
				Triggers.onPropertyChange(
					node, device, 'Status', null, 'Done', 
					'Messages.done', 
					ParamBuilder().
						parameter('node', '[NodeToken]')
				)
			).
			
			addTrigger(
				Triggers.onConnectionClose(
					node,
					'Messages.remove', 
					ParamBuilder().
						parameter('node', '[NodeToken]')
				)
			)
		;
		
		this._queues.push(queue);
		return queue;
	},
	
	remove: function (node, url) {
		if (typeof url == 'undefined') {
			url = null;
		}
		
		var connection = Connections.get(node);
		var token = connection === null ?
			node
			:
			connection.node.Token
		;
		
		console.log('Messages.remove("'+token+'", "'+url+'").totalBefore('+this._queues.length+')');
		
		var removed = 0;
		var index = 0;
		while (this._queues.length > index) {
			var current = this._queues[index];
			if (
				current.token() === token && 
				(url === null || current.url() === url)
			) {
				current.removeTriggers();
				this._queues.splice(index, 1);
				removed++;
			} else {
				index++;
			}
		}
		
		console.log('Messages.removed('+removed+').remainingAfter('+this._queues.length+')');
		return removed;
	},
	
	findQueue: function (params) {
		var connection = Connections.get(params.node);
		var token = connection === null ?
			params.node
			:
			connection.node.Token
		;
		
		var result = [];
		for (var index in this._queues) {
			var current = this._queues[index];
			if (current.token() === token || token === '*' || token === null) {
				result.push(this._queues[index]);
			}
		}
		
		return result;
	},
	
	display: function (params) {
		console.log('Messages.display('+JSON.stringify(params)+')');
		var queues = this.findQueue(params);
		queues.forEach(
			function (queue) {
				queue.display(params);
			}
		);
	},
	
	setTimeout: function (params) {
		console.log('Messages.setTimeout('+JSON.stringify(params)+')');
		var queues = this.findQueue(params);
		queues.forEach(
			function (queue) {
				queue.setTimeout();
			}
		);
	},
	
	done: function (params) {
		console.log('Messages.done('+JSON.stringify(params)+')');
		var queues = this.findQueue(params);
		queues.forEach(
			function (queue) {
				queue.done();
			}
		);
	},
	
	busy: function (params) {
		console.log('Messages.busy('+JSON.stringify(params)+')');
		var queues = this.findQueue(params);
		queues.forEach(
			function (queue) {
				queue.busy();
			}
		);
	}
	
};

/****************************************************************************
 * Initialization
 ****************************************************************************/
Actions.
	register(
		'Messages.display', 
		function (params) {
			Messages.display(params);
		}
	).
	parameter('node',  'string', true).
	parameter('text',  'string', true).
	parameter('r',     'number', false).
	parameter('g',     'number', false).
	parameter('b',     'number', false).
	parameter('rows',  'number', false).
	parameter('cols',  'number', false).
	parameter('speed', 'number', false)
;

Actions.
	register(
		'Messages.setTimeout', 
		function (params) {
			Messages.setTimeout(params);
		}
	).
	parameter('node', 'string', true)
;

Actions.
	register(
		'Messages.done', 
		function (params) {
			Messages.done(params);
		}
	).
	parameter('node', 'string', true)
;

Actions.
	register(
		'Messages.busy', 
		function (params) {
			Messages.busy(params);
		}
	).
	parameter('node', 'string', true)
;

Actions.
	register(
		'Messages.remove', 
		function (params) {
			Messages.remove(params.node);
		}
	).
	parameter('node', 'string', true)
;
