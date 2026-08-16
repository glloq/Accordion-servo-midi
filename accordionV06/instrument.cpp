#include "instrument.h"

// L'ordre de la liste d'initialisation suit l'ordre de declaration des membres.
Instrument::Instrument()
    : servoController(),
      bellowController(servoController),
      leftHand(servoController, LEFT_HAND_MAPPING, NUM_NOTES_LEFT),
      rightHand(servoController, RIGHT_HAND_MAPPING, NUM_NOTES_RIGHT),
      state(SYS_BOOT),
      totalAirFlow(0.0f), activeNotes(0), noteSequence(0),
      lastVelocity(100), lastAttackTime(0), attackActive(false),
      sustainActive(false), bellowIdle(false), lastActivityTime(0) {}

// === INITIALISATION ===
void Instrument::begin() {
    state = SYS_SERVO_INIT;
    servoController.begin();   // Initialise les PCA9685

    // Ferme tous les servos au demarrage (position initiale sure), de maniere echelonnee
    // pour ne pas solliciter les 59 servos en meme temps.
    leftHand.closeAllServos();
    rightHand.closeAllServos();

    #if DEBUG
    Serial.println(F("[DEBUG] Instrument: servos initialises"));
    #endif

    state = SYS_HOMING;
    bellowController.begin();  // Initialise le moteur pas a pas et lance le homing

    lastActivityTime = millis();

    #if DEBUG
    Serial.println(F("[DEBUG] Instrument: homing en cours"));
    #endif
}

// === SELECTION DE LA MAIN ===
HandController *Instrument::handForChannel(byte channel) {
    if (channel == MIDI_CHANNEL_LEFT) return &leftHand;
    if (channel == MIDI_CHANNEL_RIGHT) return &rightHand;
    return NULL;
}

// === FACTEUR DE DYNAMIQUE ===
// La velocite ne peut pas agir sur les servos (une valve est ouverte ou fermee). Elle agit
// donc sur la demande d'air : une attaque breve plus forte, puis un niveau tenu.
float Instrument::velocityFactor() const {
    if (activeNotes == 0) return 1.0f;

    float v = lastVelocity / 127.0f;
    float sustainLevel = VELOCITY_SUSTAIN_MIN + (1.0f - VELOCITY_SUSTAIN_MIN) * v;

    if (attackActive) {
        return sustainLevel * (1.0f + VELOCITY_ATTACK_BOOST * v);
    }
    return sustainLevel;
}

void Instrument::refreshAirDemand() {
    bellowController.setAirDemand(totalAirFlow, velocityFactor());
}

// === RETRAIT D'AIR (note relachee ou volee) ===
void Instrument::removeAir(float airFlow, byte noteCount) {
    totalAirFlow -= airFlow;
    if (totalAirFlow < 0.0f) totalAirFlow = 0.0f;

    if (activeNotes >= noteCount) activeNotes -= noteCount;
    else activeNotes = 0;

    if (activeNotes == 0) {
        lastActivityTime = millis();
        attackActive = false;
    }
    refreshAirDemand();
}

// === VOL DE VOIX ===
// Quand la limite de notes simultanees est atteinte, une note entrante ne peut prendre la
// place que d'une note STRICTEMENT moins prioritaire (accord < melodie < basse), la plus
// ancienne d'abord. Les notes retenues par la seule pedale de sustain partent en premier.
// Si aucune candidate n'existe, la note entrante est ignoree.
bool Instrument::stealVoice(byte incomingPriority) {
    byte leftIndex = 0, leftPriority = 0;
    byte rightIndex = 0, rightPriority = 0;
    uint16_t leftSeq = 0, rightSeq = 0;

    bool hasLeft = leftHand.findStealCandidate(incomingPriority, leftIndex, leftPriority, leftSeq);
    bool hasRight = rightHand.findStealCandidate(incomingPriority, rightIndex, rightPriority, rightSeq);

    if (!hasLeft && !hasRight) return false;

    HandController *victim;
    byte index;

    if (hasLeft && hasRight) {
        bool takeLeft = (leftPriority < rightPriority) ||
                        (leftPriority == rightPriority && leftSeq < rightSeq);
        victim = takeLeft ? &leftHand : &rightHand;
        index = takeLeft ? leftIndex : rightIndex;
    } else if (hasLeft) {
        victim = &leftHand;
        index = leftIndex;
    } else {
        victim = &rightHand;
        index = rightIndex;
    }

    float airFlow = victim->releaseIndex(index);
    if (airFlow <= 0.0f) return false;

    totalAirFlow -= airFlow;
    if (totalAirFlow < 0.0f) totalAirFlow = 0.0f;
    if (activeNotes > 0) activeNotes--;

    #if DEBUG
    Serial.println(F("[DEBUG] Vol de voix"));
    #endif
    return true;
}

