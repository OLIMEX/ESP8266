(function ($) {
	$.fn.bits = function () {
		return this.find(':checkbox').each(
			function (i, e) {
				var $this = $(e);
				var name = $this.attr('name');
				var match = name.match(/(\w+)\[(\d+)\]/);
				if (match) {
					var $target = $this.closest('form').find('input[name='+match[1]+']')
					var mask = 1 << match[2];
					
					$target.bind(
						'refresh',
						function () {
							$this.prop('checked', ($target.val() & mask) != 0);
						}
					);
					
					$this.change(
						function() {
							if ($this.is(':checked')) {
								$target.val($target.val() | mask);
							} else {
								$target.val($target.val() & (~mask));
							}
						}
					);
					
					$target.trigger('refresh');
				}
			}
		);
	}
})(jQuery);

(function ($) {
	$.fn.stationDHCP = function () {
		return this.each(
			function (i, e) {
				var $this = $(e);
				var $form = $this.closest('form');
				
				function dhcp() {
					$form.find('fieldset[name=IP] input').prop('disabled', $this.is(':checked'));
				}
				
				$this.bind('refresh', dhcp);
				$this.change(dhcp);
			}
		);
	}
})(jQuery);

(function ($) {
	$.fn.menu = function () {
		$(document).click(
			function(event) {
				$('.contextual').hide();
			}
		);
		
		return this.each(
			function (i, e) {
				var $this = $(e);
				$this.click(
					function (event) {
						$('#tabs').show();
						event.stopPropagation();
						return false;
					}
				);
			}
		);
	}
})(jQuery);

(function ($) {
	$.fn.range = function () {
		return this.each(
			function (i, e) {
				var $this = $(e);
				var $form = $this.closest('form');
				var timeout = null;
				var value;
				var $text = $this.closest('form').find('input[type=text][name='+$this.attr('name')+']:disabled');
				
				$this.bind(
					'input',
					function () {
						$this.data('refresh', false);
						if ($text.length > 0) {
							$text.val($this.val());
						}
					}
				);
				
				$this.change(
					function (event) {
						value = $this.val();
						$this.data('refresh', false);
						if (timeout) {
							clearTimeout(timeout);
						}
						
						timeout = setTimeout(
							function () {
								$this.val(value);
								
								var data = {};
								data[$this.attr('name')] = parseInt($this.val());
								$form.trigger('post8266', data);
								
								$this.data('refresh', true);
								timeout = null;
							},
							200
						);
					}
				);
			}
		);
	}
})(jQuery);

(function ($) {
	$.fn.formTabs = function (selector, titleSelector) {
		var $this = $(this);
		
		if ($this.css('position') == 'absolute') {
			$('#menu').show();
			$this.addClass('contextual');
		}
		
		$(selector).each(
			function (i, e) {
				var $e = $(e);
				var title = $e.find(titleSelector).text();
				var $tab = $('<div>'+title+'</div>').
					click(
						function (event) {
							$(selector).hide();
							$e.show().css('display', 'inline-block');
							$tab.parent().children().removeClass('active');
							$tab.addClass('active');
							
							if ($this.css('position') == 'absolute') {
								$this.hide();
							}
							
							event.stopPropagation();
							return false;
						}
					)
				;
				$this.append($tab);
				$e.data('tab', $tab);
				
				if ($e.hasClass('esp') || $e.hasClass('module')) {
					$tab.hide();
				}
				
				if ($e.hasClass('on-top')) {
					$tab.addClass('on-top');
				}
			}
		);
		$this.children(':first').click();
	};
})(jQuery);

(function ($) {
	$.fn.serializeJSON = function() {
		var json = {};
		
		this.children().each(
			function (i, e) {
				var $e = $(e);
				if (
					$e.is(':button') || 
					$e.is(':disabled') ||
					($e.is(':checkbox') && $e.parents('.bits').length > 0)
				) {
					return json;
				}
				
				if ($e.is(':file')) {
					json[$e.attr('name')] = 'FILE-DATA:'+(typeof e.files != 'undefined' && e.files.length > 0 ? e.files[0].size : 0);
					return json;
				}
				
				if ($e.is(':input')) {
					$e.trigger('toJSON');
					var val = $e.data('JSON') ? $e.data('JSON') : $e.val();
					$e.data('JSON', null);
					json[$e.attr('name')] = $e.is(':not(:checkbox)') || $e.is(':checked') ?
						(val === null ?
							null
							:
							(typeof val == 'string' && val.match(/^-?\d+$/) ?
								parseInt(val)
								:
								val
							)
						)
						:
						0
					;
					return json;
				}
				
				if ($e.attr('name') != undefined) {
					json[$e.attr('name')] = $e.serializeJSON();
				} else {
					$.extend(json, $e.serializeJSON());
				}
			}
		);
		return json;
	};
})(jQuery);

