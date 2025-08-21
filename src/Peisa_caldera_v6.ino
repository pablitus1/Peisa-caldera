// #include <WiFiManager.h> // Para la configuración WiFi
// #include <PicoMQTT.h>   // Para el servidor MQTT
// //#include <ESP8266WiFi.h> // Necesario para WiFi.localIP()
// #include <ArduinoOTA.h>

// // --- Configuración de Pines ---
// const int RELAY_PIN = 0; // GPIO0 (D3 en NodeMCU). Asume LOW desactiva, HIGH activa.
// const int STATUS_LED_PIN = 2; // GPIO2 (D4 en NodeMCU). Para el LED de estado.

// // --- Variables de Estado del Sensor ---
// int sensorPB_state = -1; // -1 para estado inicial desconocido
// int sensorPA_state = -1;
// unsigned long lastUpdatePB = 0;
// unsigned long lastUpdatePA = 0;
// const unsigned long MAX_INACTIVITY_TIME = 2 * 60 * 1000; // 2 minutos en milisegundos

// // --- Objeto del Servidor MQTT ---
// PicoMQTT::Server mqtt; // Este será nuestro broker MQTT

// // --- Parámetros Personalizados de WiFiManager ---
// char topic_pa_str[65];
// char topic_pb_str[65];
// char topic_status_str[65];

// // Valores por defecto para los parámetros personalizados
// const char* DEFAULT_TOPIC_PA = "/monet/peisa/PA";
// const char* DEFAULT_TOPIC_PB = "/monet/peisa/PB";
// const char* DEFAULT_TOPIC_STATUS = "/monet/peisa/status";

// // Objetos WiFiManagerParameter
// WiFiManagerParameter custom_topic_pa("topicpa", "Topic PA", "", 64);
// WiFiManagerParameter custom_topic_pb("topicpb", "Topic PB", "", 64);
// WiFiManagerParameter custom_topic_status("topicstatus", "Topic Status", "", 64);

// // --- Temporización no bloqueante para Control del Relé y LED ---
// unsigned long lastRelayControlTime = 0;
// const unsigned long RELAY_CHECK_INTERVAL = 5000; // Revisar relé cada 5 segundos

// unsigned long lastLedUpdateTime = 0;
// const unsigned long LED_UPDATE_INTERVAL = 1000; // Revisar LED cada 1 segundo

// // --- Variables de Estado del Relé y Publicación ---
// int lastPublishedRelayState = -1; // -1: desconocido, 0: OFF, 1: ON. Solo publicar cambios.

// // --- Funciones de Callback y Lógica Principal ---

// // Función de callback para mensajes recibidos por el SERVIDOR MQTT
// void onMqttMessageReceived(const char * topic, const char * payload) {
//     Serial.printf("Mensaje recibido por el SERVIDOR [ %s ]: %s\n", topic, payload);

//     String message = payload;
//     int state = message.toInt();

//     if (String(topic) == String(topic_pb_str)) {
//         sensorPB_state = state;
//         lastUpdatePB = millis();
//         Serial.print("Estado PB (Actualizado por Servidor): "); Serial.println(sensorPB_state);
//     } else if (String(topic) == String(topic_pa_str)) {
//         sensorPA_state = state;
//         lastUpdatePA = millis();
//         Serial.print("Estado PA (Actualizado por Servidor): "); Serial.println(sensorPA_state);
//     }
// }

// void saveConfigCallback() {
//   Serial.println("Se guardará la configuración.");
//   strcpy(topic_pa_str, custom_topic_pa.getValue());
//   strcpy(topic_pb_str, custom_topic_pb.getValue());
//   strcpy(topic_status_str, custom_topic_status.getValue());

//   Serial.println("Nuevos valores de configuración:");
//   Serial.print("  Topic PA: "); Serial.println(topic_pa_str);
//   Serial.print("  Topic PB: "); Serial.println(topic_pb_str);
//   Serial.print("  Topic Status: "); Serial.println(topic_status_str);
// }

// // Esta función publica el estado del relé usando el servidor MQTT
// void publishRelayState(int state) {
//   if (state != lastPublishedRelayState) {
//     String payload = String(state);
//     Serial.printf("Publicando estado del relé (desde el servidor) en %s: %s\n", topic_status_str, payload.c_str());
//     mqtt.publish(topic_status_str, payload.c_str(), true); // ← Usar mensaje retained
//     lastPublishedRelayState = state;
//   }
// }

// void controlRelay() {
//   unsigned long currentTime = millis();

//   bool pb_is_updated_recently = (currentTime - lastUpdatePB < MAX_INACTIVITY_TIME);
//   bool pa_is_updated_recently = (currentTime - lastUpdatePA < MAX_INACTIVITY_TIME);

//   Serial.print("PB Estado: "); Serial.print(sensorPB_state); Serial.print(", Actualizado: "); Serial.println(pb_is_updated_recently ? "Sí" : "No");
//   Serial.print("PA Estado: "); Serial.print(sensorPA_state); Serial.print(", Actualizado: "); Serial.println(pa_is_updated_recently ? "Sí" : "No");

//   bool activateRelay = false;

//   if ((sensorPB_state == 1 && pb_is_updated_recently) || (sensorPA_state == 1 && pa_is_updated_recently)) {
//     activateRelay = true;
//     Serial.println(">>> Relé ACTIVADO (Consumo detectado y dato reciente) <<<");
//   } else {
//     Serial.println(">>> Relé DESACTIVADO (No hay consumo reciente o datos no válidos) <<<");
//   }

//   if (activateRelay) {
//     digitalWrite(RELAY_PIN, HIGH);
//     publishRelayState(1);
//   } else {
//     digitalWrite(RELAY_PIN, LOW);
//     publishRelayState(0);
//   }
// }