// === ACTIVATION D'UNE NOTE ===
void Instrument::noteOn(byte note, byte velocity, byte channel) {
    // Aucune note tant que l'instrument n'est pas pret (init servos, homing, defaut).
    if (state != SYS_READY) {
        #if DEBUG
        Serial.println(F("[DEBUG] NoteOn refusee: instrument non pret"));
        #endif
        return;
    }

    HandController *hand = handForChannel(channel);
    if (hand == NULL) {
        #if DEBUG
        Serial.print(F("[DEBUG] Canal MIDI ignore: "));
        Serial.println(channel);
        #endif
        return;
    }

    if (!hand->canPlay(note)) return;

    // Note deja ouverte : on relance seulement l'attaque et on annule un eventuel
    // maintien par la pedale, sans recompter le debit d'air.
    if (hand->isNoteActive(note)) {
        hand->noteOn(note, ++noteSequence);
        lastVelocity = velocity;
        lastAttackTime = millis();
        attackActive = true;
        refreshAirDemand();
        return;
    }

    // Limite de notes simultanees : on tente de liberer une voix moins prioritaire.
    if (activeNotes >= MAX_SIMULTANEOUS_NOTES) {
        if (!stealVoice(hand->priorityOf(note))) {
            #if DEBUG
            Serial.println(F("[DEBUG] Limite de notes atteinte, note ignoree"));
            #endif
            return;
        }
    }

    float airFlow = hand->noteOn(note, ++noteSequence);
    if (airFlow <= 0.0f) return;

    // Ferme la valve si c'est la premiere note (creation de la pression)
    if (activeNotes == 0) {
        bellowController.closeValve();
    }

    totalAirFlow += airFlow;
    activeNotes++;
    bellowIdle = false;

    lastVelocity = velocity;
    lastAttackTime = millis();
    attackActive = true;

    refreshAirDemand();

    #if DEBUG
    Serial.print(F("[DEBUG] NoteOn: "));
    Serial.print(note);
    Serial.print(F(" ch="));
    Serial.print(channel);
    Serial.print(F(" active="));
    Serial.println(activeNotes);
    #endif
}

// === DESACTIVATION D'UNE NOTE ===
void Instrument::noteOff(byte note, byte channel) {
    HandController *hand = handForChannel(channel);
    if (hand == NULL) return;

    // Sustain actif : la note reste ouverte mais est marquee comme relachee au clavier.
    // Elle sera fermee au relachement de la pedale (CC64 < 64).
    if (sustainActive && hand->isNoteActive(note)) {
        hand->markSustained(note);
        return;
    }

    float airFlow = hand->noteOff(note);
    if (airFlow <= 0.0f) return;

    removeAir(airFlow, 1);

    #if DEBUG
    Serial.print(F("[DEBUG] NoteOff: "));
    Serial.print(note);
    Serial.print(F(" active="));
    Serial.println(activeNotes);
    #endif
}

