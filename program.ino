#include <ezBuzzer.h>           //buzzer kirjasto
#include <Servo.h>              //Servomoottorin kirjasto. Servo toimii Timer1 -> kirjoita ajastin 8-bit Timerille 2.
#include <SPI.h>                //Kuljettaa tiedon lukituksen tilasta
#include <MFRC522.h>            //RFID-lukijan kirjasto
#include <Keypad.h>             // Näppäimistön kirjasto
#include <LiquidCrystal_I2C.h>  //LCD näytön kirjasto
#include <Wire.h>               //LCD näytön tukikirjasto. 6/12: Tarvitaanko tätä?
#include <EEPROM.h>             //EEPROM-muisti

#define SS_PIN 10
#define RST_PIN 9
#define BUZZER_PIN 3

int position = 0;                    //servomoottorin positio on oletukselta 0 (astetta)
MFRC522 mfrc522(10, 9);              // RFID lukija MFRC522. Syntaksi määrittelylle: mfrc522(SS_PIN, RST_PIN)
LiquidCrystal_I2C lcd(0x27, 16, 2);  //LCD näyttö kommunikoi I2C moduulin kautta
Servo sg90;                          //määritellään servomoottori sg90
ezBuzzer buzzer(BUZZER_PIN);         //tässä vielä määritellään että käytetään tosiaan ezBuzzer-kirjastoa.
int melody[] = {                     //Normaali taustamusiikki
NOTE_B5,
NOTE_D6, NOTE_A6, NOTE_G6, NOTE_D6
};
int melody2[] = {  //Error-ääni
  NOTE_C5, NOTE_C5, NOTE_G4
};
int melody3[] = {  //Jouluinen taustamusiikki
  NOTE_E5, NOTE_E5, NOTE_E5,
  NOTE_E5, NOTE_E5, NOTE_E5,
  NOTE_E5, NOTE_G5, NOTE_C5, NOTE_D5,
  NOTE_E5,
  NOTE_F5, NOTE_F5, NOTE_F5, NOTE_F5,
  NOTE_F5, NOTE_E5, NOTE_E5, NOTE_E5, NOTE_E5,
  NOTE_E5, NOTE_D5, NOTE_D5, NOTE_E5,
  NOTE_D5, NOTE_G5
};
int noteDurations3[] = {  //zelda kestot
  11, 11, 6,
  11, 11, 6,
  11, 11, 11, 11,
  3,
  11, 11, 11, 11,
  11, 11, 11, 22, 22,
  11, 11, 11, 11,
  6, 6
};
int noteDurations[] = {  //taustamusiikki: jouluinen nuottien kestot
2,4,2,4,1
};
int noteDurations2[] = {
  //error nuottien kestot
  9,
  11,
  3,
};
/*EEPROM-muuttujat*/
char viimeisinKayttaja[6];     //tallentaa viimeisimmän käyttäjän
char viimeisinKayttoAika[12];  //   6/12 Tähän TULEE aika kun Timer kakkosella toimiva ajastin on lisätty ohjelmaan ja sen avulla saadaan tarvittavat muuttua kasattua yhdeksi merkkijonoksi joka on helppo tallentaa ja printata. -S
/*^^^^^^^^^^^^^^^^*/
bool RFIDMode = true;                               //oletuksena RFID-lukija-asetus on päällä. Asetus menee pois päältä jos näppäimistö aktivoidaan, ja tulee takaisin päälle kun lukitus on avattu koodilla.
bool numeronap = false;                             //numeronap-asetus on poissä päältä. Tällä on vähänkun käänteinen logiikka, tämä muuttuja on totta silloin kun näppäimistö on jo käytössä.
bool onAuki = false;                                //Tämä muuttuja pitää ohjelmassa silmällä lukituksen tilaa.
bool jouluMusic = false;                               //käyttäjän itse vaihtama, koskee vaan musiikkia.
bool browse = false;
bool displayTime = false;
bool slotassingmentmode = false;                    //estää ohjelman tilaa vaihtumasta kun uutta tagia ollaan alustamassa.
char initial_password[4] = { '1', '2', '3', '4' };  // Salasana joka avaa kassakaapin.
char password[4];
char key_pressed = 0;
String aika = "                ";
String komento = " ";           // Serial Monitorille lokien printtaamista varten komentostringi. Luetaan ja sen perusteella tehdään printit.
String tagUID = "0C EE BF 6D";  // Valmiiksi alustettu "admin" tagi.
String tagUIDname = "Admin";
String tagUID2 = "          ";  // Tässä se tag-muuttuja, mitä verrataan lukitusta avattaessa ja jonka voi sitten asettaa itse kun laite on auki.
String tagUID2name = "User1";
enum State {
  RFID_MODE,
  KEYPAD_MODE,
  OPEN_MODE,
  ASSIGN_SLOT,
};
uint8_t painallustenMaara = 0;  // Laskee painallukset kokonaislukuina
const byte rows = 4;            // Ja tästä alkaa
const byte columns = 3;         // näppäimistön määrittely
// Merkkitaulukko joka kartoittaa näppäimet
char hexaKeys[rows][columns] = {
  { '1', '2', '3' },
  { '4', '5', '6' },
  { '7', '8', '9' },
  { '*', '0', '#' }
};
byte row_pins[rows] = { A0, A1, A2, A3 };  // Pinnit keypadille
byte column_pins[columns] = { 5, 6, 8 };
Keypad keypad_key = Keypad(makeKeymap(hexaKeys), row_pins, column_pins, rows, columns);  //näppäimistö laitteena pitää määritellä tässä vasta kun hexaKeys, row_pins, column_pins on luotu.

