$(document).ready(function() {

	$("#status").html("Connexion en cours...");

	$.get("config", function(data) {
		$("#status").html("Chargement en cours...");
		
		var power = data.power;
		var brightness = data.brightness;
		var color = data.color;
		var panels = data.panels;
		var grid = data.grid.split(",");
		var direction = data.direction;

		// Setting Power 

		var btnOn = $("#power-on");
		var btnOff = $("#power-off");

		btnOn.attr("class", power ? "btn btn-primary" : "btn btn-outline-primary");
		btnOff.attr("class", !power ? "btn btn-primary" : "btn btn-outline-primary");

		btnOn.on("click", function() {
			btnOn.attr("class", "btn btn-primary");
			btnOff.attr("class", "btn btn-outline-primary");
			postValue("power", 1, "Réglage de l'alimentation", "Alimentation réglée");
			power = 1;
			updatePreview();
		});
		btnOff.on("click", function() {
			btnOn.attr("class", "btn btn-outline-primary");
			btnOff.attr("class", "btn btn-primary");
			postValue("power", 0, "Réglage de l'alimentation", "Alimentation réglée");
			power = 0;
		});

		// Setting Selection
		var inputSelect = $("#input-select");

		var select = Array.from({length: panels}, (v, k) => k + 1);
		inputSelect.val(select.join());

		for(let i = 1; i <= panels; i++) {
			$("#btn-group-custom-selection").append('<button type="button" class="btn btn-primary" id="select-' + i + '">' + i + '</button>');
		}

		var btnQuickSelection = $("#btn-group-quick-selection button");
		var btnCustomSelection = $("#btn-group-custom-selection button");
		
		btnQuickSelection.on("click", function() {
			btnQuickSelection.attr("class", "btn btn-outline-primary");
			btnCustomSelection.attr("class", "btn btn-outline-primary");
		});
		
		btnCustomSelection.on("click", function() {
			btnQuickSelection.each(function(index, button) { 
				if($(button).attr("class") == "btn btn-primary") {
					$(this).attr("class", "btn btn-outline-primary");
				}
			});
			btnQuickSelection.attr("class", "btn btn-outline-primary");
		});
		
		var btnAll = $("#select-all");
		var btnNone = $("#select-none");
		var btnEven = $("#select-even");
		var btnOdd = $("#select-odd");
		
		btnAll.on("click", function() {
			$(this).attr("class", "btn btn-primary");
			select = Array.from({length: panels}, (v, k) => k + 1);
			inputSelect.val(select.join());
			for(let i = 0; i < select.length; i++) {
				$("#select-" + select[i]).attr("class", "btn btn-primary");
			}
		});		
		btnNone.on("click", function() {
			$(this).attr("class", "btn btn-primary");
			select.length = 0;
			inputSelect.val(select.join());
		});
		btnEven.on("click", function() {
			$(this).attr("class", "btn btn-primary");
			select = Array.from({length: panels}, (v, k) => k + 1).filter(n => n%2 === 0);
			inputSelect.val(select.join());
			for(let i = 0; i < select.length; i++) {
				$("#select-" + select[i]).attr("class", "btn btn-primary");
			}
		});
		btnOdd.on("click", function() {
			$(this).attr("class", "btn btn-primary");
			select = Array.from({length: panels}, (v, k) => k + 1).filter(n => n%2 !== 0);
			inputSelect.val(select.join());
			for(let i = 0; i < select.length; i++) {
				$("#select-" + select[i]).attr("class", "btn btn-primary");
			}
		});

		btnCustomSelection.each(function(index, button) {
			$(button).on("click", function() {
				var id = parseInt($(this).attr("id").substr(7));
				
				if(!select.includes(id)) {
					$(this).attr("class", "btn btn-primary");
					select.push(id);
				} else {
					$(this).attr("class", "btn btn-outline-primary");
					select.splice(select.indexOf(id), 1);
				}
				inputSelect.val(select.join());
			});
		});

		// Setting Brightness
		var inputBrightness = $("#input-brightness");
		var sliderBrightness = $("#slide-brightness");

		inputBrightness.val(brightness);
		sliderBrightness.val(brightness);

		sliderBrightness.on("mousemove", function() {
			inputBrightness.val($(this).val());
		});

		sliderBrightness.on("change", function() {
			var value = $(this).val();
			inputBrightness.val(value);
			postValue("brightness", value, "Réglage de la luminosité", "Luminosité réglée");

			if(value == 0) {
				btnOn.attr("class", "btn btn-outline-primary");
				btnOff.attr("class", "btn btn-primary");
				power = 0;
			}
		});

		inputBrightness.on("change", function() {
			var value = $(this).val();
			sliderBrightness.val(value);
			postValue("brightness", value, "Réglage de la luminosité", "Luminosité réglée");

			if(value == 0) {
				btnOn.attr("class", "btn btn-outline-primary");
				btnOff.attr("class", "btn btn-primary");
				power = 0;
			}
		});

		// Setting Color
		var inputColor = $("#input-color");

		inputColor.val("rgb(" + color + ")");
		inputColor.minicolors({
			theme: "bootstrap",
			changeDelay: 200,
			control: "wheel",
			format: "rgb",
			inline: true
		});

		$("button[id^=color-]").each(function(index, button) { 
			$(button).on("click", function() {
				var rgb = $(this).css("backgroundColor");
				inputColor.minicolors("value", {color: rgb});
			});
		});

		inputColor.on("change", function() {
			if(power) {
				var value = $(this).val();
				var components = rgbToComponents(value);
				postColor(components);
			}
		});
		
		// Setting Preview
		if(direction == "horizontal") { var w = 50; var h = Math.round(Math.sqrt(3) / 2 * w); var w_offset = 0.5; var h_offset = 1; }
		if(direction == "vertical") { var h = 50; var w = Math.round(Math.sqrt(3) / 2 * h); var w_offset = 1; var h_offset = 0.5; }
		var rows = []; var cols = [];
		
		for(let i = 0; i < panels; i++) {
			var params = gridToParams(grid[i]);
			var id = i + 1;
			
			cols.push(params.x);
			rows.push(params.y);
				
			var x0 = (params.x - 1) * w * w_offset; 
			var y0 = (params.y - 1) * h * h_offset;
			var d = params.d;

			$("#svg-preview").append('<polygon id="preview-' + id + '" points="' + createTriangle(x0, y0, w, h, d) + '" style="fill: rgb(' + color + ');" />');
		}
		$("#svg-preview").height(h * (Math.max(...rows) - 1) * h_offset + h).width(w * (Math.max(...cols) - 1) * w_offset + w);
		$("#svg-preview").html($("#svg-preview").html());
		updatePreview();

		$("#status").html("Prêt &#128521;");
	})
	.fail(function(error) {
		console.log("Erreur (configuration) : " + error);
	});

});

