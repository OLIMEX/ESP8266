var TwitterStream = require('node-tweet-stream');
var Triggers      = require('./triggers');
var Properties    = require('./properties');

var TRACK = [
	'#OpenFest15'
];

if (typeof process.env.TWITTER_CONSUMER_KEY == 'undefined') {
	throw new Error('TWITTER ERROR: Missing TWITTER_CONSUMER_KEY environment variable');
}

if (typeof process.env.TWITTER_CONSUMER_SECRET == 'undefined') {
	throw new Error('TWITTER ERROR: Missing TWITTER_CONSUMER_SECRET environment variable');
}

if (typeof process.env.TWITTER_TOKEN == 'undefined') {
	throw new Error('TWITTER ERROR: Missing TWITTER_TOKEN environment variable');
}

if (typeof process.env.TWITTER_TOKEN_SECRET == 'undefined') {
	throw new Error('TWITTER ERROR: Missing TWITTER_TOKEN_SECRET environment variable');
}

// Init Twitter Stream 
var Twitter = new TwitterStream(
	{
		consumer_key    : process.env.TWITTER_CONSUMER_KEY,
		consumer_secret : process.env.TWITTER_CONSUMER_SECRET,
		token           : process.env.TWITTER_TOKEN,
		token_secret    : process.env.TWITTER_TOKEN_SECRET
	}
);

// Handle tweets
Twitter.on(
	'tweet', 
	function (tweet) {
		console.log('TWEET: ['+tweet.user.name+'] '+tweet.text+' \n');
		Triggers.fire({
			TwitterEvent: 'Tweet',
			EventData: {
				user: tweet.user.name,
				text: tweet.text
			}
		});
	}
);

// Handle Twitter errors
Twitter.on(
	'error', 
	function (err) {
		console.log('TWEET ERROR: '+err+'\n');
	}
);

// Init tracks
for (i in TRACK) {
	Twitter.track(TRACK[i]);
}

Properties.
	register(null, 'TweetUser', '$[?($.TwitterEvent == "Tweet")].EventData.user').
	register(null, 'TweetText', '$[?($.TwitterEvent == "Tweet")].EventData.text')
;

module.exports = Twitter;
