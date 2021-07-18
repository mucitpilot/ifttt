//IFTTT kullanımı ve Google Excel Tablolarına veri aktarımı
//ESP32 veya ESP8266 ile çalışır.

#ifdef ESP32
  #include <WiFi.h>
#else
  #include <ESP8266WiFi.h>
#endif

#include <Wire.h>
#include <Adafruit_Sensor.h>//https://github.com/adafruit/Adafruit_Sensor
#include <DHT.h>//https://github.com/adafruit/DHT-sensor-library

// Ağ bilgilerinizi girin
const char* ssid     = "*******";
const char* password = "*******";

// IFTTT sitesinden aldığınız applet linkini girin
const char* resource = "/trigger/sicaklik_nem/with/key/***********";

// Örnek olarak IFTTT'den aldığınız link aşağıdakine benzer olmalıdır:
//const char* resource = "/trigger/sicaklik_nem/with/key/aaaaasizeözelAPIkeybbbb";

// IFTTT webhook servisi için bağlanılacak sunucu adresi
const char* server = "maker.ifttt.com";

// Deepsleep için uyku süreleri
uint64_t msdenSaniyeyeCevir = 1000000;  // microsaniyeden saniyeye dönüşüm katsayısı
// 1 dakika için uyku süresi = 60 saniye
//bu uygulamada videoda çok beklememek adına 1dk yaptım
//siz daha uzun süreler girebilirsiniz.
uint64_t UYKU_SURESI = 60;

// DHT sensör GPIO2 pinine bağlı
#define DHTPIN 2    

// DHT tipini seçin
  #define DHTTYPE    DHT11     // DHT 11
//#define DHTTYPE    DHT22     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)

DHT dht(DHTPIN, DHTTYPE); //bir dht nesnesi oluşturuyoruz

// Kodda kullandığımız değişkenler
float temperature;
float humidity;


void setup() {
  Serial.begin(115200); 
  delay(2000);

  

  agaBaglan();
  IFTTTistekYap();
    
  #ifdef ESP32
    // eğer kart ESP32 ise zamanlayıcılı deepsleep'i aktive et
    esp_sleep_enable_timer_wakeup(UYKU_SURESI * msdenSaniyeyeCevir);    
    Serial.println("Kart Uykuya Dalıyor");
    // Belirlenen saniye kadar uyanık kalacak şekilde kartı uykuya gönderiyoruz
    esp_deep_sleep_start();
  #else //eğer kart esp8266 ise
    // Belirlenen saniye kadar uyanık kalacak şekilde kartı uykuya gönderiyoruz
    Serial.println("Kart Uykuya Dalıyor");
    ESP.deepSleep(UYKU_SURESI * msdenSaniyeyeCevir); 
  #endif
}

void loop() {
  // zaten deepsleep olacağı için buraya girmeye gerek olmayacak 
}

// ağ bağlantısını sağlamak için kullandığımız fonksiyon
void agaBaglan() {
  Serial.print("Ağa Bağlanılıyor: "); 
  Serial.print(ssid);
  WiFi.begin(ssid, password);  

  int timeout = 10 * 4; // 4 saniye zaman aşımı
  while(WiFi.status() != WL_CONNECTED  && (timeout-- > 0)) {
    delay(250);
    Serial.print(".");
  }
  Serial.println("");

  if(WiFi.status() != WL_CONNECTED) {
     Serial.println("Ağa bağlanılamadı, uykuya devam ediliyor");
  }

  Serial.print("Ağ bağlantısı Sağlandı: "); 
  Serial.print(millis());
  Serial.print(", IP adresi: "); 
  Serial.println(WiFi.localIP());
}

// IFTTT web sunucusuna HTTP isteği göndermek için gerekli prosedür
void IFTTTistekYap() {
  Serial.print("Sunucuya Bağlanılıyor: "); 
  Serial.print(server);
  //5 defa bağlanmayı deneyecek
  WiFiClient client;
  int retries = 5;
  while(!!!client.connect(server, 80) && (retries-- > 0)) {
    Serial.print(".");
  }
  Serial.println();
  if(!!!client.connected()) {
    Serial.println("Sunucuya Bağlanılamadı!...");
  }
  
  Serial.print("İstek Kaynağı: "); 
  Serial.println(resource);

  // Ölçülen Sıcaklığı ve Nemi IFTTT sunucusuna göndermek için gerekli veri bloğu
  String jsonObject = String("{\"value1\":\"");
  jsonObject=jsonObject+sicaklikGonder();
  jsonObject=jsonObject+String("\",\"value2\":\"");
  jsonObject=jsonObject+nemGonder();
  jsonObject=jsonObject+String("\"}");
                                    
  client.println(String("POST ") + resource + " HTTP/1.1");
  client.println(String("Host: ") + server); 
  client.println("Connection: close\r\nContent-Type: application/json");
  client.print("Content-Length: ");
  client.println(jsonObject.length());
  client.println();
  client.println(jsonObject);
        
  int timeout = 5 * 10; // 5 saniye zaman aşımı ile deneyecek            
  while(!!!client.available() && (timeout-- > 0)){
    delay(100);
  }
  if(!!!client.available()) {
    Serial.println("Cevap Alınamadı...");
  }
  while(client.available()){
    Serial.write(client.read());
  }
  
  Serial.println("\nbağlantı sonlandırılıyor");
  client.stop(); 
}

float sicaklikGonder()
{
  temperature = dht.readTemperature();
  //veri okunamadıysa
  if (isnan(temperature)){
    Serial.println("DHT sensör sıcaklık verisi okunamadı!!!");
    temperature = 0;
    
  }
  Serial.print("Okunan Sıcaklık:");
  Serial.println(temperature);
 return temperature;
}

float nemGonder()
{
  humidity = dht.readHumidity();
 
  //veri okunamadıysa
  if (isnan(humidity)){
    Serial.println("DHT sensör nem verisi okunamadı!!!");
    humidity = 0;
  }
    
  Serial.print("Okunan Nem:");
  Serial.println(humidity);
  return humidity;

}
