var templates = {};
templates['status'] = `
        <h3>System</h3>
        <table class="mui-table mui-table--bordered">
          <tr>
            <td><b>Uptime</b></td>
            <td>{{uptime}}</td>
          </tr>
          <tr>
            <td><b>Free heap</b></td>
            <td>{{free_heap}}</td>
          </tr>
          <tr>
            <td><b>Build</b></td>
            <td>{{build}}</td>
          </tr>
        </table>
        <h3>EvalBoard</h3>
        <table class="mui-table mui-table--bordered">
          <tr>
            <td><b>USR LED</b></td>
            <td>
              <div class="onoffswitch">
                <input type="checkbox" name="led" class="onoffswitch-checkbox" id="led-switch" {{#led}}checked{{/led}}>
                <label class="onoffswitch-label" for="led-switch">
                  <span class="onoffswitch-inner"></span>
                  <span class="onoffswitch-switch"></span>
                </label>
              </div>
            </td>
          </tr>
          <tr>
            <td><b>MINT Button</b></td>
            <td>
              <div class="onoffswitch">
                <input type="checkbox" name="button" class="onoffswitch-checkbox" id="button-switch" disabled="disabled" {{#button}}checked{{/button}}>
                <label class="onoffswitch-label" for="button-switch">
                  <span class="onoffswitch-inner"></span>
                  <span class="onoffswitch-switch"></span>
                </label>
              </div>
            </td>
          </tr>
        </table>
        <h3>Network</h3>
        <table class="mui-table mui-table--bordered">
          <tr>
            <td><b>Hostname</b></td>
            <td>{{hostname}}</td>
          </tr>
          <tr>
            <td><b>MAC</b></td>
            <td>{{mac}}</td>
          </tr>
          <tr>
            <td><b>IP</b></td>
            <td>{{ip}}</td>
          </tr>
          <tr>
            <td><b>Netmask</b></td>
            <td>{{netmask}}</td>
          </tr>
          <tr>
            <td><b>Gateway</b></td>
            <td>{{gateway}}</td>
          </tr>
          <tr>
            <td><b>DNS</b></td>
            <td>{{dns}}</td>
          </tr>
        </table>
`;
templates['config'] = `
        <h3>Clickboards</h3>
        <table class="mui-table mui-table--bordered">
          <tr>
            <th>Clickboard</td>
            <th>Port 1</td>
            <th>Port 2</td>
          </tr>
          <tr>
              <td>None</td>
              <td><input type="radio" name="port1" value="none" checked="checked"></td>
              <td><input type="radio" name="port2" value="none" checked="checked"></td>
          {{#clickboards}}
          <tr>
            <td>{{name_format}}</td>
            <td><input type="radio" name="port1" value="{{name}}" {{#port1_active}}checked="checked"{{/port1_active}} {{^port1_available}}disabled="disabled"{{/port1_available}}></td>
            <td><input type="radio" name="port2" value="{{name}}" {{#port2_active}}checked="checked"{{/port2_active}} {{^port2_available}}disabled="disabled"{{/port2_available}}></td>
          </tr>
          {{/clickboards}}
        </table>
		<h3>MQTT Client</h3>
		<table class="mui-table mui-table--bordered">
			<tr>
				<td>Use MQTT</td>
				<td><input type="checkbox" name="mqttSwitch" {{#mqttSwitch}}checked{{/mqttSwitch}}></td>
			</tr>
		</table>
`;
templates['color2'] = `
        <h3>Sensor</h3>
        <table class="mui-table mui-table--bordered">
          <tr>
            <th>Color</th>
            <th>Raw</th>
            <th>Normalized</th>
          </tr>
          <tr>
            <td>Red</td>
            <td>{{r}}</td>
            <td>{{r_norm}}</td>
          </tr>
          <tr>
            <td>Green</td>
            <td>{{g}}</td>
            <td>{{g_norm}}</td>
          </tr>
          <tr>
            <td>Blue</td>
            <td>{{b}}</td>
            <td>{{b_norm}}</td>
          </tr>
        </table>

        <h3>Color</h3>
        <table class="mui-table mui-table--bordered">
            <tr>
                <td><b>Hex</b></td>
                <td>#{{r_hex}}{{g_hex}}{{b_hex}}</td>
            </tr>
        </table>
        <div style="width:200px; height:100px; margin:auto; background:rgb({{r_dec}},{{g_dec}},{{b_dec}});"></div>
		<h3>MQTT Topics</h3>
		<table class="mui-table mui-table--bordered">
			<tr>
				<td>Topic Color</td>
				<td><input type="text" name="ctopic" size="50" value="{{ctopic}}"></td>
			</tr>
		</table>
`;
templates['thermo3'] = `
        <h3>Temperature</h3>
        <table class="mui-table mui-table--bordered">
            <tr>
                <td>Current temperature</td>
                <td>{{cur}}&deg;C</td>
            </tr>
            <tr>
                <td>Highest temperature&nbsp;{{temp_high_time}}&nbsp;seconds ago</td>
                <td>{{high}}&deg;C</td>
            </tr>
            <tr>
                <td>Lowest temperature&nbsp;{{temp_low_time}}&nbsp;seconds ago</td>
                <td>{{low}}&deg;C</td>
            </tr>
        </table>
		<h3>MQTT Topics</h3>
		<table class="mui-table mui-table--bordered">
			<tr>
				<td>Topic Temperature</td>
				<td><input type="text" name="ttopic" size="50" value="{{ttopic}}"></td>
			</tr>
		</table>
        <h3>History</h3>
        <table class="mui-table mui-table--bordered">
            {{#history}}
            <tr>
                <td>{{date}}</td>
                <td>{{val}}&deg;C</td>
            </tr>
            {{/history}}
        </table>
    <input type="button" id="resetTemp" value="Reset" onclick="resetTempHist()">
`;
templates['expand2'] = `
        <h3>Water Meter</h3>
        <table class="mui-table mui-table--bordered">
            {{#watermeter}}
            <tr>
                <td>Water Meter {{name}}</td>
                <td>{{quantity}}&nbsp;Liter</td>
                <td>
                    <select name="pin{{index}}">
                        {{#options}}
                        <option value="{{val}}" {{#sel}}selected{{/sel}}>{{name}}</option>
                        {{/options}}
                    </select>
                </td>
            </tr>
            {{/watermeter}}
            <tr>
                <td>Multiplicator</td>
                <td></td>
                <td><input type="number" name="multi" min="0" step="10" value="{{multi}}">&nbsp;ml</td>
            </tr>
        </table>
        <h3>Input Register</h3>
        <table class="mui-table mui-table--bordered">
            <tr>
                <td>Value</td>
                <td>{{input}}</td>
            </tr>
            <tr>
                <td>Bits</td>
                <td class="bits">
                    {{#inputs}}
                    <label><input type="checkbox" disabled name="{{name}}" {{#val}}checked{{/val}}>{{number}}</label>
                    {{/inputs}}
                </td>
            </tr>
        </table>
        <h3>Output Register</h3>
        <table class="mui-table mui-table--bordered">
            <tr>
                <td>Value</td>
                <td><input type="number" name="output" min="0" max="255" step="1" value="{{output}}"></td>
            </tr>
            <tr>
                <td>Bits</td>
                <td class="bits">
                    {{#outputs}}
                    <label><input type="checkbox" name="output[]" onchange="toggleBit(this, event)" {{#val}}checked{{/val}}>{{number}}</label>
                    {{/outputs}}
                </td>
            </tr>
        </table>
		<h3>MQTT Topics</h3>
		<table class="mui-table mui-table--bordered">
			<tr>
				<td>Topic Water-Meter 1</td>
				<td><input type="text" name="etopic1" size="50" value="{{etopic1}}"></td>
			</tr>
			<tr>
				<td>Topic Water-Meter 2</td>
				<td><input type="text" name="etopic2" size="50" value="{{etopic2}}"></td>
			</tr>
		</table>
`;
templates['mqtt'] = `
		<h3>MQTT Client Information</h3>
		<table class="mui-table mui-table--bordered">
			<tr>
				<td>Status</td>
				<td>{{mqttOnline}}</td>
			</tr>
			<tr>
				<td>Uptime</td>
				<td>{{mqttUptime}}</td>
			</tr>
			<tr>
				<td>Published Messages</td>
				<td>{{mqttPubMsg}}</td>
			</tr>
			<tr>
				<td><input type="button" id="mqttboot" value="{{mqttButton}}" onclick="rebootMqttClient();"></td>
				<td> </td>
			</tr>
		</table>
		
		<h3>Configure Credentials</h3>
		<table class="mui-table mui-table--bordered">
			<tr>
				<td>Broker Address</td>
				<td><input type="text" name="broker" size="40" value="{{bad}}"></td>
			</tr>
			<tr>
				<td>Broker Port</td>
				<td><input type="number" name="port" min="1" max="65535" step="1"  value="{{bpd}}"></td>
			</tr>
			<tr>
				<td>Client ID</td>
				<td><input type="text" name="client" size="40" value="{{cID}}"></td>
			</tr>
			<tr>
				<td>Username</td>
				<td><input type="text" name="user" size="40" value="{{user}}"></td>
			</tr>
			<tr>
				<td>Password</td>
				<td><input type="password" name="password" size="40" value="{{pwd}}"></td>
			</tr>
			<tr>
				<td>Last Will Active</td>
				<td><input type="checkbox" name="will" {{#will}}checked{{/will}}></td>
			</tr>
			<tr>
				<td>Will Topic</td>
				<td><input type="text" name="willtopic" size="40" value="{{wtp}}"></td>
			</tr>
			<tr>
				<td>Will Message</td>
				<td><input type="text" name="willmessage" size="40" value="{{wms}}"></td>
			</tr>
		</table>
`;


