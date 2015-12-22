(function ($) {
	$.fn.emtr = function () {
		return this.each(
			function (i, e) {
				var $this = $(e);
				$this.find('.integer').
					bind(
						'toJSON',
						function () {
							var $this = $(this);
							$this.data('JSON', parseInt($this.val()));
						}
					)
				;
				
				$this.find('.milli, .deci, .normalize, .kWh').
					each(
						function (i, e) {
							var $e = $(e);
							$e.data('factor', 1);
							$e.data('precision', 0);
							if ($e.is('.milli')) {
								$e.data('factor', 1000);
								$e.data('precision', 3);
							} else if ($e.is('.deci')) {
								$e.data('factor', 10);
								$e.data('precision', 1);
							} else if ($e.is('.normalize')) {
								$e.data('factor', 32767);
								$e.data('precision', 5);
							} else if ($e.is('.kWh')) {
								$e.data('factor', 3600000);
								$e.data('precision', 3);
							}
						}
					).
					
					bind(
						'fromJSON',
						function (event, value) {
							var $this = $(this);
							$this.data('JSON', (value / $this.data('factor')).toFixed($this.data('precision')));
						}
					).
					bind(
						'toJSON',
						function () {
							var $this = $(this);
							$this.data('JSON', $this.val() * $this.data('factor'));
						}
					)
				;
			}
		);
	}
})(jQuery);

(function ($) {
	$.fn.emtrMode = function () {
		return this.each(
			function (i, e) {
				var $this = $(e);
				var $form = $this.closest('form');
				
				$this.bind(
					'refresh',
					function () {
						var value = $this.val().toLowerCase();
						$form.find('div.emtr-mode').hide();
						$form.find('div.emtr-mode.'+value).show();
						$('.bits').bits();
					}
				);
				
				$this.change(
					function () {
						var data = {};
						data[$this.attr('name')] = $this.val();
						$this.trigger('refresh');
						$form.trigger('post8266', data);
					}
				);
				
				$this.trigger('refresh');
			}
		);
	}
})(jQuery);

(function ($) {
	$.fn.emtrReset = function () {
		return this.each(
			function (i, e) {
				var $this = $(e);
				var $form = $this.closest('form');
				var $counters = $form.find('input.counter');
				
				$this.click(
					function () {
						if (confirm('Do you really want to reset counters?')) {
							var data = {};
							$counters.each(
								function (i, e) {
									data[e.name] = 0;
								}
							);
							$form.trigger('post8266', data);
						}
					}
				);
			}
		);
	}
})(jQuery);

(function ($) {
	$.fn.emtrCalibrate = function () {
		return this.each(
			function (i, e) {
				var $this = $(e);
				var $form = $this.closest('form');
				
				$this.click(
					function () {
						if (confirm('Do you really want to calibrate?')) {
							var names = $this.attr('class').split(/\s+/);
							var data = {CalcCalibration: 1};
							$form.find('[name='+names.join('],[name=')+']').each(
								function (i, e) {
									var $e = $(e);
									$e.trigger('toJSON');
									data[e.name] = $e.data('JSON') ? $e.data('JSON') : $e.val();
								}
							);
							$form.trigger('post8266', data);
						}
					}
				);
			}
		);
	}
})(jQuery);
