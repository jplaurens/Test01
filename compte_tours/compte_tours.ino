/*
 * Compte-tours de 0 à 10000 tr/min, auteur Christophe JUILLET
 * 
 * Voir ici pour discussion et complément d'infos: http://forum.arduino.cc/index.php?topic=562866.0
 * 
 * Précision après calibrage: environ 0,01% +- 1 digit (sans calibrage environ 1%).
 * Environ 2 mesures par seconde, réglable, ou une mesure par tour en dessous de 120 tr/min.
 * Message d'erreur "EEEE" si le capteur loupe une impulsion. Améliore la fiabilité de la lecture, ne fonctionne pas sous 300 tr/min.
 * En cas de messages d'erreurs répétées, revoir le montage du capteur.
 * Le message d'erreur peut s'afficher pendant une accélération ou décélération brutale, ou vitesse erratique (par exemple en cas de freinage avec ABS, roue bloquée par moments).
 * 
 * nomenclature:
 * un Arduino
 * un afficheur 4 digits série avec TM1637 (facultatif si lecture en console)
 * un capteur au choix (hall, optique ou autre) générant une impulsion de 5v maxi, 3v mini, la mesure se fait sur le front montant ou descendant
 * 
 */

#include <TM1637.h> // menu IDE Arduino => croquis => inclure une bibliothèque => gérer les bibliothèques => filtrez votre recherche => TM1637 => Grove4-Digit Display => more info => installer. Voir ici pour infos: https://github.com/Seeed-Studio/Grove_4Digital_Display  

// variables de configuration:
#define LUM                   4 // luminosité de l'afficheur de 0 à 7
#define DELAI_MESURE        500 // délai minimal entre deux mesures en ms, 500 par défaut 
#define PIN_CAPTEUR           2 // la broche utilisée par le capteur, uniquement 2 ou 3!
#define CLK                   5 // broche d'horloge du TM1637 au choix
#define DIO                   4 // broche de données du TM1637 au choix
#define FREQUENCE_ARDUINO 16000 // fréquence de l'Arduino en KHz (voir calibrage ici: http://forum.arduino.cc/index.php?topic=561774.0 ) 16000 par défaut, si Arduino pro 8 MHz multipliez la fréquence par deux

const bool console = false;     // true pour un retour console
const bool deuxTours = false;   // true si une impulsion tous les deux tours (moteur 4 temps), sinon une impulsion par tour
// variables de configuration, fin

volatile uint32_t t2;
volatile bool nouvTour = false;
TM1637 tm1637(CLK,DIO);

void mesurer(void);

void setup() {
  pinMode(PIN_CAPTEUR, INPUT_PULLUP);
  if (console) Serial.begin(115200);
  tm1637.init();
  tm1637.set(LUM);
  attachInterrupt(digitalPinToInterrupt(PIN_CAPTEUR), mesurer, FALLING); // RISING front montant, FALLING front descendant
}

void loop() {
  static bool afficher = true;
  static bool erreurMesure = false;
  static uint32_t t1 = micros(), dt = 0, dtOld = 0, sommeT = 0; // t1 et t2: moment des deux dernières impulsions, dt: durée de la dernière impulsion, dtOld: durée de l'avant dernière impulsion, sommeT: durée de la somme des impulsions de la dernière mesure
  static int nbTours = 0; // nombre de tours à prendre en compte pour la dernière mesure
  int rpm;

  if (nouvTour == true) { nouvTour = false;
    nbTours++;
    dt = t2 - t1;
    t1 = t2;
    sommeT += dt;
    if ((dt > 1.8 * dtOld || dtOld > 1.8 * dt) && dt < 200000) erreurMesure = true; // en cas de loupé, afficher le message d'erreur "EEEE". désactivé en dessous de 300 tr/min
    dtOld = dt;
    if (sommeT > 1000L * DELAI_MESURE) { afficher = true; // si le temps écoulé depuis la dernière mesure dépasse 500ms actualiser l'affichage
      if (deuxTours) sommeT /= 2;
      rpm = 3750L * FREQUENCE_ARDUINO * nbTours / sommeT; // 3750 = 60 * 1000 / 16
      if (!erreurMesure) {
        if (console) Serial.println(rpm);
        tm1637.display(3, rpm%10);
        tm1637.display(2, (rpm/=10) > 0 ? rpm%10 : 0x10);
        tm1637.display(1, (rpm/=10) > 0 ? rpm%10 : 0x10);
        tm1637.display(0, (rpm/=10) > 0 ? rpm%10 : 0x10);
      } else { erreurMesure = false;
        if (console) Serial.println("Erreur de mesure");
        tm1637.display(0,0x0E);
        tm1637.display(1,0x0E);
        tm1637.display(2,0x0E);
        tm1637.display(3,0x0E);
      }
      sommeT = nbTours = 0;
    }
  }
  if (afficher && micros() - t1 > 1000000) { afficher = false; // après une seconde sans détection, afficher 0
    if (console) Serial.println(0);
    tm1637.display(0, 0x10);
    tm1637.display(1, 0x10);
    tm1637.display(2, 0x10);
    tm1637.display(3, 0);
  }
}

void mesurer() {
  t2 = micros();
  nouvTour = true;
}

