//var root_url = "http://demo9239551.mockable.io"
var root_url = "."
$("#wifisettings.tab-pane").show(function(){
		$.get(root_url+"/wifi","").done(callback_get_wifi_settings).fail(function(data){alert('GetWiFiFailed: '+JSON.stringify(data))});
	});
$("#form_mqtt_settings#mqtt_showpassword").click(function(){
		
	})
$('a[href="#seedata"]').on("shown.bs.tab",function(){
		var dataTab = $("#seedata")
		var datajson;
		$.get(root_url+"/data").done(function (data){
				//dataTab.text(JSON.stringify(data));
				//alert(data);
				if (typeof(data) == "string"){
					data = JSON.parse(data)
				}
				var table = $('#seedata #tblData');
				table.children("tr").remove();
				table.append('<tr><th>Name</th><th>Celsuim</th><th>ID</th></tr>')
				ds = data['DS18B20'];
				for (var id in ds)
				{
					var tr = $("<tr>");
					tr.append("<td>")
					var td = tr.find("td")
					td.append('<div class="input-group mb-3">\
								  <input type="text" id="name" class="form-control" placeholder="No name yet" aria-label="Name" aria-describedby="basic-addon2">\
								  <div class="input-group-append">\
									<button class="save btn btn-outline-secondary" type="button">Save</button>\
								  </div>\
								</div>');
					var input = td.find("#name")
					input.val( ds[id].name?ds[id].name:"");
					input.id = id;
					var btnSave = td.find(":button");
					btnSave.removeAttr("onclick")
					btnSave.attr("onclick","saveDS18Name(this)");
					
					var td_celsium = $("<td>",{"text": ds[id].celsium});
					tr.append(td_celsium)
					

					var td_id = $("<td>",{"text":id})
					tr.append(td_id)

					table.append(tr);
				}
			}).fail(function(a,b,c){alert ("fail " + b)})
		//dataTab.append();
	})	
	
$("#form_wifi_settings").submit(function(e){
		//e.PreventDefault();
		var url = root_url+'/wifi'

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
	var url = root_url+'/mqtt'
	
	$.post(url,$(this).serialize())
		.done(function(data){
			alert("Set mqtt settings OK. " + JSON.stringify(data));
		})
		.fail(function(data){
			alert("Set mqtt settings Failed. " + JSON.stringify(data));
		})
	//e.PreventDefaults();
});

function saveDS18Name(e)
{
	alert(e.parentNode.parentNode.parentNode.parentNode.childNodes[2].textContent);
	var url = root_url+'/ds18b20/alias'
	var params = {  "id":e.parentNode.parentNode.parentNode.parentNode.childNodes[2].textContent,
					"name": e.parentNode.parentNode.childNodes[1].value
				 }
	//alert(JSON.stringify(params))
	$.get(url,params)
		.done(function(data){
			alert("Set Alias settings OK. " + JSON.stringify(data));
		})
		.fail(function(data){
			alert("Set Alias settings Failed. " + JSON.stringify(data));
		})
}


$("#form_ds18b20_setname").submit(function(e){
	var url = root_url+'/ds18b20/alias'
	var params = $(this).serialize()
	alert(JSON.stringify(params))
	$.get(url,params)
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
	alert(JSON.stringify(data));
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