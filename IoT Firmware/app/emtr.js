(function ($) {
	$.fn.emtr = function () {
		return this.each(
			function (i, e) {
				var $this = $(e);
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
								$e.data('factor', 32768);
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
						$this.trigger('refresh');
						$this.submit();
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
				var $counterActive = $this.closest('form').find('input[name=CounterActive]');
				var $counterApparent = $this.closest('form').find('input[name=CounterApparent]');
				
				$this.click(
					function () {
						if (confirm('Do you really want to reset counters?')) {
							$counterActive.prop('disabled', false).val(0);
							$counterApparent.prop('disabled', false).val(0);
							$this.submit();
							$counterActive.prop('disabled', true);
							$counterApparent.prop('disabled', true);
						}
					}
				);
			}
		);
	}
})(jQuery);