void setup() {
  Serial.begin(9600);
  SPI.begin();         // init SPI bus
  mfrc522.PCD_Init();  // Init MFRC522
  EEPROM.begin();
  lcd.init();            //lcd näytön alustus
  lcd.backlight();       //lcd näytön taustavalo päälle
  lcd.clear();           //"putsaa" näytön 6/12: En ole nähnyt todisteita tämän toimivuudesta kertaakaan -S
  sg90.attach(2);        //servo käyttää 2. digitaalipinnia
  sg90.write(position);  //asetetaan servon asento 0 eli vaakatasoon joka on lukituksessa pystyssä eli lukossa.
  pinMode(A4, OUTPUT);
  pinMode(A5, OUTPUT);
  pinMode(3, OUTPUT);  //summeri
  pinMode(7, OUTPUT);  //vihreä
  Serial.println();
  Serial.print("Enter /help for a list of commands.\n");
}

void loop() {
  LukitusTila();
  ajastin();
  buzzer.loop();  //tämä on pakko kirjaston mukaisesti olla tässä loopissa. Myöhästyttää melodioita jos loopissa ei olla esim. delay() takia, mutta sille ei voi mitään.
  KayttoTila();
}

//KayttoTila-aliohjelma kirjoittaa LCD-näytölle lukituksen tilan ja tarkistelee myös milloin se tulisi ohjelman sisäisten tapahtumien johdosta muuttaa.
//aliohjelma kirjoittaa näytölle lukituksen tilan olevan Keypad vain silloin kun jotakin näppäintä on painettu. Laitteen sulkeminen ja koodin väärin antaminen
//johtaa aina RFIDModen ehtojen täyttymiseen. Toisin sanoen lukitus resetoituu aina RFID-tilaan.
void KayttoTila() {
  State state = DEFAULT;
  if (RFIDMode == true && onAuki == false && slotassingmentmode == false) {
    state = RFID_MODE;
  } else if (numeronap == false && onAuki == false && slotassingmentmode == false) {
    state = KEYPAD_MODE;
  } else if (onAuki == true && slotassingmentmode == false) {
    state = OPEN_MODE;
  } else if (onAuki == true && slotassingmentmode == true) {
    state = ASSIGN_SLOT;
  }
  switch (state) {
    case RFID_MODE:
      digitalWrite(7, 1);
      lcd.setCursor(0, 0);
      lcd.print("Safe Mode: RFID");
      lcd.setCursor(0, 1);
      lcd.print("Read tag to open");
      RFIDlukija();
      break;
    case KEYPAD_MODE:
      digitalWrite(7, 1);
      lcd.setCursor(0, 0);
      lcd.print("Safe Mode:Keypad");
      lcd.setCursor(0, 1);
      lcd.print("PIN-code to open");
      break;
    case OPEN_MODE:
      digitalWrite(7, 0);
      lcd.setCursor(0, 0);
      lcd.print("Safe Mode: Open");
      if (jouluMusic == true) {
        if (buzzer.getState() == BUZZER_IDLE) {
          int length = sizeof(noteDurations) / sizeof(int);
          buzzer.playMelody(melody, noteDurations, length);
        }
      } else {
        if (buzzer.getState() == BUZZER_IDLE) {
          int length = sizeof(noteDurations3) / sizeof(int);
          buzzer.playMelody(melody3, noteDurations3, length);
        }
      }
      if (browse == true){
      lcd.setCursor(0, 1);
      lcd.print("2-Music 3-Time  ");
      KayttoLiittyma();
      return;
      }
      if (displayTime == true){
      lcd.setCursor(0, 1);
      lcd.print("Time: " + aika + "      ");
      KayttoLiittyma();
      return;
      }
      else {
      }
      lcd.setCursor(0, 1);
      lcd.print("#-Close 1-Add  ");
      KayttoLiittyma();
      return;
  }
}

