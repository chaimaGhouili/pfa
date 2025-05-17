#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

// WiFi Configuration
//const char *ssid = "TOPNET_CE68";
//const char *password = "fb36b8i211";
const char *ssid = "DESKTOP-U9CN6IK 9510"; 
const char *password = "nerimenaf";
//const char* ssid = "HUAWEI-2.4G-EnC2";
//const char* password = "140220192";
//const char* ssid = "ETUDIANT";
//const char* password = "ISIMGwifi";
// Adafruit IO Configuration
#define AIO_SERVER "io.adafruit.com"
#define AIO_SERVERPORT 8883  // Pour SSL sécurisé (TLS)
#define AIO_USERNAME "chaimaghouili"
#define AIO_KEY "aio_EKvr41GD4TrcIBs56LLzQat4n9Zw"

// Objets WiFi et MQTT
WiFiClientSecure client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// Ton feed Adafruit IO
Adafruit_MQTT_Publish testFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/test");
Adafruit_MQTT_Publish temperatureFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/temperature");
Adafruit_MQTT_Publish humidityFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/humidity");
Adafruit_MQTT_Publish lumiereFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/lumiere");
Adafruit_MQTT_Publish solFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/sol_hymidity");
Adafruit_MQTT_Publish errorFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/errors");
Adafruit_MQTT_Subscribe timeFeed = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/time");

void mqttMessage(char *data, uint16_t len) {
  Serial.print("Message reçu: ");
  Serial.write(data, len);
  Serial.println();
}
void reconnect() {

  int8_t ret;

  // Boucle de reconnexion
  while ((ret = mqtt.connect()) != 0) {
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Nouvelle tentative dans 5 secondes...");
    mqtt.disconnect();
    delay(1000);
  }
  Serial.println("Reconnecté à Adafruit IO !");
  mqtt.subscribe(&timeFeed);
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, 16, 17);  // UART pour capteur

  // Connexion WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connexion WiFi...");
  }
  Serial.println("WiFi Connecté");

  client.setInsecure();  // Ignorer la vérification SSL (à sécuriser pour production)
}
bool validateDateFormat(String date) {
  // Validation simple du format jj/mm/aaaa
  if (date.length() != 10) return false;
  if (date[2] != '/' || date[5] != '/') return false;
  
  for (int i = 0; i < 10; i++) {
    if (i == 2 || i == 5) continue;
    if (!isdigit(date[i])) return false;
  }
  
  return true;
}
void loop() {
  // Vérifier la connexion MQTT
  if (!mqtt.connected()) {
    Serial.println("MQTT déconnecté !reconnecter");
    reconnect();  // Appel à reconnect()
  }
  mqtt.processPackets(10000);
  mqtt.ping();

  if (Serial2.available()) {
    String data = Serial2.readStringUntil('\n');
    data.trim();

    Serial.println("Donnée reçue : " + data);

    // Publication brute
    if (!testFeed.publish(data.c_str())) {
      Serial.println("Erreur publication sur feed test !");
    }

    // Séparer les données (4 valeurs attendues)
    int index1 = data.indexOf(',');
    int index2 = data.indexOf(',', index1 + 1);
    int index3 = data.indexOf(',', index2 + 1);


    String tempStr = data.substring(0, index1);
    String humStr = data.substring(index1 + 1, index2);
    String solStr = data.substring(index2 + 1, index3);
    String lumiereStr = data.substring(index3 + 1);

    // Nettoyage
    tempStr.trim();
    humStr.trim();
    solStr.trim();
    lumiereStr.trim();

    // Publication des données individuelles
    if (!temperatureFeed.publish(tempStr.c_str()))
      Serial.println("Erreur publication température !");
    else
      Serial.println("Température publiée !");

    if (!humidityFeed.publish(humStr.c_str()))
      Serial.println("Erreur publication humidité !");
    else
      Serial.println("Humidité publiée !");

    if (!solFeed.publish(solStr.c_str()))
      Serial.println("Erreur publication humidité du sol !");
    else
      Serial.println("Humidité du sol publiée !");

    if (!lumiereFeed.publish(lumiereStr.c_str()))
      Serial.println("Erreur publication lumière !");
    else
      Serial.println("Lumière publiée !");
  }
  Adafruit_MQTT_Subscribe *subscription;
 
  while ((subscription = mqtt.readSubscription(1000))) {
    Serial.println("MQTT message reçu");
    if (subscription == &timeFeed) {
      String msg = (char *)timeFeed.lastread;
      Serial.print("Message reçu sur le feed 'time' : ");
      Serial.println(msg);

      if (!validateDateFormat(msg)) {
        Serial.println("Format invalide !");
        errorFeed.publish("Erreur : format invalide, utilisez jj/mm/aaaa");
      } else {
        Serial.println("Format valide heure!");
        errorFeed.publish("validee");
        // Traitement supplémentaire ici
      }
    }
  }
  delay(1000);  // Attente pour respecter les lim
}