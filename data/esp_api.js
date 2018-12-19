$("#wifisettings.tab-pane").show(function(){
		$.get("./wifi","").done(callback_get_wifi_settings).fail(function(data){alert('GetWiFiFailed: '+JSON.stringify(data))});
	});
$("#form_mqtt_settings#mqtt_showpassword").click(function(){
		
	})
$('a[href="#seedata"]').on("shown.bs.tab",function(){
		var dataTab = $("#seedata")
		var datajson;
		$.get("./data").done(function (data){
				//dataTab.text(JSON.stringify(data));
				//alert(data);
				if (typeof(data) == "string"){
					data = JSON.parse(data)
				}
				var table = $('#seedata #tblData');
				table.children('tr').remove();
				table.append('<tr><th>ID</th><th>Name</th><th>Value</th></tr>')
				ds = data['DS18B20'];
				for (var id in ds){
					var curName = ds[id].name?ds[id].name:"";
					table.append('<tr><td>'+id+'</td><td>'+curName+'</td><td>'+ds[id].celsium+'</td></tr>');
					
				}
			}).fail(function(a,b,c){alert ("fail " + b)})
		dataTab.append();
	})	
	
$("#form_wifi_settings").submit(function(e){
		//e.PreventDefault();
		var url = './wifi'

		// if ($("#wifi_ssid").val() == "-1"){
			// $("#group-ssid").removeClass("success").removeClass("warning").addClass("error");
			// return;
		// }


		$.post(url, $(this).serialize())
			.done(function(data){
				alert("Set wifi settings OK. " + JSON.stringify(data));
			})
			.fail(function(data){
				alert("Set wifi settings Failed. " + JSON.stringify(data));
			})
	});
$("#form_mqtt_settings").submit(function(e){
	var url = './mqtt'
	
	$.post(url,$(this).serialize())
		.done(function(data){
			alert("Set mqtt settings OK. " + JSON.stringify(data));
		})
		.fail(function(data){
			alert("Set mqtt settings Failed. " + JSON.stringify(data));
		})
	//e.PreventDefaults();
});
function callback_get_wifi_settings(data)
{	
	if (typeof(data) == "string"){
		data = JSON.parse(data)
	}
	var selSSID = $("#wifi_ssid");	
	$.each(data.networks, function (index, name){selSSID.append('<option value="'+name+'">'+name+'</option>')});
	
}

function getDS18B20Alias(id)
{
	return "todo"
}
// Example starter JavaScript for disabling form submissions if there are invalid fields
(function() {
  'use strict';
  window.addEventListener('load', function() {
    // Fetch all the forms we want to apply custom Bootstrap validation styles to
    var forms = document.getElementsByClassName('needs-validation');
    // Loop over them and prevent submission
    var validation = Array.prototype.filter.call(forms, function(form) {
      form.addEventListener('submit', function(event) {
        if (form.checkValidity() === false) {
          event.preventDefault();
          event.stopPropagation();
        }
        form.classList.add('was-validated');
      }, false);
    });
  }, false);
})();