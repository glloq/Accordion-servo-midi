#include "instrument.h"

Instrument::Instrument()
    : totalAirFlow(0), activeNotes(0), lastActivityTime(millis()),
      leftHand(servoController, LEFT_HAND_MAPPING, FIRST_NOTE_LEFT, NUM_NOTES_LEFT),
      rightHand(servoController, RIGHT_HAND_MAPPING, FIRST_NOTE_RIGHT, NUM_NOTES_RIGHT),
      bellowController(servoController) {}

// === INITIALISATION ===
void Instrument::begin() {
    servoController.begin();   // Initialise les PCA9685

    // Ferme tous les servos au démarrage (position initiale sûre)
    leftHand.closeAllServos();
    rightHand.closeAllServos();

    #if DEBUG
    Serial.println(F("[DEBUG] Instrument: servos initialisés"));
    #endif

    bellowController.begin();  // Initialise le moteur pas à pas et calibre

    #if DEBUG
    Serial.println(F("[DEBUG] Instrument: initialisation terminée"));
    #endif
}

// === ACTIVATION D'UNE NOTE ===
void Instrument::noteOn(byte note, byte velocity, byte channel) {
    // Protection: limite le nombre de notes simultanées
    if (activeNotes >= MAX_SIMULTANEOUS_NOTES) {
        #if DEBUG
        Serial.println(F("[DEBUG] Limite de notes atteinte, note ignorée"));
        #endif
        return;
    }

    float airFlow = 0.0f;

    if (channel == MIDI_CHANNEL_LEFT) {
        if (leftHand.canPlay(note)) {
            airFlow = leftHand.noteOn(note, velocity);
        }
    } else if (channel == MIDI_CHANNEL_RIGHT) {
        if (rightHand.canPlay(note)) {
            airFlow = rightHand.noteOn(note, velocity);
        }
    }
    #if DEBUG
    else {
        Serial.print(F("[DEBUG] Canal MIDI ignoré: "));
        Serial.println(channel);
    }
    #endif

    // Ne traiter que si une note a réellement été activée
    if (airFlow > 0) {
        // Fermer la valve si c'est la première note (créer la pression)
        if (activeNotes == 0) {
            bellowController.closeValve();
        }

        totalAirFlow += airFlow;
        activeNotes++;
        bellowController.updateSpeed(totalAirFlow);

        #if DEBUG
        Serial.print(F("[DEBUG] NoteOn: "));
        Serial.print(note);
        Serial.print(F(" ch="));
        Serial.print(channel);
        Serial.print(F(" active="));
        Serial.println(activeNotes);
        #endif
    }
}

// === DÉSACTIVATION D'UNE NOTE ===
void Instrument::noteOff(byte note, byte channel) {
    float airFlow = 0.0f;

    if (channel == MIDI_CHANNEL_LEFT) {
        airFlow = leftHand.noteOff(note);
    } else if (channel == MIDI_CHANNEL_RIGHT) {
        airFlow = rightHand.noteOff(note);
    }

    if (airFlow > 0) {
        totalAirFlow -= airFlow;
        if (activeNotes > 0) activeNotes--;

        bellowController.updateSpeed(totalAirFlow);

        if (activeNotes == 0) {
            lastActivityTime = millis();
        }

        #if DEBUG
        Serial.print(F("[DEBUG] NoteOff: "));
        Serial.print(note);
        Serial.print(F(" active="));
        Serial.println(activeNotes);
        #endif
    }
}

// === DÉSACTIVE TOUTES LES NOTES (MIDI PANIC) ===
void Instrument::allNotesOff() {
    leftHand.allNotesOff();
    rightHand.allNotesOff();

    totalAirFlow = 0;
    activeNotes = 0;
    lastActivityTime = millis();

    bellowController.updateSpeed(0);
    bellowController.openValve();

    #if DEBUG
    Serial.println(F("[DEBUG] All Notes Off - MIDI Panic"));
    #endif
}

// === MET À JOUR L'INSTRUMENT ===
void Instrument::update() {
    bellowController.update();
    managePCA();        // Gère l'activation/désactivation des PCA
    manageInactivity(); // Gère l'inactivité du soufflet
}

// === GESTION DE LA DÉSACTIVATION DES PCA ===
void Instrument::managePCA() {
    if (activeNotes == 0 && millis() - lastActivityTime > PCA_DISABLE_DELAY) {
        servoController.enableServos(false);
    }
}

// === GESTION DU VOLUME MIDI ===
void Instrument::setVolume(byte volume) {
    bellowController.updateVolume(volume);

    #if DEBUG
    Serial.print(F("[DEBUG] Volume: "));
    Serial.println(volume);
    #endif
}

// === GESTION DE L'INACTIVITÉ DU SOUFFLET ===
void Instrument::manageInactivity() {
    if (activeNotes == 0 && millis() - lastActivityTime > BELLOW_INACTIVITY_TIMEOUT) {
        bellowController.openValve();
        bellowController.stopWithDecay();

        #if DEBUG
        Serial.println(F("[DEBUG] Inactivité - moteur désactivé"));
        #endif
    }
}
