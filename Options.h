#define WIFI_SSID_LEN_MAX (20)
#define WIFI_PASSWORD_LEN_MAX (30)
char ssidWiFi[WIFI_SSID_LEN_MAX];
char passwordWiFi[WIFI_PASSWORD_LEN_MAX];


#define MQTT_SERVER_LEN_MAX (50)
#define MQTT_PORT_LEN_MAX (6)
#define MQTT_USER_LEN_MAX (20)
#define MQTT_PASSWORD_LEN_MAX (30)

char mqtt_server[MQTT_SERVER_LEN_MAX];// = "192.168.1.52";//"m14.cloudmqtt.com"; // Имя сервера MQTT
char mqtt_port[MQTT_PORT_LEN_MAX];// = 1883;//15303; // Порт для подключения к серверу MQTT
char mqtt_user[MQTT_USER_LEN_MAX];// = "";//"aNano1"; // Логи от сервер
char mqtt_password[MQTT_PASSWORD_LEN_MAX];// = "";//"Hui123"; // Пароль от сервера

const String opt_wifi_file = "/opts/wifi.p";
#define opt_wifi_ssid_i (0)
#define opt_wifi_password_i (1)
const String opt_mqtt_file = "/opts/mqtt.p";
#define opt_mqtt_server_i (0)
#define opt_mqtt_port_i (1)
#define opt_mqtt_user_i (2)
#define opt_mqtt_password_i (3)

