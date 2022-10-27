#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>

const char* ssid = "*"; // masked for privacy
const char* password = "*";

//const char* ssid = "*"; 
//const char* password = "*";

ESP8266WebServer server(80); //Server on port 80
WiFiUDP udp;

const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html><html lang="en-US"><head>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
</head>
<script>
var firemode = true;
function torgb(hexmatch) {
	hexmatch = hexmatch.replace("#", "");
	var rgbhex = hexmatch.match(/.{1,2}/g);  
	var rgbhex = [parseInt(rgbhex[0], 16), parseInt(rgbhex[1], 16), parseInt(rgbhex[2], 16)];
	return rgbhex;
}
function sendData() {
	var rgbhex = torgb(colorpicker.value);
	var fullstring = 'l=' + lantern.checked;
	fullstring += "&s=" + strip.checked;
	fullstring += "&a=" + intense.value;
	fullstring += "&r=" + rgbhex[0];
	fullstring += "&g=" + rgbhex[1];
	fullstring += "&b=" + rgbhex[2];
	fullstring += "&f=" + flipflop.value;
	fullstring += "&d=" + delay.value;
	location.href = '/get?'+fullstring;
}
</script>
<body style="font-family: monospace;">
<input id="colorpicker" type="color" style="width:80px;" value="#e27923"><br />
<input type="button" style="width:80px;background:#e27923" onclick="colorpicker.value='#e27923';">
<input type="button" style="width:80px;background:#9e0894" onclick="colorpicker.value='#9e0894';">
<input type="button" style="width:80px;background:#4a960c" onclick="colorpicker.value='#4a960c';"><br />
<input type="button" style="width:80px;background:#ff0000" onclick="colorpicker.value='#ff0000';">
<input type="button" style="width:80px;background:#00ff00" onclick="colorpicker.value='#00ff00';">
<input type="button" style="width:80px;background:#0000ff" onclick="colorpicker.value='#0000ff';"><br />
<input type="button" style="width:80px;background:#ff00ff" onclick="colorpicker.value='#ff00ff';">
<input type="button" style="width:80px;background:#ffff00" onclick="colorpicker.value='#ffff00';">
<input type="button" style="width:80px;background:#00ffff" onclick="colorpicker.value='#00ffff';"><br />
<input type="checkbox" id="lantern" checked>Lantern<br />
<input type="checkbox" id="strip">Ledstrip<br />
Vuur aan/uit: <input id="flipflop" type="button" value="1" onclick="firemode=-firemode;flipflop.value=firemode"><br />

<p style="white-space: pre; margin: 0px;">1%            Brightness          100%</p>
<input id="intense" style="width:200px;" type="range" min="10" max="100" value="100" step="1"><br />
<p style="white-space: pre; margin: 0px;">instant            Delay              5s</p>
<input id="delay" style="width:200px;" type="range" min="100" max="10000" value="5000" step="100"><br /><br />
<input type="button" value="Send" onclick="sendData();"><hr>
<input type="button" value="Thunder - Distant" onclick="location.href = '/thunder?type=DISTANT'">
<input type="button" value="Thunder - Close" onclick="location.href = '/thunder?type=CLOSE'"><br />
<input type="button" style="width:50px;background:#ffffff" onclick="location.href = '/thunder?color=W'">
<input type="button" style="width:50px;background:#ff0000" onclick="location.href = '/thunder?color=R'">
<input type="button" style="width:50px;background:#00ff00" onclick="location.href = '/thunder?color=G'">
<input type="button" style="width:50px;background:#0000ff" onclick="location.href = '/thunder?color=B'">
<p style="white-space: pre; margin: 0px;">Off 5s       Auto Delay            5m</p>
<input id="delayThunder" style="width:200px;" type="range" min="1" max="300" value="300" step="1"><br /><br />
<input type="button" value="Send" onclick="location.href = '/thunder?delay=' + delayThunder.value">
</body></html>
)=====";


void setup(void){
	
	pinMode(LED_BUILTIN, OUTPUT);
	Serial.begin(9600);

	WiFi.begin(ssid, password);
	Serial.println("");

	int errCounter = 0;
	while (WiFi.status() != WL_CONNECTED) {
		digitalWrite(LED_BUILTIN, LOW);   
		delay(250);
		digitalWrite(LED_BUILTIN, HIGH);   
		delay(250);
		Serial.print(".");
		errCounter++;
		if (errCounter > 50) { break; }
	}

	if (errCounter > 50) { // Not connected, send warning
		while (true) { // lock up forever
			digitalWrite(LED_BUILTIN, LOW);   
			delay(1000);                      
			digitalWrite(LED_BUILTIN, HIGH);  
			delay(2000);      
		}
	}
	
	Serial.println("");
	Serial.print("Success! IP address: ");
	Serial.println(WiFi.localIP());  //IP address assigned to your ESP

	server.on("/", handleRoot);      //Which routine to handle at root location
	server.on("/get", handleForm); //form action is handled here
	server.on("/thunder", handleThunder); //form action is handled here
	server.begin();                  //Start server
	Serial.println("HTTP server started");
	
	String ipSection = "https://www.draan.net/ip.php?ip=" + WiFi.localIP().toString();	
	
	WiFiClientSecure client;
	HTTPClient http;
	const char* host = "www.draan.net";
	
	client.setInsecure(); //the magic line, use with caution
	client.connect(host, 443); // prep client
	http.begin(client, ipSection); // prep http
	http.GET(); // push
}

void loop(void){
	server.handleClient();   
}

void handleRoot() {
	String s = MAIN_page; //Read HTML contents
	server.send(200, "text/html", s); //Send web page
}

void handleForm() {
	String alpha = server.arg("a"); 
	String red = server.arg("r"); 
	String green = server.arg("g"); 
	String blue = server.arg("b"); 
	String flicker = server.arg("f"); 
	String delay = server.arg("d"); 

	if (server.arg("l") == "true" ) {
		String fullstring = "SET,"+red+","+green+","+blue+","+alpha+","+delay+","+flicker;
		Serial.println(fullstring);
		udpSend(fullstring);
	}

	if (server.arg("s") == "true" ) {
		String fullstring = "STRIPSET,"+red+","+green+","+blue+","+alpha+","+delay+","+flicker;
		Serial.println(fullstring);
		udpSend(fullstring);
	}
	server.send(200, "text/html", "<html><body><script>location.href = '/'</script></body></html>"); //Return to index
}

void handleThunder() {
	if ( server.arg("color").length() > 0 ) { 
		udpSend( "FCOLOR," + server.arg("color") );
		Serial.println( "FCOLOR," + server.arg("color") );
	}
	if (server.arg("type").length() > 0) { 
		udpSend( "FLASH," + server.arg("type") );
		Serial.println("FLASH," + server.arg("type"));
	}
	if (server.arg("delay").length() > 0) { 
		udpSend( "FDELAY," + server.arg("delay") );
		Serial.println( "FDELAY," + server.arg("delay") );
	}
	server.send(200, "text/html", "<html><body><script>location.href = '/'</script></body></html>"); //Return to index
}

void udpSend(String message) {	
	IPAddress broadcastIP( {WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], 255} );
	udp.beginPacket( broadcastIP, 4200 ); 	
	udp.write(message.c_str());
	udp.endPacket();
}