var MOD_IRDA = {
	RC5: {
		Device: [
			{id:  0, name: 'TV 1'},
			{id:  1, name: 'TV 2'},
			{id:  5, name: 'VCR 1'},
			{id:  6, name: 'VCR 2'},
			{id:  8, name: 'Sat 1'},
			{id: 10, name: 'Sat 2'},
			{id: 17, name: 'Radio'},
			{id: 20, name: 'CD'},
			{id: 26, name: 'CD-R'},
		],
		Command: [
			{id: 16, name: 'Volume +'},
			{id: 17, name: 'Volume -'},
			{id: 13, name: 'Mute'},
			{id: 32, name: 'Channel +'},
			{id: 33, name: 'Channel -'},
		]
	},
	
	SIRC: {
		Device: [
			{id:  1, name: 'TV'},
			{id:  2, name: 'VCR 1'},
			{id:  3, name: 'VCR 2'},
			{id: 17, name: 'CD'},
		],
		Command: [
			{id: 16, name: 'Channel  +'},
			{id: 17, name: 'Channel  -'},
			{id: 18, name: 'Volume  +'},
			{id: 19, name: 'Volume  -'},
			{id: 20, name: 'Mute'},
		]
	}
};
		
(function ($) {
	$.fn.irdaMode = function () {
		return this.each(
			function (i, e) {
				var $this = $(e);
				$this.change(
					function () {
						$this.parent().find('select[name!='+$this.attr('name')+']').find('option').remove();
						for (var name in MOD_IRDA[$this.val()]) {
							var select = $this.parent().find('select[name='+name+']');
							for (var i in MOD_IRDA[$this.val()][name]) {
								select.append('<option value="'+MOD_IRDA[$this.val()][name][i].id+'">'+MOD_IRDA[$this.val()][name][i].name+'</option>');
							}
						}
					}
				);
				$this.val('RC5').change();
			}
		);
	}
})(jQuery);