var timeout;

function prevent( event ) {
      event.stopPropagation();
}

function rebootMqttClient()
{
	sendRequest('mqtt', 'toggle', $.noop );
}

// Used by the Expand2Click output bits
function toggleBit( element, event ) {
    var x = 0;
    $($('input[name="'+element.name+'"').get().reverse()).each(function(i, v) {
        $(v).prop('checked') && ( x += 2**i );
    });
    // Change the numeric input field, which triggers the request to the GreenPHY module.
    $('input[name="'+element.name.substring(0, element.name.lastIndexOf('['))+'"]').val(x).change();
    // Prevent triggering the request twice.
    event.stopPropagation();
}

function capitalize(string) {
    return string.charAt(0).toUpperCase() + string.slice(1);
}

// thermo3click variables
var tempHistory = [];
// expand2click variables
var wmeterPin = [0,0];
var wmeterMultiplicator = 0.25;

function processJSON(page, json) {
    switch(page) {
        case 'status':
            $('#hostname').text(json['hostname']);
            json['uptime'] += ' s';
            json['free_heap'] += ' B';
            break;
        case 'config':
            $('#nav .clickboard').addClass('hidden');
			if( json['mqttSwitch'] > 0 )
				$('#nav li.clickboard').eq(2).removeClass('hidden');
            $.each(json['clickboards'], function(i, clickboard) {
                clickboard['name_format'] = capitalize(clickboard['name']) + 'Click';
                clickboard['port1_available'] = clickboard['available'] & (1 << 0) ? true : false;
                clickboard['port2_available'] = clickboard['available'] & (1 << 1) ? true : false;
                clickboard['port1_active'] = clickboard['active'] & (1 << 0) ? true : false;
                clickboard['port2_active'] = clickboard['active'] & (1 << 1) ? true : false;

                // If the clickboard is active, add an entry to the menu.
                if(clickboard['port1_active']) {
                    $('#nav li.clickboard').eq(0).removeClass('hidden').find('a').attr('href', '#'+clickboard['name']).find('span').text(clickboard['name_format']);
                }
                if(clickboard['port2_active']) {
                    $('#nav li.clickboard').eq(1).removeClass('hidden').find('a').attr('href', '#'+clickboard['name']).find('span').text(clickboard['name_format']);
                }
            });
            break;
        case 'color2':
            var rNorm = 1.0;
            var gNorm = 2.12;
            var bNorm = 2.0;
            json['r_norm'] = Math.round(json['r'] / rNorm);
            json['g_norm'] = Math.round(json['g'] / gNorm);
            json['b_norm'] = Math.round(json['b'] / bNorm);
            var colorMax = Math.max(json['r_norm'], json['g_norm'], json['b_norm']);
            json['r_dec'] = Math.round(json['r_norm'] * 255 / colorMax);
            json['g_dec'] = Math.round(json['g_norm'] * 255 / colorMax);
            json['b_dec'] = Math.round(json['b_norm'] * 255 / colorMax);
            json['r_hex'] = json['r_dec'].toString(16);
            json['g_hex'] = json['g_dec'].toString(16);
            json['b_hex'] = json['b_dec'].toString(16);
            break;
        case 'thermo3':
            json['cur'] = json['temp_cur'] / 100;
            json['high'] = json['temp_high'] / 100;
            json['low'] = json['temp_low'] / 100;
            // Store temp value in the global history
            tempHistory.unshift({ 'date' : new Date().toLocaleString('de-DE'), 'val' : json['cur'] });
            json['history'] = tempHistory;
            break;
        case 'expand2':
            json['inputs'] = [];
            json['outputs'] = [];
            for( var i = 0; i < 8; i++ ) {
                json['inputs'].unshift({'number': i, 'val': (json['input'] & ( 1 << i )) });
                json['outputs'].unshift({'number':i, 'val': (json['output'] & ( 1 << i )) });
            }
            // Convert toggle count to water quantity
            json['watermeter'] = [];
            for( var x = 0; x < 2; x++ ) {
                var options = [{ 'val': 0, 'sel': (!json['pin'+x] ? 1 : 0), 'name': 'Off' }];
                for( var i = 0; i < 8; i++) {
                    options.push({ 'val': Math.pow(2, i) , 'sel': (json['pin'+x] & ( 1 << i )), 'name': 'PA'+i});
                }
                json['watermeter'].push({
                    'index': x,
                    'name': x+1,
                    'quantity': (json['count'+x] * json['multi'] / 1000).toFixed(3),
                    'options': options
                });
            }
			break;
		case 'mqtt':
			if( json['mqttUptime'] > 0 ) {
				json['mqttOnline'] = 'Online';
				json['mqttButton'] = 'Disconnect';
			}
			else {
				json['mqttOnline'] = 'Offline';
				json['mqttButton'] = 'Connect';
			}
			break;
        default:
            break;
    }
    return json;
}

