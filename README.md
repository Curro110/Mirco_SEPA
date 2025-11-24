# Micro_SEPA Guia de instalación
Este documento explica cómo configurar el microcontrolador y la API de WhatsApp (CallMeBot) para el correcto funcionamiento del proyecto.
Siga los pasos cuidadosamente, ya que la configuración no es trivial.
1. Activación del Bot de WhatsApp
Agregue el número de teléfono +34 621 33 17 09 en sus contactos con el nombre PastilleroBot.
2.	En WhatsApp busque el contacto y escríbale ```I allow callmebot to send me messages``` esto activará el bot para que reconozca su número de teléfono.
3.	 Mandará un mensaje con una apikey a su número de teléfono el cual deberá copiar para poder configurar el programa.
4.	 Abra Arduino IDE e instale el paquete de board de ESP32 y en librerías instale también UrlEndoce de Masayuki.
5.	En esta parte de código sustituya su ssid por el nombre de su wifi y la contraseña de esta, aparte copie y pegue su apikey y ponga el número de teléfono donde recibirá los mensajes.
 ```
const char* ssid = "NOMBRE_DE_TU_WIFI";
const char* password = "CONTRASEÑA_DE_TU_WIFI";

String apiKey = "TU_API_KEY";
String phoneNumber = "34XXXXXXXXX";  // Prefijo + número, sin el símbolo +
```
Al arrancar el programa debera salir un mensaje por WhatsApp diciendo que esta conectada la esp32.

6.   Conenecte los pines del Micro alos pines de la esp32 como muestra el esquema.
   
| Micro | ESP32 |
| ----- | ----- |
| K6    | 4     |
| K7    | 5     |
| H0    | 18    |

7.  Conecte lo siguiente en el Micro:

  El BoosterPack de servos.

  La pantalla (parte superior del Micro).

  Los servos a los terminales S1, S2 y S3.
