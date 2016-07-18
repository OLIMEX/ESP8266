(function ($) {
	$.fn.switch_dev = function () {
		return this.each(
			function (i, e) {
				var $this = $(e);
				
				$this.find('select[name=function]').each(
					function (i, e) {
						var $select = $(e);
						var $fieldset = $select.closest('fieldset');
						var $group = $fieldset.find('div.function');
						var $interval = $fieldset.find('input[name=interval]');
						var $preference = $fieldset.find('input[name^=Preference]');
						
						$select.on(
							'change',
							function (event, value) {
								if (typeof value == 'undefined') {
									value = Number($select.val());
									$preference.val(value);
								}
								
								if (value >= 0 && value <= 2) {
									if (value != $select.val()) {
										$select.val(value);
									}
									$group.hide();
								} else {
									if (value != $select.val()) {
										$select.val(Math.sign(value) * 1000);
									}
									$group.show();
									$interval.val(Math.abs(value));
								}
							}
						);
						
						$interval.change(
							function (event) {
								var value = Number($interval.val());
								$preference.val(Math.sign($select.val()) * value);
							}
						);
						
						$preference.bind(
							'fromJSON',
							function (event, value) {
								$select.trigger('change', value);
							}
						);
						
					}
				);
			}
		);
	};
})(jQuery);