(function ($) {
	var files = null;
	
	$.fn.serializeFiles = function(ready) {
		if (ready) {
			files = {
				readers: [],
				
				loaded: function () {
					for (var i in this.readers) {
						if (this.readers[i].readyState != FileReader.DONE) {
							return;
						}
					}
					
					if (ready) {
						ready(this.readers);
					}
				}
			};
		}
		
		this.children().each(
			function (i, e) {
				var $e = $(e);
				
				if ($e.is(':file')) {
					if (typeof e.files[0] != 'undefined') {
						var reader = new FileReader();
						reader.onload = function () {
							files.loaded();
						}
						
						reader.onerror = function () {
							files.loaded();
						}
						
						files.readers[$e.attr('name')] = reader;
						reader.readAsArrayBuffer(e.files[0]);
					}
					return files;
				}
				
				$e.serializeFiles();
			}
		);
		
		if (ready) {
			files.loaded();
		}
	};
})(jQuery);

(function ($) {
	$.fn.unserializeJSON = function(json, reset) {
		if (typeof reset === "undefined" || reset === null) { 
			reset = true; 
		}
		
		if (this.is('form') && !this.hasClass('no-reset') && reset) {
			this.get(0).reset();
		}
		
		for (var name in json) {
			var i = this.find('[name="'+name+'"]');
			if (i.is(':input')) {
				if (i.is(':checkbox')) {
					if (json[name]) {
						i.prop('checked', true);
					} else {
						i.prop('checked', false);
					}
				} else {
					if (i.data('refresh') === false) {
						continue;
					}
					i.trigger('fromJSON', [json[name]]);
					i.val(i.data('JSON') ? i.data('JSON') : json[name]);
					i.data('JSON', null);
				}
				i.trigger('refresh');
			} else if (typeof json[name] === 'object') {
				i.unserializeJSON(json[name]);
			}
		}
	}
})(jQuery);

(function ($) {
	$.fn.status = function() {
		$.extend(
			this, {
				message : function (msg, css) {
					return this.each(
						function (i, e) {
							var $this = $(e);
							var status = $this.hasClass('status');
							var active = $this.hasClass('active');
							var onTop  = $this.hasClass('on-top');
							$this.removeClass();
							
							if (typeof css != 'undefined' && css != 'activity' && css != 'error') {
								$this.show();
								if (onTop) {
									$this.click();
								}
							}
							
							if (
								msg.indexOf('SSL is not enabled') >= 0 ||
								msg.indexOf('URL Not Found')      >= 0 ||
								msg.indexOf('Device not found')   >= 0
							) {
								$this.hide();
							}
							
							if (status) {
								$this.html(msg);
								$this.addClass('status');
							}
							
							if (active) {
								$this.addClass('active');
							}
							
							if (onTop) {
								$this.addClass('on-top');
							}
							
							if (css) {
								$this.addClass(css);
							}
						}
					);
				}
			}
		);
		
		return this;
	};
})(jQuery);

(function ($) {
	$.fn.connection = function() {
		return this.each(
			function (i, e) {
				var $this = $(e);
				
				var $status = $this.find('.status').add($this.data('tab')).status();
				
				e.message = function (msg, css) {
					$status.message(msg, css);
				}
				
				$this.bind(
					'error8266',
					function (event, data) {
						$status.message(data, 'error');
					}
				);
				
				$this.bind(
					'event8266',
					function (event, data) {
						$status.message('OK', 'event');
					}
				);
				
				$this.bind(
					'abort8266',
					function (event, data) {
						$this.data('tab').click();
					}
				);
				
				$this.submit(
					function (event) {
						$status.message('Connecting...', 'activity');
						$('.esp').trigger('init8266', true);
						event.preventDefault();
						return false;
					}
				);
				
				$this.find('button[type=button]').click(
					function () {
						$('.esp').trigger('abort8266');
					}
				);					
			}
		);
	};
})(jQuery);


