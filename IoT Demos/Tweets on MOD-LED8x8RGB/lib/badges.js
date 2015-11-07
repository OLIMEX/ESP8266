var Actions      = require('./actions');
var Triggers     = require('./triggers');
var ParamBuilder = require('./param-builder');
var Messages     = require('./messages');

try {
	var Twitter = require('./twitter');
} catch (err) {
	console.log(err.message);
}

/****************************************************************************
 * Badges
 ****************************************************************************/
module.exports = Badges = {
	add: function (params) {
		console.log('Badges.add('+JSON.stringify(params)+')');
		var messageQueue = Messages.init(params.node, 'ESP-BADGE', '/badge', 1, 1, 40);
		if (typeof Twitter == 'object') {
			messageQueue.defaultMsg(
				{
					text: 'Tweet with '+Twitter.tracking().join(' or ')+' to see your message here ;-)',
					r: 1, g: 1, b: 0
				}
			);
		}
	}
};

Actions.
	register(
		'Badges.add', 
		function (params) {
			Badges.add(params);
		}
	).
	parameter('node', 'string', true)
;

Triggers.
	onRegisterDevice(
		null, '/badge',
		'Badges.add',
		ParamBuilder().
			parameter('node', '[NodeToken]')
	)
;
