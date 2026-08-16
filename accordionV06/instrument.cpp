#include "instrument.h"

// L'ordre de la liste d'initialisation suit l'ordre de declaration des membres.
Instrument::Instrument()
    : servoController(),
      bellowController(servoController),
      leftHand(servoController, LEFT_HAND_MAPPING, NUM_NOTES_LEFT),
      rightHand(servoController, RIGHT_HAND_MAPPING, NUM_NOTES_RIGHT),
      state(SYS_BOOT), instrumentFault(INST_FAULT_NONE),
      noteSequence(0),
      attackHand(NULL), attackIndex(0), lastAttackTime(0), attackActive(false),
      sustainActive(false), bellowIdle(false), lastActivityTime(0) {}

byte Instrument::getActiveNoteCount() const {
    // Derive des deux mains plutot que maintenu a la main : aucun risque de desynchronisation
    // entre le compteur et l'etat reel des valves.
    return leftHand.getActiveNoteCount() + rightHand.getActiveNoteCount();
}

// === INITIALISATION ===
void Instrument::begin() {
    state = SYS_SERVO_INIT;

    // Un PCA9685 absent signifie des anches muettes, voire une valve generale inoperante
    // alors que le soufflet, lui, fonctionnerait : on ne demarre pas.
    if (!servoController.begin()) {
        // Le soufflet n'est pas encore initialise : on met le driver hors tension
        // directement, sans passer par BellowController.
        pinMode(STEPPER_EN_PIN, OUTPUT);
        digitalWrite(STEPPER_EN_PIN, HIGH);
        enterFault(INST_FAULT_PCA_MISSING);
        #if DEBUG
        Serial.print(F("[DEBUG] PCA manquants, masque: "));
        Serial.println((int)servoController.getMissingMask());
        #endif
        return;
    }

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

// === ATTAQUE ===
// Supplement de debit applique uniquement a la derniere note declenchee. Une note jouee
// fort au milieu d'un accord ne booste donc plus l'accord entier, et une note jouee
// doucement ne fait plus chuter le debit des autres.
float Instrument::attackBonus() const {
    if (!attackActive || attackHand == NULL) return 0.0f;

    byte velocity = attackHand->velocityOfIndex(attackIndex);
    float v = velocity / 127.0f;
    return attackHand->airFlowOfIndex(attackIndex) * velocitySustainWeight(velocity) *
           VELOCITY_ATTACK_BOOST * v;
}

void Instrument::clearAttack() {
    attackActive = false;
    attackHand = NULL;
}

// === DEMANDE D'AIR ===
void Instrument::refreshAirDemand() {
    float demand = leftHand.weightedAirFlow() + rightHand.weightedAirFlow() + attackBonus();
    bellowController.setAirDemand(demand);
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
                        (leftPriority == rightPriority && seqIsOlder(leftSeq, rightSeq));
        victim = takeLeft ? &leftHand : &rightHand;
        index = takeLeft ? leftIndex : rightIndex;
    } else if (hasLeft) {
        victim = &leftHand;
        index = leftIndex;
    } else {
        victim = &rightHand;
        index = rightIndex;
    }

    // Si la voix volee est celle en cours d'attaque, l'attaque n'a plus d'objet.
    if (attackActive && attackHand == victim && attackIndex == index) clearAttack();

    if (victim->releaseIndex(index) <= 0.0f) return false;

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

    int8_t index = hand->indexOf(note);
    if (index < 0) return; // Note absente du mapping de cette main

    bool wasActive = hand->isNoteActive(note);

    // Limite de notes simultanees : on tente de liberer une voix moins prioritaire.
    // (Une note deja ouverte ne consomme pas de voix supplementaire.)
    if (!wasActive && getActiveNoteCount() >= MAX_SIMULTANEOUS_NOTES) {
        if (!stealVoice(hand->priorityOf(note))) {
            #if DEBUG
            Serial.println(F("[DEBUG] Limite de notes atteinte, note ignoree"));
            #endif
            return;
        }
    }

    bool firstNote = (getActiveNoteCount() == 0);
    hand->noteOn(note, velocity, ++noteSequence);

    // Ferme la valve si c'est la premiere note (creation de la pression)
    if (firstNote) bellowController.closeValve();

    bellowIdle = false;
    attackHand = hand;
    attackIndex = (byte)index;
    lastAttackTime = millis();
    attackActive = true;

    refreshAirDemand();

    #if DEBUG
    Serial.print(F("[DEBUG] NoteOn: "));
    Serial.print(note);
    Serial.print(F(" ch="));
    Serial.print(channel);
    Serial.print(F(" active="));
    Serial.println(getActiveNoteCount());
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

    int8_t index = hand->indexOf(note);
    if (index >= 0 && attackActive && attackHand == hand && attackIndex == (byte)index) {
        clearAttack();
    }

    if (hand->noteOff(note) <= 0.0f) return;

    refreshAirDemand();

    #if DEBUG
    Serial.print(F("[DEBUG] NoteOff: "));
    Serial.print(note);
    Serial.print(F(" active="));
    Serial.println(getActiveNoteCount());
    #endif
}

