var Messages = require('./messages');

const CLOCK_INTERVAL = 1000 * 60 * 1; // 1 minutes

function dateTimePad(n) {
	if (n < 10) {
		return '0'+n;
	}
	return n;
}

setInterval(
	function () {
		var now = new Date();
		Messages.display(
			{
				node: '*',
				text: 
					// dateTimePad(now.getDate() )+'.'+dateTimePad(now.getMonth())+'.'+now.getFullYear()+' '+
					dateTimePad(now.getHours())+':'+dateTimePad(now.getMinutes()),
				r: 1, g: 1, b: 1
			}
		);
	},
	CLOCK_INTERVAL
);
