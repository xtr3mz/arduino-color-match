//https://randomnerdtutorials.com/esp8266-web-server-with-arduino-ide/
//https://github.com/adafruit/Adafruit_TCS34725

#include "ESP8266WiFi.h"
#include "Adafruit_TCS34725.h"
/*
ESP8266
3V
G
D1:SCL
D2:SDL
*/
#define TCS34725_R_Coef 0.136 
#define TCS34725_G_Coef 1.000
#define TCS34725_B_Coef -0.444
#define TCS34725_GA 1.0
#define TCS34725_DF 310.0
#define TCS34725_CT_Coef 3810.0
#define TCS34725_CT_Offset 1391.0


// Autorange class for TCS34725
class tcs34725 {
public:
  	tcs34725(void);

  	boolean begin(void);
  	void getData(void);  
  	void toRGB(void);
  
  	boolean isAvailable, isSaturated;
  	uint16_t againx, atime, atime_ms;
  	uint16_t r, g, b, c;
  	uint16_t ir; 
  	uint16_t r_comp, g_comp, b_comp, c_comp;
  	uint16_t saturation, saturation75;
  	float cratio, cpl, ct, lux, maxlux;
	String rgb="";
  
private:
  struct tcs_agc {
    tcs34725Gain_t ag;
    tcs34725IntegrationTime_t at;
    uint16_t mincnt;
    uint16_t maxcnt;
  };
  static const tcs_agc agc_lst[];
  uint16_t agc_cur;

  void setGainTime(void);  
  Adafruit_TCS34725 tcs;    
};
//
// Gain/time combinations to use and the min/max limits for hysteresis 
// that avoid saturation. They should be in order from dim to bright. 
//
// Also set the first min count and the last max count to 0 to indicate 
// the start and end of the list. 
//
const tcs34725::tcs_agc tcs34725::agc_lst[] = {
  { TCS34725_GAIN_60X, TCS34725_INTEGRATIONTIME_700MS,     0, 20000 },
  { TCS34725_GAIN_60X, TCS34725_INTEGRATIONTIME_154MS,  4990, 63000 },
  { TCS34725_GAIN_16X, TCS34725_INTEGRATIONTIME_154MS, 16790, 63000 },
  { TCS34725_GAIN_4X,  TCS34725_INTEGRATIONTIME_154MS, 15740, 63000 },
  { TCS34725_GAIN_1X,  TCS34725_INTEGRATIONTIME_154MS, 15740, 0 }
};
tcs34725::tcs34725() : agc_cur(0), isAvailable(0), isSaturated(0) {
}

// initialize the sensor
boolean tcs34725::begin(void) {
  tcs = Adafruit_TCS34725(agc_lst[agc_cur].at, agc_lst[agc_cur].ag);
  if ((isAvailable = tcs.begin())) 
    setGainTime();
  return(isAvailable);
}

// Set the gain and integration time
void tcs34725::setGainTime(void) {
  tcs.setGain(agc_lst[agc_cur].ag);
  tcs.setIntegrationTime(agc_lst[agc_cur].at);
  atime = int(agc_lst[agc_cur].at);
  atime_ms = ((256 - atime) * 2.4);  
  switch(agc_lst[agc_cur].ag) {
  case TCS34725_GAIN_1X: 
    againx = 1; 
    break;
  case TCS34725_GAIN_4X: 
    againx = 4; 
    break;
  case TCS34725_GAIN_16X: 
    againx = 16; 
    break;
  case TCS34725_GAIN_60X: 
    againx = 60; 
    break;
  }        
}
// Retrieve data from the sensor and do the calculations
void tcs34725::getData(void) {
  // read the sensor and autorange if necessary
  tcs.getRawData(&r, &g, &b, &c);
  while(1) {
    if (agc_lst[agc_cur].maxcnt && c > agc_lst[agc_cur].maxcnt) 
      agc_cur++;
    else if (agc_lst[agc_cur].mincnt && c < agc_lst[agc_cur].mincnt)
      agc_cur--;
    else break;

    setGainTime(); 
    delay((256 - atime) * 2.4 * 2); // shock absorber
    tcs.getRawData(&r, &g, &b, &c);
    break;    
  }

  // DN40 calculations
  ir = (r + g + b > c) ? (r + g + b - c) / 2 : 0;
  r_comp = r - ir;
  g_comp = g - ir;
  b_comp = b - ir;
  c_comp = c - ir;   
  cratio = float(ir) / float(c);
	//65k颜色 / 255颜色 = 256
  saturation = ((256 - atime) > 63) ? 65535 : 1024 * (256 - atime);
  saturation75 = (atime_ms < 150) ? (saturation - saturation / 4) : saturation;
  isSaturated = (atime_ms < 150 && c > saturation75) ? 1 : 0;
  cpl = (atime_ms * againx) / (TCS34725_GA * TCS34725_DF); 
  maxlux = 65535 / (cpl * 3);

  lux = (TCS34725_R_Coef * float(r_comp) + TCS34725_G_Coef * float(g_comp) + TCS34725_B_Coef * float(b_comp)) / cpl;
  ct = TCS34725_CT_Coef * float(b_comp) / float(r_comp) + TCS34725_CT_Offset;

  toRGB();
}
void tcs34725::toRGB(void){
  	float r1,g1,b1;
  	int r2,g2,b2;
  
  	// Avoid divide by zero errors ... if clear = 0 return black
  	if (c == 0) {
		rgb[0]=0;
		rgb[1]=0;
		rgb[2]=0;
    	return;
  	}
	//0-65535 -> val/256=255
  	r1=(float)r / c * 255.0;
  	g1=(float)g / c * 255.0;
  	b1=(float)b / c * 255.0;

  	r2=(int) r1*2.5;
  	g2=(int) g1*2.5;
  	b2=(int) b1*2.5;
  	
  	r2=r2>255?255:r2;
  	g2=g2>255?255:g2;
  	b2=b2>255?255:b2;
  	
	rgb=String(r2)+","+String(g2)+","+String(b2);
}

