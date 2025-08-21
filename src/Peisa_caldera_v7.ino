#include <WiFiManager.h> // Para la configuración WiFi
#include <PicoMQTT.h>   // Para el servidor MQTT

// --- Configuración de Pines ---
const int RELAY_PIN = 0;
const int STATUS_LED_PIN = 2;

// --- Variables de Estado del Sensor ---
int sensorPB_state = -1;
int sensorPA_state = -1;
unsigned long lastUpdatePB = 0;
unsigned long lastUpdatePA = 0;
const unsigned long MAX_INACTIVITY_TIME = 2 * 60 * 1000;

// --- Objeto del Servidor MQTT ---
PicoMQTT::Server mqtt;

// --- Parámetros Personalizados de WiFiManager ---
char topic_pa_str[65];
char topic_pb_str[65];
char topic_status_str[65];

const char* DEFAULT_TOPIC_PA = "/monet/peisa/PA";
const char* DEFAULT_TOPIC_PB = "/monet/peisa/PB";
const char* DEFAULT_TOPIC_STATUS = "/monet/peisa/status";

WiFiManagerParameter custom_topic_pa("topicpa", "Topic PA", "", 64);
WiFiManagerParameter custom_topic_pb("topicpb", "Topic PB", "", 64);
WiFiManagerParameter custom_topic_status("topicstatus", "Topic Status", "", 64);

// --- Temporización ---
unsigned long lastRelayControlTime = 0;
const unsigned long RELAY_CHECK_INTERVAL = 5000;

unsigned long lastLedUpdateTime = 0;
const unsigned long LED_UPDATE_INTERVAL = 1000;

// --- Lógica de retardo antes de activar el relé ---
unsigned long relayActivationStartTime = 0;
bool waitingToActivateRelay = false;
const unsigned long RELAY_DELAY_MS = 5 * 60 * 1000;

int lastPublishedRelayState = -1;

// --- Funciones de Callback ---
void onMqttMessageReceived(const char * topic, const char * payload) {
    Serial.printf("Mensaje recibido por el SERVIDOR [ %s ]: %s\n", topic, payload);
    String message = payload;
    int state = message.toInt();

    if (String(topic) == String(topic_pb_str)) {
        sensorPB_state = state;
        lastUpdatePB = millis();
    } else if (String(topic) == String(topic_pa_str)) {
        sensorPA_state = state;
        lastUpdatePA = millis();
    }
}

void saveConfigCallback() {
  strcpy(topic_pa_str, custom_topic_pa.getValue());
  strcpy(topic_pb_str, custom_topic_pb.getValue());
  strcpy(topic_status_str, custom_topic_status.getValue());
}

void publishRelayState(int state) {
  if (state != lastPublishedRelayState) {
    String payload = String(state);
    mqtt.publish(topic_status_str, payload.c_str(), true);
    lastPublishedRelayState = state;
  }
}

void controlRelay() {
  unsigned long currentTime = millis();

  bool pb_recent = (currentTime - lastUpdatePB < MAX_INACTIVITY_TIME);
  bool pa_recent = (currentTime - lastUpdatePA < MAX_INACTIVITY_TIME);

  bool conditionMet = 
    (sensorPB_state == 1 && pb_recent) || 
    (sensorPA_state == 1 && pa_recent);

  if (conditionMet) {
    if (!waitingToActivateRelay) {
      relayActivationStartTime = currentTime;
      waitingToActivateRelay = true;
      Serial.println("Condiciones cumplidas, iniciando temporizador de 5 minutos...");
    } else if (currentTime - relayActivationStartTime >= RELAY_DELAY_MS) {
      digitalWrite(RELAY_PIN, LOW);
      publishRelayState(1);
      Serial.println(">>> Relé ACTIVADO tras 5 minutos de condiciones cumplidas <<<");
    } else {
      Serial.println("Esperando que se cumplan los 5 minutos para activar el relé...");
    }
  } else {
    if (waitingToActivateRelay) {
      Serial.println("Condiciones dejaron de cumplirse antes de los 5 minutos. Cancelando.");
    }
    waitingToActivateRelay = false;
    digitalWrite(RELAY_PIN, HIGH);
    publishRelayState(0);
    Serial.println(">>> Relé DESACTIVADO <<<");
  }
}

void updateStatusLed() {
  unsigned long currentTime = millis();
  bool wifiConnected = (WiFi.status() == WL_CONNECTED);
  bool pb_recent = (currentTime - lastUpdatePB < MAX_INACTIVITY_TIME);
  bool pa_recent = (currentTime - lastUpdatePA < MAX_INACTIVITY_TIME);

  digitalWrite(STATUS_LED_PIN, (wifiConnected && pb_recent && pa_recent) ? HIGH : LOW);
}

void setup() {
  Serial.begin(9600);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);

  WiFiManager wifiManager;
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.setConnectTimeout(120);
  wifiManager.setConfigPortalTimeout(600);

  custom_topic_pa.setValue(DEFAULT_TOPIC_PA, 64);
  custom_topic_pb.setValue(DEFAULT_TOPIC_PB, 64);
  custom_topic_status.setValue(DEFAULT_TOPIC_STATUS, 64);

  wifiManager.addParameter(&custom_topic_pa);
  wifiManager.addParameter(&custom_topic_pb);
  wifiManager.addParameter(&custom_topic_status);

  if (!wifiManager.autoConnect("ESP01_Setup_AP", "password123")) {
    delay(3000);
    ESP.restart();
  }

  strcpy(topic_pa_str, custom_topic_pa.getValue());
  strcpy(topic_pb_str, custom_topic_pb.getValue());
  strcpy(topic_status_str, custom_topic_status.getValue());

  mqtt.subscribe("#", onMqttMessageReceived);
  mqtt.begin();
}

void loop() {
  mqtt.loop();

  unsigned long currentMillis = millis();
  if (currentMillis - lastRelayControlTime >= RELAY_CHECK_INTERVAL) {
    lastRelayControlTime = currentMillis;
    controlRelay();
  }

  if (currentMillis - lastLedUpdateTime >= LED_UPDATE_INTERVAL) {
    lastLedUpdateTime = currentMillis;
    updateStatusLed();
  }
}