void LukitusTila() {
  key_pressed = keypad_key.getKey();
  if (key_pressed) {
    RFIDMode = false;
    numeronap = true;
    if (RFIDMode == false && onAuki == false) {
      password[painallustenMaara++] = key_pressed;  // Tallettaa painettuja nappeja password-merkkimuuttujaan.
      switch (painallustenMaara) {
        case 1:
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Safe Mode:Keypad");
          lcd.setCursor(0, 1);
          lcd.print("Entered: *");
          numeronap = true;
          delay(1000);
          break;
        case 2:
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Safe Mode:Keypad");
          lcd.setCursor(0, 1);
          lcd.print("Entered: **");
          numeronap = true;
          delay(1000);
          break;
        case 3:
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Safe Mode:Keypad");
          lcd.setCursor(0, 1);
          lcd.print("Entered: ***");
          numeronap = true;
          delay(1000);
          break;
        case 4:
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Safe Mode:Keypad");
          lcd.setCursor(0, 1);
          lcd.print("Entered: ****");
          delay(2000);
          numeronap = true;
          if (!(strncmp(password, initial_password, 4))) {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Safe Mode: Open      ");
            lcd.setCursor(0, 1);
            lcd.print("Accessing UI        ");
            sg90.write(100);  //Servomoottorilukkoon.
            delay(5000);
            lcd.clear();
            painallustenMaara = 0;
            numeronap = false;
            onAuki = true;  //Tämä lopettaa tämän lohkon pyörittämisen, koska auki oleminen toimii ehtona sille että voidaan avata "uudelleen". Täytyy varmistua sulkemisesta ensin.
            RFIDMode = true;
            break;
          }
        case 5:  // Jos viisinumeroinen koodi on syötetty mutta se on väärä.
          int length = sizeof(noteDurations2) / sizeof(int);
          buzzer.playMelody(melody2, noteDurations2, length);  // playing
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Wrong PIN !");
          lcd.setCursor(0, 1);
          lcd.print("Switching mode.");
          painallustenMaara = 0;
          delay(2000);
          RFIDMode = true;
          numeronap = false;
          onAuki = false;  //Tämä lopettaa tämän lohkon pyörittämisen, koska auki oleminen toimii ehtona sille että voidaan avata "uudelleen". Täytyy varmistua sulkemisestac ensin.
          break;
      }
      return;
    }
  }
}

