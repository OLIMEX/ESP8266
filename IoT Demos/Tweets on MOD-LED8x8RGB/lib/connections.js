// Connections
module.exports = {
	_poll: [],
	
	add: function (connection) {
		connection.node = {
			Name: "",
			Token: "",
			Devices: []
		};
		
		this._poll.push(connection);
	},
	
	setName: function (connection, name) {
		connection.node.Name = name;
	},
	
	setToken: function (connection, token) {
		var old = this.get(token);
		if (old !== null) {
			this.remove(old);
		}
		
		connection.node.Token = token;
	},
	
	remove: function (connection) {
		if (connection.connected) {
			connection.close();
			return;
		}
		
		var i  = this._poll.indexOf(connection);
		if (i >= 0) {
			this._poll.splice(i, 1);
		}
	},
	
	get: function (node) {
		if (typeof node == 'object') {
			if (node.connected) {
				return node;
			}
			node = node.node.Token;
		}
		
		var connection = null;
		for (var index in this._poll) {
			var current = this._poll[index];
			if (
				current.node.Name  == node ||
				current.node.Token == node
			) {
				connection = current;
				break;
			}
		}
		
		return connection;
	},
	
	getNodes: function () {
		var data = [];
		this._poll.forEach(
			function (current, index, array) {
				data.push(
					{
						IP:      current.remoteAddress,
						Name:    current.node.Name,
						Token:   current.node.Token,
						Devices: current.node.Devices
					}
				);
			}
		);
		return data;
	}
};
