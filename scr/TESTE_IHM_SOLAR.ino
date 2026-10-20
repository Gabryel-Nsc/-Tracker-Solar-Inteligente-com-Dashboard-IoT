#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

#include "web_server.h"

// ==========================================
// CONFIGURAÇÕES E PINOS (Gabriel)
// ==========================================
const char* ssid = "GABRIEL";
const char* password = "Biel_32150757";

const int pinoLdrCE = 33;  
const int pinoLdrCD = 35;  
const int pinoLdrBE = 32;  
const int pinoLdrBD = 34;  

const int pinoServoAzimutal = 14; 
const int pinoServoVertical = 13; 
const int pinoSDA = 21;            
const int pinoSCL = 22;            

const int limiteMin = 15;  
const int limiteMax = 165;

// ==========================================
// PARÂMETROS DO CONTROLADOR PID
// ==========================================
float Kp = 0.015f;  
float Ki = 0.001f;  
float Kd = 0.005f;  

float erroIntVert = 0.0f, erroIntHoriz = 0.0f;
float erroAntVert = 0.0f, erroAntHoriz = 0.0f;

float posAzimutalFloat = 90.0f;
float posVerticalFloat = 45.0f;

// ==========================================
// FILTRO EMA E TOLERÂNCIA DINÂMICA
// ==========================================
int toleranciaLuz = 150;          
bool usarToleranciaDinamica = true; 
float fatorK = 0.08f;               
const int toleranciaMinima = 30;    
int toleranciaEfetiva = 150;        

float alfaFiltro = 0.15f;          
long intervaloMovimento = 100; 

int offsetCE = 0, offsetCD = 0, offsetBE = 0, offsetBD = 0;

const long intervaloLCD = 300;
const long tempoSplashLCD = 2000; 

// ==========================================
// VARIÁVEIS GLOBAIS E OBJETOS
// ==========================================
WebServer server(80);
Servo servoAzimutal;
Servo servoVertical;
LiquidCrystal_I2C lcd(0x27, 16, 2);

int valCE = 0, valCD = 0, valBE = 0, valBD = 0;
float ldrCE_filtrado = 0.0f, ldrCD_filtrado = 0.0f, ldrBE_filtrado = 0.0f, ldrBD_filtrado = 0.0f;
int luzMediaAmbiente = 0;

int posicaoAzimutal = 90;   
int posicaoVertical = 45;   

int ultimaPosAzimutal = -1;
int ultimaPosVertical = -1;

bool modoManualAtivo = false;
bool modoUmEixo = false; 

bool lcdConectado = false;
bool lcdEmSplash = false;
bool wifiNotificado = false;

unsigned long tempoAnteriorMovimento = 0;   
unsigned long tempoAnteriorLCD = 0;
unsigned long tempoInicioLCD = 0;
unsigned long tempoAnteriorFiltro = 0;

// ==========================================
// FUNÇÕES
// ==========================================
void inicializarServos();
void inicializarLCD();
void executarAutocalibracao();
void iniciarWiFiAssincrono();
void verificarStatusWiFi();
void configurarRotasWeb();

void tratarRastreamentoAutomatico(unsigned long tempoAtual);
void atualizarLCD(unsigned long tempoAtual);

void handleRoot();
void handleLogin();
void handleLogout();
void handleSetModo();
void handleSetEixos();
void handleControlManual();
void handleSetParametros();
void handleCalibrar();
void handleSetPID();

// ==========================================
// SETUP E LOOP
// ==========================================
void setup() {
  Serial.begin(115200);

  inicializarServos();
  inicializarLCD();
  executarAutocalibracao(); 
  iniciarWiFiAssincrono();

  configurarRotasWeb();
  server.begin();
}

void loop() {
  server.handleClient();
  unsigned long tempoAtual = millis();

  verificarStatusWiFi();

  // Isola a leitura dos sensores e o filtro a cada 20ms para funcionamento correto do EMA
  if (tempoAtual - tempoAnteriorFiltro >= 20) {
    tempoAnteriorFiltro = tempoAtual;

    int rawCE = analogRead(pinoLdrCE);
    int rawCD = analogRead(pinoLdrCD);
    int rawBE = analogRead(pinoLdrBE);
    int rawBD = analogRead(pinoLdrBD);

    // Aplicação do Filtro EMA nos sinais brutos ajustados pelo offset
    ldrCE_filtrado = (alfaFiltro * (rawCE - offsetCE)) + ((1.0f - alfaFiltro) * ldrCE_filtrado);
    ldrCD_filtrado = (alfaFiltro * (rawCD - offsetCD)) + ((1.0f - alfaFiltro) * ldrCD_filtrado);
    ldrBE_filtrado = (alfaFiltro * (rawBE - offsetBE)) + ((1.0f - alfaFiltro) * ldrBE_filtrado);
    ldrBD_filtrado = (alfaFiltro * (rawBD - offsetBD)) + ((1.0f - alfaFiltro) * ldrBD_filtrado);

    // Aplicação do constrain APÓS o filtro suavizar as flutuações
    valCE = constrain((int)ldrCE_filtrado, 0, 4095);
    valCD = constrain((int)ldrCD_filtrado, 0, 4095);
    valBE = constrain((int)ldrBE_filtrado, 0, 4095);
    valBD = constrain((int)ldrBD_filtrado, 0, 4095);

    luzMediaAmbiente = (valCE + valCD + valBE + valBD) / 4;
    
    if (usarToleranciaDinamica) {
      int tolCalc = (int)(luzMediaAmbiente * fatorK);
      toleranciaEfetiva = (tolCalc < toleranciaMinima) ? toleranciaMinima : tolCalc;
    } else {
      toleranciaEfetiva = toleranciaLuz;
    }
  }

  tratarRastreamentoAutomatico(tempoAtual);
  atualizarLCD(tempoAtual);
}