(function ($) {
	$.fn.event8266 = function() {
		return this.each(
			function (i, e) {
				var $this = $(e);
				
				if (typeof e.message == 'undefined') {
					e.message = function () {};
				}
				
				$this.bind(
					'error8266',
					function (event, data) {
						if ($this.is(':not(.long-poll)') && $this.get(0).message) {
							$this.get(0).message(data, 'error');
						}
					}
				);
				
				$this.bind(
					'event8266',
					function (event, data) {
						var reset = false;
						if (typeof data.EventData === 'object') {
							if ($this.is(':not(.long-poll)') && data.EventURL != $this.attr('action')) {
								return;
							}
							reset = true;
							data = data.EventData;
						} else {
							if (
								typeof data.Device != 'undefined' && 
								typeof data.Error == 'undefined' &&
								data.Device == 'ESP8266'
							) {
								return;
							}
						}
						
						if (typeof data.Error != 'undefined') {
							$this.get(0).message(data.Error, 'error');
							return;
						}
						
						if (typeof data.Data === 'object') {
							$this.unserializeJSON(data.Data, reset);
						}
						
						if ($this.is('.long-poll')) {
							$this.find('input[name=Event]').val(data.Device);
						}
						
						if ($this.is('.esp:not(.long-poll)')) {
							if (typeof data.Status != 'undefined') {
								$this.get(0).message(data.Status, 'event');
							}
							return;
						}
						
						setTimeout(
							function () {
								$this.get(0).reset();
							},
							500
						);
					}
				);
			}
		);
	};
})(jQuery);
	
(function ($) {
	$.fn.esp8266 = function() {
		var ssl, websocket, baseURL, timeout, user, password;
		var Commands = $.Conveyor();
		var socketURL;
		
		var longPollAction = $('.long-poll').attr('action');
		
		if (typeof window.WebSocket == 'undefined') {
			$('#esp input[name=websocket]').prop('checked', false);
			$('#esp input[name=websocket]').prop('disabled', true);
		} else {
			$('#esp input[name=websocket]').prop('checked', true);
		}
		
		function init() {
			websocket = $('#esp input[name=websocket]').is(':checked');
			ssl       = $('#esp input[name=ssl]').is(':checked');
			
			socketURL = 
				'ws' + (ssl ? 's' : '') + '://' + 
				$('#esp input[name=host]').val() +
				longPollAction
			;
			
			baseURL   = 
				'http' + (ssl ? 's' : '') + '://' + 
				$('#esp input[name=host]').val()
			;
			
			user      = $('#esp input[name=user]').val();
			password  = $('#esp input[name=password]').val();
		}
		
		return this.each(
			function (i, e) {
				var $this = $(e);
				var $status = $this.find('.status').add($this.data('tab')).status();
				var longPoll = $this.is('.long-poll');
				var timeout = longPoll ? 30000 : 5000;
				if ($this.data('timeout')) {
					timeout = $this.data('timeout') * 1000;
				}
				
				function commandError(status, error, xhr) {
					// console.log('error(%s)', baseURL + $this.attr('action'));
					$status.message(status+': '+error, 'error');
					if (longPoll && status != 'abort') {
						if (xhr.status == 0) {
							$this.trigger('init8266');
						} else {
							$this.addClass('disabled');
							$status.message('['+xhr.status+'] '+status+': '+error, 'error');
						}
					}
				}
				
				function commandSuccess(data, status, xhr) {
					// console.log('success(%s)', baseURL + $this.attr('action'));
					if (typeof data.Error != 'undefined') {
						$status.message(data.Device+': '+data.Error, 'error');
						return;
					}
					
					if (typeof data.Data === 'object') {
						$this.unserializeJSON(data.Data);
					}
					
					if (typeof data.Status != 'undefined') {
						$status.message(data.Status, 'event');
						if (data.Status == 'Rebooting') {
							Commands.abort('Rebooting');
							return;
						}
					} else {
						$status.message('OK', 'event');
					}
					
					$('#esp').trigger('event8266');
					if (longPoll) {
						$('form').trigger('event8266', data);
						$this.trigger('init8266');
					}
				}
				
				e.message = function (msg, css) {
					$status.message(msg, css);
				}
				
				$this.bind(
					'init8266',
					function (event, force) {
						init();
						
						if (websocket && longPoll) {
							return;
						}
						
						if (typeof force != 'undefined' && force) {
							Commands.aborted = false;
						}
						
						$status.message('Refreshing...', 'activity');
						$this.removeClass('disabled');
						Commands.pipe({
							retry: 2,
							websocket: websocket,
							longPoll: longPoll,
							
							socketURL: socketURL,
							baseURL: baseURL,
							action: $this.attr('action'),
							
							type: 'GET',
							timeout: timeout,
							user: user,
							password: password,
							
							event: 'event8266',
							
							success: commandSuccess,
							error: commandError
						});
					}
				);
				
				$this.bind(
					'abort8266',
					function () {
						var $tab = $this.data('tab');
						if ($tab) {
							$tab.hide();
						}
						if (!longPoll) {
							$this.hide();
						}
						Commands.abort();
					}
				);
				
				$this.find('button[type=button]:contains(Refresh)').click(
					function() {
						// console.log('');
						$this.trigger('init8266');
					}
				);
				
				$this.bind(
					'post8266',
					function (event, jsonData) {
						init();
						$status.message('Submit...', 'activity');
						
						Commands.pipe({
							retry: 2,
							websocket: websocket,
							longPoll: false,
							
							socketURL: socketURL,
							baseURL: baseURL,
							action: $this.attr('action'),
							
							type: $this.attr('method'),
							data: jsonData,
							timeout: timeout,
							user: user,
							password: password,
							
							event: 'event8266',
							
							success: commandSuccess,
							error: commandError
						});
					}
				);
				
				$this.submit(
					function (event) {
						init();
						$status.message('Submit...', 'activity');
						var jsonData = $this.serializeJSON();
						
						$this.serializeFiles(
							function (files) {
								var error = false;
								var fileData = new Blob();
								var count = 0;
								for (var i in files) {
									if (files[i].error) {
										commandError(files[i].error.name, files[i].error.message);
										error = true;
									} else {
										fileData = new Blob([
											fileData,
											i + ':' + files[i].result.byteLength + ':',
											files[i].result
										]);
										count++;
									}
								}
								
								if (count) {
									fileData = new Blob([String.fromCharCode(0), fileData]);
								} else {
									fileData = null;
								}
								
								if (!error) {
									Commands.pipe({
										retry: 2,
										websocket: websocket,
										longPoll: false,
										
										socketURL: socketURL,
										baseURL: baseURL,
										action: $this.attr('action'),
										
										type: $this.attr('method'),
										data: jsonData,
										files: fileData,
										timeout: timeout,
										user: user,
										password: password,
										
										event: 'event8266',
										
										success: commandSuccess,
										error: commandError
									});
								}
							}
						);
						
						event.preventDefault();
						return false;
					}
				);
				
			}
		);
	}
})(jQuery);