// === PEDALE DE SUSTAIN (CC64) ===
void Instrument::setSustain(bool active) {
    if (sustainActive == active) return;
    sustainActive = active;

    if (active) return; // Appui : rien a faire, les NoteOff seront differees

    // Relachement : on ferme reellement toutes les notes retenues par la pedale.
    byte releasedLeft = 0, releasedRight = 0;
    leftHand.releaseSustained(releasedLeft);
    rightHand.releaseSustained(releasedRight);

    byte released = releasedLeft + releasedRight;
    if (released == 0) return;

    if (getActiveNoteCount() == 0) clearAttack();
    refreshAirDemand();

    #if DEBUG
    Serial.print(F("[DEBUG] Sustain relache, notes fermees: "));
    Serial.println(released);
    #endif
}

// === DESACTIVE TOUTES LES NOTES (MIDI PANIC) ===
void Instrument::allNotesOff() {
    leftHand.allNotesOff();
    rightHand.allNotesOff();

    clearAttack();
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
void Instrument::enterFault(InstrumentFault reason) {
    if (state == SYS_FAULT) return; // Premier defaut conserve

    leftHand.allNotesOff();
    rightHand.allNotesOff();

    clearAttack();
    sustainActive = false;
    lastActivityTime = millis();
    instrumentFault = reason;
    state = SYS_FAULT;

    // Le soufflet doit aussi se mettre en securite si le defaut vient d'ailleurs.
    if (!bellowController.hasFault() && reason != INST_FAULT_BELLOW) {
        bellowController.stopAndDisable();
        bellowController.openValve();
    }

    #if DEBUG
    Serial.print(F("[DEBUG] Instrument en defaut, cause: "));
    Serial.println((int)reason);
    #endif
}

// === MET A JOUR L'INSTRUMENT ===
void Instrument::update() {
    bellowController.update();

    // Transitions d'etat
    if (bellowController.hasFault()) {
        enterFault(INST_FAULT_BELLOW);
    } else if (servoController.hasBusFailure()) {
        // Un PCA qui cesse de repondre en cours de jeu laisserait des anches ouvertes.
        enterFault(INST_FAULT_PCA_BUS);
    } else if (state == SYS_HOMING && bellowController.isReady()) {
        state = SYS_READY;
        lastActivityTime = millis();
        #if DEBUG
        Serial.println(F("[DEBUG] Instrument pret"));
        #endif
    }

    // Fin de la phase d'attaque : on repasse au niveau tenu.
    if (attackActive && (millis() - lastAttackTime) >= VELOCITY_ATTACK_MS) {
        clearAttack();
        refreshAirDemand();
    }

    // Horodatage de la derniere activite : mis a jour tant qu'une note est ouverte, ce qui
    // evite d'avoir a detecter la transition vers zero.
    if (getActiveNoteCount() > 0) lastActivityTime = millis();

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
    if (getActiveNoteCount() != 0) return;

    if ((millis() - servoController.getLastCommandTime()) > PCA_DISABLE_DELAY) {
        servoController.enableServos(false);
    }
}

// === GESTION DE L'INACTIVITE DU SOUFFLET ===
void Instrument::manageInactivity() {
    // Jamais pendant le homing : la calibration peut durer plusieurs dizaines de secondes
    // et couper le driver a ce moment-la bloquait definitivement le soufflet.
    if (state != SYS_READY) return;
    if (getActiveNoteCount() != 0 || bellowIdle) return;

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
