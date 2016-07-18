(function ($) {
	$.fn.emtr = function () {
		return this.each(
			function (i, e) {
				var $this = $(e);
				
				$this.find('select[name=Mode]').emtrMode();
				$this.find('.emtr-reset').emtrReset();
				$this.find('button.calibrate').emtrCalibrate();
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