/*** Requests Conveyor  */

(function ($) {
	$.Conveyor = function (settings) {
		return new Conveyor(settings);
	};
	
	function Conveyor(settings) {
		this.settings = {};
		this.queue  = new Array();
		
		$.extend(this.settings, settings);
	}

	Conveyor.prototype = {
		aborted: false,
		current: null,
		
		ajax: null,
		
		socket: null,
		
		_push: function (request) {
			if (this.aborted) {
				// console.log('_push(%s) ABORTED', request.url);
				$('form').trigger('error8266', 'Not connected');
				return;
			}
			
			if (this.current) {
				if (
					this.current.longPoll &&
					this.current.status != 'done' &&
					this.current.baseURL == request.baseURL &&
					this.current.action == request.action &&
					request.longPoll
				) {
					// console.log('_push(%s) IGNORED', request.url);
					return;
				}
			}
			
			request.status = 'waiting';
			this.queue.push(request);
			this.queue.sort(
				function (a, b) {
					return (a.longPoll - b.longPoll);
				}
			);
		},
		
		pipe: function (request) {
			// console.log('pipe(%s)', request.action);
			this._push(request);
			var conveyor = this;
			setTimeout(
				function () {
					conveyor.run();
				},
				100
			);
		},
		
		add: function (request) {
			// console.log('add(%s)', request.action);
			this._push(request);
		},
		
		length: function () {
			return this.queue.length;
		},
		
		_abortAjax: function() {
			if (this.ajax) {
				// console.log('abort(AJAX)');
				this.ajax.abort();
				this.ajax = null;
			}
			this.queue = this.queue.filter(
				function (e) {
					return !(e.longPoll && e.status == 'waiting');
				}
			);
		},
		
		_abortSocket: function() {
			if (this.socket) {
				// console.log('abort(%s)', this.socket.url);
				this.socket.close(1000);
				this.socket = null;
			}
		},
		
		clearSocketRequests: function() {
			if (this.current && this.current.websocket) {
				this.current = null;
			}
			this.queue = this.queue.filter(
				function (e) {
					return !e.websocket;
				}
			);
		},
		
		abort: function (message) {
			this.aborted = true;
			
			this._abortAjax();
			this._abortSocket();
			
			this.queue = new Array();
			this.current = null;
			$('form').trigger('error8266', typeof message == 'undefined' ? 'Not connected' : message);
		},
		
		before: function (callback) {
			this.beforeCallback = callback;
			return this;
		},
		
		done: function (callback) {
			this.doneCallback = callback;
			return this;
		},
		
		run: function () {
			if (
				this.length() == 0 && 
				(
					this.current == null ||
					this.current.status == 'done'
				)
			) {
				if (this.doneCallback) {
					this.doneCallback();
				}
				return;
			}
			
			if (this.current) {
				if (this.current.status != 'done') {
					if (this.current.longPoll && this.length() != 0) {
						this._abortAjax();
						this.add(this.current);
					} else {
						return;
					}
				}
			}
			this.current = this.queue.shift();
			
			if (this.beforeCallback) {
				this.beforeCallback(this.current);
			}
			
			this.ajax = this._execute(this.current);
		},
		
		_execute: function (request) {
			// console.log('_execute(%s)', request.action);
			var conveyor = this;
			
			if (request.websocket) {
				this._abortAjax();
				
				var data = JSON.stringify({
						URL: request.action,
						Method: request.type,
						Data: request.data
				});
				
				if (request.files) {
					data = new Blob([
						data,
						request.files
					]);
				}
				
				if (this.socket === null || this.socket.url != request.socketURL) {
					if (this.socket) {
						this.socket.close(1000);
					}
					
					this.socket = new WebSocket(request.socketURL);
					
					this.socket.onopen = function () {
						this.send(
							JSON.stringify(
								{
									User: request.user,
									Password: request.password
								}
							)
						);
						
						this.send(data);
						
						setTimeout(
							function () {
								request.status = 'done';
								conveyor.run();
							},
							100
						);
					}
					
					this.socket.onmessage = function (event) {
						try {
							$('form').trigger(request.event, JSON.parse(event.data));
						} catch (e) {
							// console.log(e.message);
							console.log(event.data);
						} 
					};
					
					this.socket.onerror = function (event) {
						console.log("WebSocket ERROR");
						console.log(event);
						// $('form').trigger('error8266', event.reason);
					};
					
					this.socket.onclose = function (event) {
						if (event.code != 1000) {
							$('form').trigger('error8266', event.code+': '+(event.reason ? event.reason : 'WebSocket error'));
						}
						if (this.readyState != WebSocket.OPEN) {
							conveyor.socket = null;
							conveyor.clearSocketRequests();
						}
						$('form').trigger('abort8266');
					};
				} else {
					if (this.socket.readyState != WebSocket.OPEN) {
						$('form').trigger('error8266', 'Not connected');
						if (this.socket.readyState != WebSocket.CONNECTING) {
							this.socket = null;
							this.clearSocketRequests();
						}
						return;
					}
					
					if (this.socket.bufferedAmount == 0) {
						this.socket.send(data);
					}
					
					setTimeout(
						function () {
							request.status = 'done';
							conveyor.run();
						},
						100
					);
				}
				return;
			}
			
			this._abortSocket();
			
			request.status = 'running';
			
			var ajax = {
				url: request.baseURL + request.action,
				type: request.type,
				timeout: request.timeout,
				headers: {
					Authorization: 'Basic '+btoa(request.user+':'+request.password)
				}
			};
			
			if (typeof request.data != 'undefined') {
				ajax.data = request.files ?
					new Blob([JSON.stringify(request.data), request.files])
					:
					JSON.stringify(request.data)
				;
				// ajax.dataType = 'json';
				ajax.contentType = false;
				ajax.processData = false;
			}
			
			return $.ajax(ajax).
			done(
				function (data, status, xhr) {
					// console.log('done(%s)', request.url);
					request.status = 'done';
					conveyor.ajax = null;
					
					if (request.success) {
						request.success(data, status, xhr);
					}
					
					conveyor.run();
				}
			).fail(
				function (xhr, status, error) {
					// console.log('fail(%s, %s, %s)', request.url, status, error);
					if (status == 'abort') {
						request.status = 'done';
						return;
					}
					
					if (error == 'Not Found') {
						error = 'URL Not Found';
					}
					
					if (xhr.status != 0 || status == 'timeout') {
						request.retry = 0;
					}
					
					if (request.retry > 0) {
						request.retry--;
						request.status = 'failed';
					} else {
						request.status = 'done';
					}
					conveyor.ajax = null;
					
					if (request.error) {
						request.error(status, error, xhr);
					}
					
					setTimeout(
						function () {
							if (request.status == 'done') {
								conveyor.run();
							} else {
								conveyor.ajax = conveyor._execute(request);
							}
						},
						100
					);
				}
			);
		}
		
	};
})(jQuery);