void RFIDlukija() {
  if (!mfrc522.PICC_IsNewCardPresent()) { // Tarkista, onko uusi tagi luettavissa
    return;
  }
  if (!mfrc522.PICC_ReadCardSerial()) { // ja lue sen tunnistenumero
    return; // Paluu, jos ei ole uutta tagia tai sen tunnistenumeroa ei voi lukea
  }
  String tag = ""; // Luotu tyhjä merkkijono tallentamaan tagin tunnistenumero
  Serial.print("Reading UID: "); // Käy läpi tagin tunnistenumeron
  for (byte e = 0; e < mfrc522.uid.size; e++) {  // Käy läpi tagin tunnistenumeron
    Serial.print(mfrc522.uid.uidByte[e] < 0x10 ? " 0" : " "); //Printataan monitorille luettu tagi
    Serial.print(mfrc522.uid.uidByte[e], HEX); //joka esiintyy samassa muodossa kuin muistiin talletettaessa
    tag.concat(String(mfrc522.uid.uidByte[e] < 0x10 ? " 0" : " "));  // Tarkista, onko hexadesimaaliluku pienempi kuin 16 ja lisää sopiva etumerkki
    tag.concat(String(mfrc522.uid.uidByte[e], HEX));  // Lisää tunnistenumeron hexadesimaaliluku tag-merkkijonoon;
  }
  tag.toUpperCase();  // Muuta kaikki tagin arvot isoiksi kirjaimiksi
  //Aloitetaan luetun tagin vertailu.
  EEPROM.get(1, tagUID2);
if (tag.substring(1) == tagUID || tag.substring(1) == tagUID2) { 
    // Jos luettu tagi vastaa määriteltyjä tagUID tai tagUID2, avaa lukko ja näytä viesti
    Serial.print("   - Success!");
    if (tag.substring(1) == tagUID) {
        tagUIDname.toCharArray(viimeisinKayttaja, 6);
    } else {
        tagUID2name.toCharArray(viimeisinKayttaja, 6);
    }
    aika.toCharArray(viimeisinKayttoAika, 12);
    EEPROM.put(0, viimeisinKayttaja); // EEPROM
    EEPROM.put(2, viimeisinKayttoAika); // -muistiin.
    Serial.println();
    Serial.println(viimeisinKayttoAika);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Auth. success    ");
    lcd.setCursor(0, 1);
    lcd.print("Unlocking safe       ");
    sg90.write(100);  
    delay(2000);      
    lcd.clear();      
    onAuki = true;   
    return;
}   else {  //ENT. if (tag.substring(1) != tagUID || tag.substring(1) != tagUID2 ||) Jos tagi ei täsmännyt.
    int length = sizeof(noteDurations2) / sizeof(int);
    buzzer.playMelody(melody2, noteDurations2, length);  // playing
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Unknown tag!");
    Serial.print("   - Failure!");
    lcd.setCursor(0, 1);
    lcd.print("Enter a valid tag");
    tagUID2 = EEPROM.get(1, tagUID2);
    Serial.println("\nAvailable tags:");
    Serial.println("Admin " + tagUID);
    Serial.println("Slot1 " + tagUID2);
    Serial.println(tagUID2.length());
    Serial.println(tagUID.length());
    delay(3000);
    lcd.clear(); 
  }
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

void KayttoLiittyma() {
  char keypressed = keypad_key.getKey();
  if (keypressed == '3') {  // Jos näppäimistöllä painetaan numeroa 2, vaihda muuttuja onJoulu joko todeksi tai epätodeksi, riippuen sen nykyisestä tilasta.
    if (displayTime == false) {
      displayTime = true;
      browse = false; //Browse tulee LCD-näytön alarivillä tielle.
    } else {
      displayTime = false;
      browse = true;
    }
    return;
  }
  if (keypressed == '*') {  // Jos näppäimistöllä painetaan numeroa 2, vaihda muuttuja onJoulu joko todeksi tai epätodeksi, riippuen sen nykyisestä tilasta.
    if (browse == false) {
      browse = true;
      displayTime = false;
    } else {
      browse = false;
    }
    return;
  }
  if (keypressed == '2') {  // Jos näppäimistöllä painetaan numeroa 2, vaihda muuttuja onJoulu joko todeksi tai epätodeksi, riippuen sen nykyisestä tilasta.
    if (jouluMusic == true) {
      jouluMusic = false;
    } else {
      jouluMusic = true;
    }
    return;
  }
  if (keypressed == '#') {
    int length = sizeof(noteDurations2) / sizeof(int);
    buzzer.playMelody(melody2, noteDurations2, length);
    digitalWrite(7, 1);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Please close the");
    lcd.setCursor(0, 1);
    lcd.print("fold with care         ");
    delay(2000);
    lcd.setCursor(0, 0);
    lcd.print("Closing in 4 s. ");
    delay(1000);
    lcd.setCursor(0, 1);
    lcd.print("Clear the fold         ");
    lcd.setCursor(0, 0);
    lcd.print("Closing in 3 s. ");
    delay(1000);
    lcd.setCursor(0, 0);
    lcd.print("Closing in 2 s. ");
    delay(1000);
    lcd.setCursor(0, 0);
    lcd.print("Closing in 1 s. ");
    delay(1000);
    sg90.write(0);
    lcd.clear();
    onAuki = false;
    numeronap = false;
    return;
  }
  if (keypressed == '1') {  // Jos näppäimistöllä painetaan numeroa 1, aloita RFID-avaimen "authorisointi" eli alustaminen käyttöön.
    Serial.println();
    Serial.print("\nCurrent user is:  ");
    Serial.print(viimeisinKayttaja);
    Serial.print("\nwho opened the safe at: ");
    Serial.print(viimeisinKayttoAika);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Please read UID");
    lcd.setCursor(0, 1);
    lcd.print("you want to authorize       ");
    delay(7000);
    onAuki = true;
    numeronap = true;
    slotassingmentmode = true;
    int counter = 0;
    while (counter < 1) {                                                    
      if (mfrc522.PICC_IsNewCardPresent()) {                                
        if (mfrc522.PICC_ReadCardSerial()) {                                 
          String tag = "";                                                  
          for (byte e = 0; e < mfrc522.uid.size; e++) {                      
            tag.concat(String(mfrc522.uid.uidByte[e] < 0x10 ? " 0" : " "));  
            tag.concat(String(mfrc522.uid.uidByte[e], HEX));
          }
          tag.toUpperCase();  
          // Serial.print(tag);  // Printataan varmuudeksi alustettu tägi.
          tagUID2 = tag;
        } else {
          Serial.println("Failed to read NUID from tag."); //Tämä else-ehto täytyy automaattisesti, jos ylläolevat ehdot RFID-tagin lukufunktioista eivät täyty. Yleensä n. 5-7 sekunttia jos ei lue tagia.
          return;
        }
      }
      counter++; //Ei tehdä alustettavan tagin arvon lukeminen kuin vaan kerran.
    }
    Serial.println("\nReading tag:");
    tagUID2.trim();
    Serial.println(tagUID2);
    EEPROM.put(1, tagUID2); 
    String tagUID2test = EEPROM.get(1, tagUID2);
    Serial.println("tagUID2: "+ tagUID2test);
    lcd.setCursor(0, 0);
    lcd.print("Key authorized ");
    lcd.setCursor(0, 1);
    lcd.print("Locking safe...            ");
    delay(2000);
    lcd.setCursor(0, 1);
    lcd.print("Clear the fold         ");
    lcd.print("Closing in 3 s. ");
    delay(1000); 
    lcd.setCursor(0, 0);
    lcd.print("Closing in 2 s. ");
    delay(1000);  
    lcd.setCursor(0, 0);
    lcd.print("Closing in 1 s. ");
    delay(1000); 
    lcd.clear();
    sg90.write(0); 
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    onAuki = false;
    numeronap = false;
    slotassingmentmode = false; //Tämä pitää muistaa laittaa pois päältä, muuten mikään tila ei toimi.
  }
}

void ajastin() { //Yksinkertainen yksinomaan millis()-funktiolla toimiva timeri. ISR timeri oli mahdoton tehdä koska kaikki timerit olivat kaapattu muiden kirjastojen toimesta, vaikka oltaisiin osattu.
  static unsigned long aikaEdellisestaSekunnista = 0;
  static int sekunnit = 0;
  static int minuutit = 0;
  static int tunnit = 0;

  unsigned long nykyinenAika = millis();

  if (nykyinenAika - aikaEdellisestaSekunnista >= 1000) {
    sekunnit++;
    if (sekunnit == 60) {
      sekunnit = 0;
      minuutit++;
      if (minuutit == 60) {
        minuutit = 0;
        tunnit++;
        if (tunnit == 24) {
          tunnit = 0;
        }
      }
    }
    aikaEdellisestaSekunnista = nykyinenAika;
  }
  aika = String(tunnit) + ":" + String(minuutit) + ":" + String(sekunnit); //Ajan muuttujat formatoitu yhdeksi stringiksi.
}