// === PEDALE DE SUSTAIN (CC64) ===
void Instrument::setSustain(bool active) {
    if (sustainActive == active) return;
    sustainActive = active;

    if (active) return; // Appui : rien a faire, les NoteOff seront differees

    // Relachement : on ferme reellement toutes les notes retenues par la pedale.
    byte releasedLeft = 0, releasedRight = 0;
    float airFlow = leftHand.releaseSustained(releasedLeft);
    airFlow += rightHand.releaseSustained(releasedRight);

    byte released = releasedLeft + releasedRight;
    if (released == 0) return;

    removeAir(airFlow, released);

    #if DEBUG
    Serial.print(F("[DEBUG] Sustain relache, notes fermees: "));
    Serial.println(released);
    #endif
}

// === DESACTIVE TOUTES LES NOTES (MIDI PANIC) ===
void Instrument::allNotesOff() {
    leftHand.allNotesOff();
    rightHand.allNotesOff();

    totalAirFlow = 0.0f;
    activeNotes = 0;
    attackActive = false;
    sustainActive = false;
    lastActivityTime = millis();

    refreshAirDemand();
    bellowController.openValve();
    bellowIdle = false;

    #if DEBUG
    Serial.println(F("[DEBUG] All Notes Off - MIDI Panic"));
    #endif
}

// === PASSAGE EN DEFAUT ===
void Instrument::enterFault() {
    leftHand.allNotesOff();
    rightHand.allNotesOff();

    totalAirFlow = 0.0f;
    activeNotes = 0;
    attackActive = false;
    sustainActive = false;
    lastActivityTime = millis();
    state = SYS_FAULT;

    #if DEBUG
    Serial.println(F("[DEBUG] Instrument en defaut"));
    #endif
}

// === MET A JOUR L'INSTRUMENT ===
void Instrument::update() {
    bellowController.update();

    // Transitions d'etat
    if (bellowController.hasFault()) {
        if (state != SYS_FAULT) enterFault();
    } else if (state == SYS_HOMING && bellowController.isReady()) {
        state = SYS_READY;
        lastActivityTime = millis();
        #if DEBUG
        Serial.println(F("[DEBUG] Instrument pret"));
        #endif
    }

    // Fin de la phase d'attaque : on repasse au niveau tenu.
    if (attackActive && (millis() - lastAttackTime) >= VELOCITY_ATTACK_MS) {
        attackActive = false;
        refreshAirDemand();
    }

    managePCA();        // Gere la coupure de l'OE des PCA
    manageInactivity(); // Gere l'inactivite du soufflet
}

// === GESTION DE LA DESACTIVATION DES PCA ===
// L'OE est partage par les 4 PCA9685, valve generale comprise. On ne le coupe donc que :
//  - a l'etat READY uniquement (pendant le homing la valve doit rester ouverte, et en
//    defaut elle doit rester ouverte pour liberer la pression),
//  - et PCA_DISABLE_DELAY apres la DERNIERE commande servo, pour laisser a la valve le
//    temps d'atteindre sa position.
void Instrument::managePCA() {
    if (state != SYS_READY) return;
    if (activeNotes != 0) return;

    if ((millis() - servoController.getLastCommandTime()) > PCA_DISABLE_DELAY) {
        servoController.enableServos(false);
    }
}

// === GESTION DE L'INACTIVITE DU SOUFFLET ===
void Instrument::manageInactivity() {
    // Jamais pendant le homing : la calibration peut durer plusieurs dizaines de secondes
    // et couper le driver a ce moment-la bloquait definitivement le soufflet.
    if (state != SYS_READY) return;
    if (activeNotes != 0 || bellowIdle) return;

    if ((millis() - lastActivityTime) > BELLOW_INACTIVITY_TIMEOUT) {
        bellowController.openValve();
        bellowController.stopAndDisable();
        bellowIdle = true; // Latch : evite de recommander la valve a chaque tour de boucle

        #if DEBUG
        Serial.println(F("[DEBUG] Inactivite - moteur desactive"));
        #endif
    }
}

// === GESTION DU VOLUME MIDI ===
void Instrument::setVolume(byte volume) {
    bellowController.setVolume(volume);

    #if DEBUG
    Serial.print(F("[DEBUG] Volume: "));
    Serial.println(volume);
    #endif
}

void Instrument::setExpression(byte expression) {
    bellowController.setExpression(expression);
}
