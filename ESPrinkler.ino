// ***********************************
// ESP8266 auto sprinkler contorller
// ***********************************

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

#ifndef STASSID
#define STASSID "wifissid"
#define STAPSK  "wifipasswd"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;
const char* ICAO = "YBAS";

typedef struct weather {
	String met_data;	// metar string
	bool precip;		// precipitation
	int datamins;		// minutes since last update
	int windspeed;
	//int winddir;
	int visibility;
	int pressure;
	int temp;
	int dew;
	int humidity;

} Weatherdata;

Weatherdata CurrentWeather;

void print_weather(Weatherdata* weather){
	Serial.println(weather -> met_data);
	Serial.print(" Mins since last update:");
	Serial.println(weather -> datamins);
	Serial.print(" Precipitation:");
	Serial.println(weather -> precip);
	Serial.print(" Wind speed [KT]:");
	Serial.println(weather -> windspeed);
	Serial.print(" Visibility [m]:");
	Serial.println(weather -> visibility);
	Serial.print(" Pressure [mBar]:");
	Serial.println(weather -> pressure);
	Serial.print(" Temp [C]:");
	Serial.println(weather -> temp);
	Serial.print(" Dew [C]:");
	Serial.println(weather -> dew);
	Serial.print(" Humidity [%]:");
	Serial.println(weather -> humidity);
}

int extractweather(String rawdata, Weatherdata* newweather){
	#define maxmetarlen 300

	newweather -> precip = false;
	if(rawdata.lastIndexOf(String(ICAO) + " ") < 0){								// keyword not in string
		return -1;
	}

	int starti = rawdata.lastIndexOf((String(ICAO) + " "));
	rawdata = rawdata.substring(starti + strlen(ICAO) + 1,starti + strlen(ICAO) + 1 + maxmetarlen);
	String metardata = rawdata.substring(0,rawdata.lastIndexOf("<"));
	metardata.trim();

	newweather -> met_data = String(ICAO) + " " + metardata;
	String timeupdated = metardata.substring(2,metardata.indexOf(" ") - 1);
	newweather -> datamins = (timeupdated.substring(0,2).toInt() * 60) + timeupdated.substring(2,4).toInt();

	metardata = metardata + " END";
	for(int i = 0;metardata.lastIndexOf(" ") >= 0; i++){
		String newword = metardata.substring(0,metardata.indexOf(" "));
		if(newword.lastIndexOf("KT") >= 0){
			newweather -> windspeed = newword.substring(newword.lastIndexOf("KT") - 3, newword.lastIndexOf("KT")).toInt();
		}else if(i == 2){
			newweather -> visibility = newword.toInt();
		}
		if(newword.lastIndexOf("+") > -1 || newword.lastIndexOf("-") > -1 ){
			newweather -> precip = true;
		}
		if(newword.lastIndexOf("Q") >= 0){
			newweather -> pressure = newword.substring(newword.lastIndexOf("Q")+1, newword.length()).toInt();
		}
		if((newword.length() > 4 && newword.length() < 8) && newword.lastIndexOf("/") >= 0){

			String Temp = newword.substring(0, newword.lastIndexOf("/"));
			int temp;
			if(newword.lastIndexOf("M")>= 0)
				temp = -1 * Temp.substring(Temp.lastIndexOf("M"), Temp.length()).toInt();
			else
				temp = Temp.toInt();

			String Dew = newword.substring(newword.lastIndexOf("/") + 1, newword.length());
			int dew;
			if(newword.lastIndexOf("M")>= 0)
				dew = -1 * Dew.substring(Dew.lastIndexOf("M"), Dew.length()).toInt();
			else
				dew = Dew.toInt();

			newweather -> humidity = ((7.5*dew/(237.7+dew)) / (7.5*temp/(237.7+temp)))*100;		// calc ?relative? humidity

			newweather -> temp = temp;
			newweather -> dew = dew;
		}

		metardata = metardata.substring(metardata.indexOf(" ") + 1,metardata.length());
	}
	return 0;
}
int gethttps(){
	const char* host = "en.allmetsat.com";
	const char* url = "/metar-taf/australia.php?icao=";
	const int httpsPort = 443;

	WiFiClientSecure client;
	client.setInsecure();												// no certificate required, very haxxy

	Serial.println("connecting to host");
	if (!client.connect(host, httpsPort)) {
		Serial.println("connection failed");
		return -1;
	}

	Serial.println("requesting URL");
	client.print(String("GET ") + url + ICAO + " HTTP/1.1\r\n" + "Host: " + host + 
		"\r\n" + "User-Agent: ESP8266\r\n" + "Connection: close\r\n\r\n");
	Serial.println("request sent");

	#define charlmt 500				// not a strict limit, flag for chunk processing
	String chunk;
	String response = "";
	
	client.setTimeout(2000);
	while(client.connected()){
		chunk = client.readStringUntil('>');
      	response += chunk;
		if(response.length() > charlmt)
			Serial.println(response);
			if(extractweather(response, &CurrentWeather) >= 0)
				break;
			response = "";
	}
	return 0;
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.print("connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

}

void loop() {
	gethttps();
	print_weather(&CurrentWeather);
	Serial.println("\n\n\n\n\n");
	delay(10000);
}
