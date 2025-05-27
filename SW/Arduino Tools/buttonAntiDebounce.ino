const int pinPulsante = 7;
const int pinLed = 13;

bool statoLed = false;
bool ultimoStatoPulsante = LOW;
unsigned long ultimoDebounceTime = 0;
const unsigned long debounceDelay = 50; // 50 ms per filtrare il rimbalzo

void setup() {
  pinMode(pinPulsante, INPUT_PULLUP);  // Usa resistenza di pullup interna
  pinMode(pinLed, OUTPUT);
  digitalWrite(pinLed, LOW);
}

void loop() {
  // Legge lo stato attuale del pulsante
  bool lettura = digitalRead(pinPulsante);

  // Se è cambiato rispetto alla lettura precedente, resetta il timer di debounce
  if (lettura != ultimoStatoPulsante) {
    ultimoDebounceTime = millis();
  }

  // Se è passato abbastanza tempo, considera stabile il nuovo stato
  if ((millis() - ultimoDebounceTime) > debounceDelay) {
    // Controlla se il pulsante è stato premuto (transizione da HIGH a LOW)
    if (lettura == LOW && ultimoStatoPulsante == HIGH) {
      statoLed = !statoLed; // Inverti stato del LED
      digitalWrite(pinLed, statoLed);
    }
  }

  // Salva lo stato per il ciclo successivo
  ultimoStatoPulsante = lettura;
}