// ==========================================
// INICIALIZAÇÕES E CALIBRAÇÃO 
// ==========================================
void inicializarServos() {
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  
  servoAzimutal.setPeriodHertz(50);
  servoVertical.setPeriodHertz(50);

  servoAzimutal.attach(pinoServoAzimutal, 500, 2400);
  servoVertical.attach(pinoServoVertical, 500, 2400);

  servoAzimutal.write((int)posAzimutalFloat);
  servoVertical.write((int)posVerticalFloat);
}

void inicializarLCD() {
  Wire.begin(pinoSDA, pinoSCL);
  Wire.beginTransmission(0x27);
  if (Wire.endTransmission() == 0) {
    lcdConectado = true;
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("TRACKER SOLAR:");
    lcd.setCursor(0, 1);
    lcd.print("Calibrando LDRs");
    lcdEmSplash = true;
    tempoInicioLCD = millis();
  }
}

void executarAutocalibracao() {
  long somaCE = 0, somaCD = 0, somaBE = 0, somaBD = 0;
  const int numAmostras = 25;
  for (int i = 0; i < numAmostras; i++) {
    somaCE += analogRead(pinoLdrCE);
    somaCD += analogRead(pinoLdrCD);
    somaBE += analogRead(pinoLdrBE);
    somaBD += analogRead(pinoLdrBD);
    delay(10);
  }
  int avgCE = somaCE / numAmostras;
  int avgCD = somaCD / numAmostras;
  int avgBE = somaBE / numAmostras;
  int avgBD = somaBD / numAmostras;
  
  int avgGeral = (avgCE + avgCD + avgBE + avgBD) / 4;

  offsetCE = avgCE - avgGeral;
  offsetCD = avgCD - avgGeral;
  offsetBE = avgBE - avgGeral;
  offsetBD = avgBD - avgGeral;
  
  ldrCE_filtrado = avgCE - offsetCE;
  ldrCD_filtrado = avgCD - offsetCD;
  ldrBE_filtrado = avgBE - offsetBE;
  ldrBD_filtrado = avgBD - offsetBD;

  valCE = (int)ldrCE_filtrado;
  valCD = (int)ldrCD_filtrado;
  valBE = (int)ldrBE_filtrado;
  valBD = (int)ldrBD_filtrado;
}

void iniciarWiFiAssincrono() {
  WiFi.persistent(false);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
}

void verificarStatusWiFi() {
  if (!wifiNotificado && WiFi.status() == WL_CONNECTED) {
    wifiNotificado = true;
    if (lcdConectado && lcdEmSplash) {
      lcd.setCursor(0, 1);
      lcd.print(WiFi.localIP());
    }
  }
}

// ==========================================
// HANDLERS WEB
// ==========================================
void configurarRotasWeb() {
  server.on("/", handleRoot);
  server.on("/login", HTTP_POST, handleLogin);
  server.on("/logout", handleLogout);
  server.on("/setModo", handleSetModo);
  server.on("/setEixos", handleSetEixos);
  server.on("/controlManual", handleControlManual);
  server.on("/setParametros", handleSetParametros);
  server.on("/calibrar", handleCalibrar);
  server.on("/setPID", handleSetPID);
  server.on("/dados", enviarDadosJSON);
}

void handleRoot() {
  if (cookies_autenticado) server.send(200, "text/html", RASTREADOR_HTML);
  else server.send(200, "text/html", LOGIN_HTML);
}

