#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <dht.h>

const char* ssid = "SSID";
const char* password = "PASS";
const char* mqtt_server = "localhost";
const char* mqtt_user = "esp8266";
const char* mqtt_password = "PASS";
const char* clientId = "espA";
#define DHT22_PIN 4
#define LED_R 16
#define LED_Y 0
//#define LED_G
//#define BUZZER
#define IR 5

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[75];
int value = 0;
dht DHT;
int chk;
int irstate;

char m_heartbeat[50];
char m_sensor[50];
char m_temp[50];
char m_rh[50];
char m_ledr[50];

char *ftoa(char *a, double f, int precision)
{
 long p[] = {0,10,100,1000,10000,100000,1000000,10000000,100000000};
 
 char *ret = a;
 long heiltal = (long)f;
 itoa(heiltal, a, 10);
 while (*a != '\0') a++;
 *a++ = '.';
 long desimal = abs((long)((f - heiltal) * p[precision]));
 itoa(desimal, a, 10);
 return ret;
}


void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  for (int i = 1; i <= 3; i++){
    digitalWrite(LED_Y, HIGH);
    delay(1);
    digitalWrite(LED_Y, LOW);
    delay(1);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if ( strcmp(topic,m_ledr) == 0 ){
     Serial.println("m_ledr detected");
    if ((char)payload[0] == '1'){
      digitalWrite(LED_R, LOW);
      Serial.println("LED RED ON");
    } else {
      if ((char)payload[0] == '0'){
        digitalWrite(LED_R, HIGH);
        Serial.println("LED RED OFF");
      }
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(clientId, mqtt_user, mqtt_password)) {
      Serial.println("connected");
      // ... and resubscribe
      client.subscribe(m_ledr);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_Y, OUTPUT);
  //pinMode(LED_G, OUTPUT);
  //pinMode(BUZZER, OUTPUT);
  pinMode(IR, INPUT_PULLUP);
  //pinMode(IR, INPUT);

  digitalWrite(LED_R, HIGH);
  irstate = 0;
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  strcat(m_heartbeat, "/heartbeat/");
  strcat(m_heartbeat, clientId);
  
  strcat(m_sensor, "/sensor/");
  strcat(m_sensor, clientId);
  
  strcat(m_temp, "/temp/");
  strcat(m_temp, clientId);
  
  strcat(m_rh, "/rh/");
  strcat(m_rh, clientId);
  
  strcat(m_ledr, "/out/");
  strcat(m_ledr, clientId);
  strcat(m_ledr, "/ledr");
    
}

void loop() {
  char temp[6], rh[6];
  int irread;
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  irread = digitalRead(IR);
  if (irread == HIGH && irstate == 0){
    snprintf(msg, 75, "IR ALARM");
    Serial.print(m_sensor);
    Serial.print(" : ");
    Serial.println(msg);
    client.publish(m_sensor, msg);
    //digitalWrite(LED_Y, HIGH);
    irstate = 1;
  } else {
    if (irread == LOW && irstate == 1){
      snprintf(msg, 75, "IR CLEAR");
      Serial.print(m_sensor);
      Serial.print(" : ");
      Serial.println(msg);
      client.publish(m_sensor, msg);
      //digitalWrite(LED_Y, LOW);
      irstate = 0;
    }
  }

  // loop of stuff to do every 5 sec
  long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;
    ++value;
    snprintf (msg, 75, "%s %ld",clientId, value);
    Serial.print(m_heartbeat);
    Serial.print(" : ");
    Serial.println(msg);
    client.publish(m_heartbeat, msg);
    
    chk = DHT.read22(DHT22_PIN);
    switch (chk)
      {
        case DHTLIB_OK:
          dtostrf(DHT.temperature, 4, 2, temp);
          dtostrf(DHT.humidity, 4, 2, rh);
          snprintf(msg, 75, "%s", temp);
          Serial.print(m_temp);
          Serial.print(" : ");
          Serial.println(msg);
          client.publish(m_temp, msg);
          snprintf(msg, 75, "%s", rh);
          Serial.print(m_rh);
          Serial.print(" : ");
          Serial.println(msg);
          client.publish(m_rh, msg);
          break;

        case DHTLIB_ERROR_CHECKSUM:
          Serial.print("Checksum error,\t");
          break;
        case DHTLIB_ERROR_TIMEOUT:
          Serial.print("Time out error,\t");
          break;

       default:
          break;
      }
   }
}
