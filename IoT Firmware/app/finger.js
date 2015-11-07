(function ($) {
	$.fn.fingerMode = function () {
		return this.each(
			function (i, e) {
				var $this = $(e);
				$this.closest('form').submit(
					function () {
						$('#finger-delete').hide();
					}
				);
				
				$this.change(
					function () {
						var value = $this.val();
						if (value != 'Delete') {
							if (value != 'Empty DB' || confirm('Are you sure?')) {
								$this.parent().submit();
							} else {
								$this.val('Read');
							}
							return;
						}
						$('#finger-delete').show();
					}
				);
			}
		);
	}
})(jQuery);