function sendRequest(page, data, success) {
    // Set domain for testing purposes to GreenPHY module URL like 'http://172.16.201.3/'
    var domain = '';
    if( !data ) data = { action: 'get' };
    $.getJSON(domain + page + '.json', data, success)
            .fail(function(xhr, text_status, error_thrown) {
                    console.log(text_status);
                    console.log(error_thrown);
            });
}

// Store the currently visible page
var currentPage;
function renderPage(page, json) {
    var html = Mustache.render(templates[page], processJSON(page, json));

    if( currentPage == page ) {
        $('#content').html(html);
    } else {
        currentPage = page;
        $('#content').html(html);
        // Mark navigation link as active
        $('#nav a.active').removeClass('active');
        $('#nav a[href^="#' + page + '"]').addClass('active');

        // Change title
        $('#page-title').html($('#nav a.active').html());
    }
}

function setRefreshRate( rate ) {
    $('input[name="refresh"]').val(rate).trigger('input');
}

function getRefreshRate() {
    var rates = [0,0.5, 1, 3, 5, 10, 20, 30, 60];
    return rates[$('input[name="refresh"]').val()];
}

function updatePage(page, data) {
    if( timeout ) clearTimeout(timeout);

    if( !page ) {
      page = window.location.hash.substr(1);
    } else {
        window.location.hash = '#' + page;
    }
    if( !(page in templates) ) {
        page = 'status';
    }

    sendRequest(page, data, function(json) {
        renderPage(page, json);
		if( timeout ) clearTimeout(timeout);
        if(getRefreshRate() != 0)
            timeout = setTimeout(updatePage, getRefreshRate()*1000);
    });
}