// // Nueva función para controlar el LED de estado
// void updateStatusLed() {
//   unsigned long currentTime = millis();
  
//   bool wifiConnected = (WiFi.status() == WL_CONNECTED);
//   bool pb_recent = (currentTime - lastUpdatePB < MAX_INACTIVITY_TIME);
//   bool pa_recent = (currentTime - lastUpdatePA < MAX_INACTIVITY_TIME);

//   // El LED se enciende si el WiFi está conectado Y AMBOS tópicos tienen datos recientes
//   if (wifiConnected && pb_recent && pa_recent) {
//     digitalWrite(STATUS_LED_PIN, HIGH); // Enciende el LED
//     // Serial.println("LED de estado: ON (WiFi OK y datos recientes en PA/PB)"); // Solo para depuración
//   } else {
//     digitalWrite(STATUS_LED_PIN, LOW); // Apaga el LED
//     // Serial.println("LED de estado: OFF (Falta WiFi o datos no recientes)"); // Solo para depuración
//   }
// }


// void setup() {
//     // Setup serial
//     Serial.begin(9600); // Usamos 115200 bps como en el sketch de la documentación
//     pinMode(RELAY_PIN, OUTPUT);
//     digitalWrite(RELAY_PIN, LOW);
//     Serial.println("Relé inicializado en estado DESACTIVADO (abierto).");

//     pinMode(STATUS_LED_PIN, OUTPUT); // Configurar el pin del LED de estado
//     digitalWrite(STATUS_LED_PIN, LOW); // Asegurarse de que el LED esté apagado al inicio
//     Serial.println("LED de estado inicializado en estado APAGADO.");


//     // --- Configuración WiFi con WiFiManager ---
//     WiFiManager wifiManager;
//     wifiManager.setDebugOutput(true);
//     wifiManager.setSaveConfigCallback(saveConfigCallback);

//     wifiManager.setConnectTimeout(120);
//     wifiManager.setConfigPortalTimeout(600);

//     // Configurar valores por defecto y añadir parámetros personalizados
//     custom_topic_pa.setValue(DEFAULT_TOPIC_PA, 64);
//     custom_topic_pb.setValue(DEFAULT_TOPIC_PB, 64);
//     custom_topic_status.setValue(DEFAULT_TOPIC_STATUS, 64);

//     wifiManager.addParameter(&custom_topic_pa);
//     wifiManager.addParameter(&custom_topic_pb);
//     wifiManager.addParameter(&custom_topic_status);

//     Serial.println("Intentando conexión WiFi o iniciando portal de configuración...");
//     if (!wifiManager.autoConnect("ESP01_Setup_AP", "password123")) {
//         Serial.println("Fallo en la conexión WiFi (timeout) o el AP no pudo iniciarse. Reiniciando...");
//         delay(3000);
//         ESP.restart();
//     }
//     Serial.println("WiFi conectado.");
//     Serial.printf("IP del ESP01 (Broker): %s\n", WiFi.localIP().toString().c_str());


//     // Cargar los valores de configuración después de la conexión (o si ya estaban guardados)
//     strcpy(topic_pa_str, custom_topic_pa.getValue());
//     strcpy(topic_pb_str, custom_topic_pb.getValue());
//     strcpy(topic_status_str, custom_topic_status.getValue());

//     Serial.println("Valores de configuración cargados/actuales:");
//     Serial.print("  Topic PA: "); Serial.println(topic_pa_str);
//     Serial.print("  Topic PB: "); Serial.println(topic_pb_str);
//     Serial.print("  Topic Status: "); Serial.println(topic_status_str);

//     // --- Inicialización del Servidor PicoMQTT (Broker) ---
//     // Suscribirse a todos los tópicos y adjuntar el callback de manejo de mensajes
//     mqtt.subscribe("#", onMqttMessageReceived); // Usamos el callback definido arriba

//     // Iniciar el servidor MQTT (broker)
//     mqtt.begin();
//     Serial.println("Servidor PicoMQTT (Broker) iniciado en el puerto 1883.");

//       // OTA
//   ArduinoOTA.setHostname("ESP-PEISA");

//   ArduinoOTA.onStart([]() {
//     Serial.println("Inicio de actualización OTA...");
//   });

//   ArduinoOTA.onEnd([]() {
//     Serial.println("\nActualización OTA finalizada.");
//   });

//   ArduinoOTA.onError([](ota_error_t error) {
//     Serial.printf("Error OTA [%u]: ", error);
//     if (error == OTA_AUTH_ERROR) Serial.println("Falló la autenticación");
//     else if (error == OTA_BEGIN_ERROR) Serial.println("Falló el inicio");
//     else if (error == OTA_CONNECT_ERROR) Serial.println("Falló la conexión");
//     else if (error == OTA_RECEIVE_ERROR) Serial.println("Falló la recepción");
//     else if (error == OTA_END_ERROR) Serial.println("Falló el final");
//   });

//   ArduinoOTA.begin();
//   Serial.println("OTA listo. Puede actualizar el firmware por red.");

// }

// void loop() {
//     // El loop del servidor es esencial para procesar mensajes y conexiones
//     ArduinoOTA.handle();

//     mqtt.loop();

//     // Lógica para el control del relé y publicación de su estado
//     unsigned long currentMillis = millis();
//     if (currentMillis - lastRelayControlTime >= RELAY_CHECK_INTERVAL) {
//         lastRelayControlTime = currentMillis;
//         controlRelay();
//         Serial.println(ESP.getFlashChipSize());
//     }

//     // Actualizar el estado del LED periódicamente
//     if (currentMillis - lastLedUpdateTime >= LED_UPDATE_INTERVAL) {
//         lastLedUpdateTime = currentMillis;
//         updateStatusLed();
//     }
// }