function postValue(name, value, before, after) {
	$("#status").html(before + " à " + value + " en cours...");

	$.post("/" + name, {value: value}, function(data) {
		$("#status").html(after + " à " + data + " &#128521;");;
	})
	.fail(function(error) {
		console.log("Erreur (réglage " + name + ") : " + error);
	});
}

function postColor(value) {
	panels = $("#input-select").val();

	$("#status").html("Réglage de la couleur à (" + value.r + "," + value.g + "," + value.b + ") en cours..."); 

	$.post("/color", {red: value.r, green: value.g, blue: value.b, panels: panels}, function(data) {
		$("#status").html("Couleur réglée à (" + data + ") &#128521;");
		updatePreview();
	})
	.fail(function(error) {
		console.log("Erreur (réglage couleur) : " + error);
	});
}

function updatePreview() {
	$.get("colors", function(data) {
		for(let i = 0; i < data.panels; i++) {
			$("#preview-" + (i + 1)).css("fill", "rgb(" + data.colors[i] + ")");
		}
	})
	.fail(function(error) {
		console.log("Erreur (couleurs) : " + error);
	});
}

function rgbToComponents(rgb) {
	var components = {};

	rgb = rgb.match(/^rgb\((\d+),\s*(\d+),\s*(\d+)\)$/);
	components.r = parseInt(rgb[1]);
	components.g = parseInt(rgb[2]);
	components.b = parseInt(rgb[3]);

	return components;
}

function gridToParams(grid) {
	var params = {};

	grid = grid.split("-");
	params.x = parseInt(grid[0]);
	params.y = parseInt(grid[1]);
	params.d = grid[2];

	return params;
}

function createTriangle(x, y, w, h, direction) {
	switch(direction) {
		case "top":
			var x1 = (w / 2) + x; var y1 = y;
			var x2 = w + x; var y2 = h + y;
			var x3 = x; var y3 = h + y;
		  break;
		case "right":
			var x1 = w + x; var y1 = (h / 2) + y;
			var x2 = x; var y2 = y;
			var x3 = x; var y3 = h + y;
		  break;
		case "bottom":
			var x1 = (w / 2) + x; var y1 = h + y;
			var x2 = w + x; var y2 = y;
			var x3 = x; var y3 = y;
		  break;
		case "left":
			var x1 = x; var y1 = (h / 2) + y;
			var x2 = w + x; var y2 = y;
			var x3 = w + x; var y3 = h + y;
		  break;
	}

	return x1 + ',' + y1 + ' ' + x2 + ',' + y2 + ' ' + x3 + ',' + y3;
}