String RGB="0,0,0";

tcs34725 rgb_sensor;



/* wifi */
const char *ssid = "Testing AP";
const char *password = "12345678";
WiFiServer server(80);
String header;

void setup(){
	Serial.begin(115200);
	Serial.println();
 
	Serial.print("Setting soft-AP ... ");
   
	IPAddress softLocal(192,168,99,1);
	IPAddress softGateway(192,168,99,1);
	IPAddress softSubnet(255,255,255,0);
	 
	WiFi.softAPConfig(softLocal, softGateway, softSubnet);

	WiFi.softAP(ssid, password);

	IPAddress myIP = WiFi.softAPIP();
	Serial.print("AP IP address: ");
	Serial.println(myIP);
	
	server.begin();
  	Serial.println("booting...40s");
  	delay(40000);
	Serial.printf("Web server started, open %s in a web browser\n", WiFi.localIP().toString().c_str());
	
	//rgb
	rgb_sensor.begin();
	pinMode(4, OUTPUT);
	digitalWrite(4, LOW); // @gremlins Bright light, bright light!
	Serial.println("Start color sensor.");

}

void loop(){
	rgb_sensor.getData();
	RGB=rgb_sensor.rgb;

	WiFiClient client = server.available();
	if (client){
	    Serial.println("\n[Client connected]");
	    String currentLine = "";
	    while (client.connected()){
	      // read line by line what the client (web browser) is requesting
	      if (client.available()){
	        char c = client.read();             // read a byte, then
	        Serial.write(c);                    // print it out the serial monitor
	        header += c;
	        if (c == '\n') {                    // if the byte is a newline character
	          // if the current line is blank, you got two newline characters in a row.
	          // that's the end of the client HTTP request, so send a response:
	          if (currentLine.length() == 0) {
		            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
		            // and a content-type so the client knows what's coming, then a blank line:
		            client.println("HTTP/1.1 200 OK");
		            client.println("Content-type:text/html");
		            client.println("Connection: close");
		            client.println();
		            
			        if (header.indexOf("GET /read") >= 0) {
			        	
			          	client.println(RGB);
			          	break;
			        }
			        if (header.indexOf("GET /cali") >= 0) {
			        	//cancelled

			          	break;
			        }
		            client.println(html1());
	            	break;

	    		} 
	    		else { // if you got a newline, then clear currentLine
	            	currentLine = "";
	          	}
	        } 
	        else if (c != '\r') {  // if you got anything else but a carriage return character,
	          currentLine += c;      // add it to the end of the currentLine
	        }
	     }
	}
 
    // close the connection:
    header ="";
    client.stop();
    Serial.println("[Client disonnected]");
  }
}

