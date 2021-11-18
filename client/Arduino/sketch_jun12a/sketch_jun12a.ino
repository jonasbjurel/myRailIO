void setup() {
  // put your setup code here, to run once:
    MALLOC_CAP_SPIRAM;
    Serial.begin(115200);
    Serial.println("Heap");
    Serial.println(ESP.getHeapSize());
    Serial.println("PSRAM");
    Serial.println(ESP.getPsramSize());

}

void loop() {
  // put your main code here, to run repeatedly:

}
