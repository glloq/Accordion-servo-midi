#include "handController.h"

// === CONSTRUCTEUR ===
HandController::HandController(ServoController &servoCtrl, const NoteConfig *mapping, byte numNotes)
    : servoController(servoCtrl), mapping(mapping), numNotes(numNotes), activeCount(0) {
    // Initialise tous les etats a inactif
    for (byte i = 0; i < MAX_NOTES_PER_HAND; i++) {
        noteStates[i] = false;
        sustainedNotes[i] = false;
        noteSeq[i] = 0;
        noteVelocity[i] = 0;
    }
}

// === VERIFICATION DE LA NOTE ===
bool HandController::canPlay(byte note) const {
    return findNoteIndex(mapping, numNotes, note) >= 0;
}

// === VERIFIE SI UNE NOTE EST ACTIVE ===
bool HandController::isNoteActive(byte note) const {
    int8_t index = findNoteIndex(mapping, numNotes, note);
    if (index < 0) return false;
    return noteStates[index];
}

// === PRIORITE DE VOIX D'UNE NOTE ===
byte HandController::priorityOf(byte note) const {
    int8_t index = findNoteIndex(mapping, numNotes, note);
    if (index < 0) return 0;
    return notePriorityAt(mapping, (uint8_t)index);
}

// === ACTIVATION D'UNE NOTE ===
float HandController::noteOn(byte note, byte velocity, uint16_t seq) {
    int8_t index = findNoteIndex(mapping, numNotes, note);
    if (index < 0) return 0.0f;

    // Une note re-declenchee alors qu'elle etait retenue par la pedale redevient
    // simplement une note tenue par le clavier : pas de double comptage du debit d'air.
    if (noteStates[index]) {
        sustainedNotes[index] = false;
        // Rafraichir l'anciennete : sans cela une note tout juste rejouee restait la plus
        // ancienne du tableau et pouvait etre volee immediatement apres.
        noteSeq[index] = seq;
        noteVelocity[index] = velocity;
        return 0.0f;
    }

    NoteConfig config;
    loadNoteConfig(mapping, (uint8_t)index, config);
    servoController.setServoAngle(config.pcaAddress, config.channel, openAngleFor(config));

    noteStates[index] = true;
    sustainedNotes[index] = false;
    noteSeq[index] = seq;
    noteVelocity[index] = velocity;
    activeCount++;

    return config.airFlowMultiplier;
}

// === DEMANDE D'AIR PONDEREE PAR LA VELOCITE ===
float HandController::weightedAirFlow() const {
    float sum = 0.0f;
    for (byte i = 0; i < numNotes; i++) {
        if (!noteStates[i]) continue;
        sum += noteAirFlowAt(mapping, i) * velocitySustainWeight(noteVelocity[i]);
    }
    return sum;
}

float HandController::airFlowOfIndex(byte index) const {
    if (index >= numNotes) return 0.0f;
    return noteAirFlowAt(mapping, index);
}

byte HandController::velocityOfIndex(byte index) const {
    if (index >= numNotes) return 0;
    return noteVelocity[index];
}

// === FERMETURE INTERNE D'UNE VALVE ===
void HandController::closeIndex(byte index) {
    NoteConfig config;
    loadNoteConfig(mapping, index, config);
    servoController.setServoAngle(config.pcaAddress, config.channel, config.closedPosition);
    noteStates[index] = false;
    sustainedNotes[index] = false;
    if (activeCount > 0) activeCount--;
}

// === DESACTIVATION D'UNE NOTE ===
float HandController::noteOff(byte note) {
    int8_t index = findNoteIndex(mapping, numNotes, note);
    if (index < 0) return 0.0f;
    if (!noteStates[index]) return 0.0f;

    float airFlow = noteAirFlowAt(mapping, (uint8_t)index);
    closeIndex((byte)index);
    return airFlow;
}

// === FERMETURE PAR INDEX (VOL DE VOIX) ===
float HandController::releaseIndex(byte index) {
    if (index >= numNotes || !noteStates[index]) return 0.0f;

    float airFlow = noteAirFlowAt(mapping, index);
    closeIndex(index);
    return airFlow;
}

// === SUSTAIN : DIFFERE LA FERMETURE ===
void HandController::markSustained(byte note) {
    int8_t index = findNoteIndex(mapping, numNotes, note);
    if (index < 0 || !noteStates[index]) return;
    sustainedNotes[index] = true;
}

// === SUSTAIN : RELACHE LES NOTES RETENUES PAR LA PEDALE ===
float HandController::releaseSustained(byte &releasedCount) {
    float airFlow = 0.0f;
    releasedCount = 0;

    for (byte i = 0; i < numNotes; i++) {
        if (noteStates[i] && sustainedNotes[i]) {
            airFlow += noteAirFlowAt(mapping, i);
            closeIndex(i);
            releasedCount++;
        }
    }
    return airFlow;
}

// === RECHERCHE D'UNE VOIX A VOLER ===
bool HandController::findStealCandidate(byte maxPriority, byte &outIndex, byte &outPriority,
                                        uint16_t &outSeq) const {
    bool found = false;

    for (byte i = 0; i < numNotes; i++) {
        if (!noteStates[i]) continue;

        byte priority = notePriorityAt(mapping, i);
        if (priority >= maxPriority) continue; // Jamais voler une voix aussi importante

        // Une note deja relachee au clavier et seulement retenue par la pedale est la
        // premiere sacrifiee : on la traite comme la priorite la plus basse possible.
        byte effective = sustainedNotes[i] ? 0 : priority;

        if (!found || effective < outPriority ||
            (effective == outPriority && seqIsOlder(noteSeq[i], outSeq))) {
            outIndex = i;
            outPriority = effective;
            outSeq = noteSeq[i];
            found = true;
        }
    }
    return found;
}

// === DESACTIVE TOUTES LES NOTES (MIDI PANIC) ===
void HandController::allNotesOff() {
    for (byte i = 0; i < numNotes; i++) {
        if (noteStates[i]) {
            NoteConfig config;
            loadNoteConfig(mapping, i, config);
            servoController.setServoAngle(config.pcaAddress, config.channel, config.closedPosition);
        }
        noteStates[i] = false;
        sustainedNotes[i] = false;
    }
    activeCount = 0;
}

// === FERME TOUS LES SERVOS (POSITION INITIALE) ===
// Echelonne les commandes : refermer des dizaines de servos au meme instant provoque un
// appel de courant que l'alimentation 5V/10A ne peut pas encaisser.
void HandController::closeAllServos() {
    for (byte i = 0; i < numNotes; i++) {
        NoteConfig config;
        loadNoteConfig(mapping, i, config);
        servoController.setServoAngle(config.pcaAddress, config.channel, config.closedPosition);
        noteStates[i] = false;
        sustainedNotes[i] = false;
        delay(SERVO_INIT_STAGGER_MS);
    }
    activeCount = 0;
}