// prepare a web page to be send to a client (web browser)
String html1(){
  return "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1.0,maximum-scale=1.0,minimum-scale=1.0,user-scalable=no'/><style>body{margin:0;padding:0;font-size:11pt}#h{height:50px;background:#000;color:#fff;line-height:50px;padding:0 10px}#colorbox p{padding:5px;margin:0}#colorbox div{width:50%;float:left;padding:450px 0 5px 0;background:#ddd}fieldset{border:1px solid #000;background:#ddd;margin:0 5px}[type=button]{width:80px;height:30px;width:99%}h3{text-align:center;color:#fff;background:red;font-size:16pt;line-height:50px}</style><script>function ajax(t,url,fnSucc,fnFaild){var oAjax=null;if(window.XMLHttpRequest){oAjax=new XMLHttpRequest();}else{oAjax=new ActiveXObject(\"Microsoft.XMLHTTP\");}oAjax.open('GET',url,true);oAjax.send();oAjax.onreadystatechange=function(){if(oAjax.readyState == 4){if(oAjax.status == 200){fnSucc(t,oAjax.responseText) }else{if(fnFaild){fnFaild();}}}};}function rgb2lab(rgb){var r=rgb[0]/255,g=rgb[1]/255,b=rgb[2]/255,x,y,z;r=(r>0.04045)?Math.pow((r+0.055)/1.055,2.4):r/12.92;g=(g>0.04045)?Math.pow((g+0.055)/1.055,2.4):g/12.92;b=(b>0.04045)?Math.pow((b+0.055)/1.055,2.4):b/12.92;x=(r*0.4124+g*0.3576+b*0.1805)/0.95047;y=(r*0.2126+g*0.7152+b*0.0722)/1.00000;z=(r*0.0193+g*0.1192+b*0.9505)/1.08883;x=(x>0.008856)?Math.pow(x,1/3):(7.787*x)+16/116;y=(y>0.008856)?Math.pow(y,1/3):(7.787*y)+16/116;z=(z>0.008856)?Math.pow(z,1/3):(7.787*z)+16/116;return [(116*y)-16,500*(x-y),200*(y-z)]}function deltaE(labA,labB){var deltaL=labA[0]-labB[0];var deltaA=labA[1]-labB[1];var deltaB=labA[2]-labB[2];var c1=Math.sqrt(labA[1]*labA[1]+labA[2]*labA[2]);var c2=Math.sqrt(labB[1]*labB[1]+labB[2]*labB[2]);var deltaC=c1-c2;var deltaH=deltaA*deltaA+deltaB*deltaB-deltaC*deltaC;deltaH=deltaH<0?0:Math.sqrt(deltaH);var sc=1.0+0.045*c1;var sh=1.0+0.015*c1;var deltaLKlsl=deltaL/(1.0);var deltaCkcsc=deltaC/(sc);var deltaHkhsh=deltaH/(sh);var i=deltaLKlsl*deltaLKlsl+deltaCkcsc*deltaCkcsc+deltaHkhsh*deltaHkhsh;return i<0?0:Math.sqrt(i);}var l0,l1;function read(t,e){if(e.length>-1){var c=e.split(',');if(t==0){$('rgb0').innerHTML=e;$('c0').style.backgroundColor='rgb('+e+')';l0=rgb2lab(c);$('l0').innerText=l0[0].toFixed(0)+','+l0[1].toFixed(0)+','+l0[2].toFixed(0);}else{$('rgb1').innerHTML=e;$('c1').style.backgroundColor='rgb('+e+')';l1=rgb2lab(c);$('l1').innerText=l1[0].toFixed(0)+','+l1[1].toFixed(0)+','+l1[2].toFixed(0);}if(l0 && l1){var de=deltaE(l0,l1);$('de').innerText='DE:'+de.toFixed(2);var dc='red';if(de<=0.5){dc='#00ff00'}else if(de<=1){dc='#ff8040'}$('de').style.backgroundColor=dc;}}}function fail(){alert('failed');}function $(id){return document.getElementById(id);}</script></head><body><div id='h'>ColorMatch v0.1</div><div id='colorbox'><div id='c0'><fieldset>RGB:<b id='rgb0'></b><br />LAB:<b id='l0'></b><br /><input type='button' value='标样' onclick='ajax(0,\"/read\",read,fail)'/></fieldset></div><div id='c1'><fieldset>RGB:<b id='rgb1'></b><br />LAB:<b id='l1'></b><br /><input type='button' value='仿样' onclick='ajax(1,\"/read\",read,fail)'/></fieldset></div></div><h3 id='de'>DE:0</h3><input type='button' onclick='ajax(1,\"/cali\",cali,fail)' value='calibrate(not coded)'/></body></html>";
}