void handleLogin() {
  if (server.arg("username") == auth_user && server.arg("password") == auth_pass) {
    cookies_autenticado = true;
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleLogout() {
  cookies_autenticado = false;
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleSetModo() {
  modoManualAtivo = server.arg("manual").toInt() == 1;
  server.send(200, "text/plain", "OK");
}

void handleSetEixos() {
  modoUmEixo = server.arg("um").toInt() == 1;
  server.send(200, "text/plain", "OK");
}

void handleControlManual() {
  if (modoManualAtivo) {
    posicaoAzimutal = server.arg("h").toInt();
    posicaoVertical = server.arg("v").toInt();
    
    posAzimutalFloat = posicaoAzimutal;
    posVerticalFloat = posicaoVertical;

    if (posicaoAzimutal != ultimaPosAzimutal) {
      servoAzimutal.write(posicaoAzimutal);
      ultimaPosAzimutal = posicaoAzimutal;
    }
    if (posicaoVertical != ultimaPosVertical) {
      servoVertical.write(posicaoVertical);
      ultimaPosVertical = posicaoVertical;
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleSetParametros() {
  if (server.hasArg("tol")) toleranciaLuz = server.arg("tol").toInt();
  if (server.hasArg("alfa")) alfaFiltro = server.arg("alfa").toFloat();
  if (server.hasArg("int")) intervaloMovimento = server.arg("int").toInt();
  if (server.hasArg("dyn")) usarToleranciaDinamica = server.arg("dyn").toInt() == 1;
  if (server.hasArg("fk")) fatorK = server.arg("fk").toFloat();
  server.send(200, "text/plain", "OK");
}

void handleCalibrar() {
  executarAutocalibracao();
  server.send(200, "text/plain", "OK");
}

void handleSetPID() {
  if (server.hasArg("kp")) Kp = server.arg("kp").toFloat();
  if (server.hasArg("ki")) Ki = server.arg("ki").toFloat();
  if (server.hasArg("kd")) Kd = server.arg("kd").toFloat();
  
  erroIntVert = 0.0f;
  erroIntHoriz = 0.0f;
  
  server.send(200, "text/plain", "OK");
}

// ==========================================
// MALHA DE CONTROLE E RASTREAMENTO AUTOMÁTICO
// ==========================================
void tratarRastreamentoAutomatico(unsigned long tempoAtual) {
  if (modoManualAtivo) return;

  unsigned long dtMs = tempoAtual - tempoAnteriorMovimento;
  
  if (dtMs >= intervaloMovimento) {
    tempoAnteriorMovimento = tempoAtual;
    float dt = dtMs / 1000.0f; 

    float mediaCima  = (valCE + valCD) / 2.0f;
    float mediaBaixo = (valBE + valBD) / 2.0f;
    float mediaEsq   = (valCE + valBE) / 2.0f;
    float mediaDir   = (valCD + valBD) / 2.0f;

    float erroVert  = mediaCima - mediaBaixo;
    float erroHoriz = mediaEsq - mediaDir;

    // Correção anti-windup: Zera o termo integral se estiver dentro da tolerância
    if (abs(erroVert) <= toleranciaEfetiva) {
      erroVert = 0;
      erroIntVert = 0;
    }
    if (abs(erroHoriz) <= toleranciaEfetiva) {
      erroHoriz = 0;
      erroIntHoriz = 0;
    }

    if (!modoUmEixo) {
      erroIntVert += (erroVert * dt);
      erroIntVert = constrain(erroIntVert, -100.0f, 100.0f); 

      float dVert = (erroVert - erroAntVert) / dt;
      float saidaVert = (Kp * erroVert) + (Ki * erroIntVert) + (Kd * dVert);
      erroAntVert = erroVert;

      posVerticalFloat += saidaVert; 
      posVerticalFloat = constrain(posVerticalFloat, 0, 90);
      posicaoVertical = (int)posVerticalFloat;

      // Banda Morta (só envia pulso se a diferença for >= 1 grau)
      if (abs(posicaoVertical - ultimaPosVertical) >= 1) {
        servoVertical.write(posicaoVertical);
        ultimaPosVertical = posicaoVertical;
      }
    }

    erroIntHoriz += (erroHoriz * dt);
    erroIntHoriz = constrain(erroIntHoriz, -100.0f, 100.0f); 

    float dHoriz = (erroHoriz - erroAntHoriz) / dt;
    float saidaHoriz = (Kp * erroHoriz) + (Ki * erroIntHoriz) + (Kd * dHoriz);
    erroAntHoriz = erroHoriz;

    posAzimutalFloat -= saidaHoriz;
    posAzimutalFloat = constrain(posAzimutalFloat, limiteMin, limiteMax);
    posicaoAzimutal = (int)posAzimutalFloat;

    // Banda Morta (só envia pulso se a diferença for >= 1 grau)
    if (abs(posicaoAzimutal - ultimaPosAzimutal) >= 1) {
      servoAzimutal.write(posicaoAzimutal);
      ultimaPosAzimutal = posicaoAzimutal;
    }
  }
}

void atualizarLCD(unsigned long tempoAtual) {
  if (!lcdConectado) return;

  if (lcdEmSplash) {
    if (tempoAtual - tempoInicioLCD >= tempoSplashLCD) {
      lcdEmSplash = false;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("AUTO RASTREIO");
    }
    return;
  }

  if (tempoAtual - tempoAnteriorLCD >= intervaloLCD) {
    tempoAnteriorLCD = tempoAtual;
    lcd.setCursor(0, 1);
    lcd.print("H:");
    lcd.print(posicaoAzimutal);
    lcd.print(" V:");
    lcd.print(posicaoVertical);
    lcd.print("   ");
  }
}