// Handle internal links by JQuery
$(document).on('click', 'a[href^="#"]', function(event) {
    event.preventDefault();
    updatePage(this.hash.substr(1));
});

function serialize(element) {
    var data = $(element).serialize();
    if( $(element).is(':checkbox') && !element.checked ) {
        data += element.name + '=off';
    }
	if( $(element).is(':text') ) {
        data = $(element).prop('name') + '=' + $(element).prop('value');
    }
    return data;
}

// Submit input fields on change
$(document).on('change', 'input, select', function() {
    updatePage(undefined, serialize(this));
});

// Stop auto refresh when focusing input fields
$(document).on('focus', 'input, select', function() {
    if( timeout ) clearTimeout(timeout);
});

// Refresh page when leaving input fields
$(document).on('focusout', 'input, select', function() {
    updatePage(); 
});

// Load config once to add current clickboards to menu
sendRequest('config', undefined, function(json) {
    processJSON('config', json);
});

// Initialize current page
updatePage();

// Button to open/close top right menu
$("#menu button").click(function(e) {
    $("#menu ul").toggle();
    e.stopPropagation();
});

// Close menu when clicked outside
$(document).click(function(){
  $("#menu ul").hide();
});

// Send request to write config to flash
$('#write-config').click(function() {
    sendRequest('config', 'write', $.noop );
});

// Send request to erase config from flash
$('#erase-config').click(function() {
    sendRequest('config', 'erase', $.noop );
});

// Send request to reset system
$('#reset-system').click(function() {
    $.getJSON('status.json', 'reset');
});
