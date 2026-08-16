#ifndef FLEXY_STEPPER_STUB_H
#define FLEXY_STEPPER_STUB_H

// =========================================================================================
// Stub FlexyStepper FIDELE.
//
// La version precedente de ce stub deplacait la position de `speed * dt` a chaque appel :
// un `setTargetPosition(positionCourante)` arretait donc le moteur instantanement, ce qui
// masquait completement le probleme d'arret sur fin de course.
//
// Ce stub modelise les proprietes qui comptent :
//   - UN SEUL pas par appel a processMovement() (comme la vraie bibliotheque) ;
//   - la cadence des pas est gouvernee par micros(), donc par la periode de la boucle ;
//   - l'etat de direction PERSISTE : changer la cible n'arrete pas le moteur ;
//   - rampes d'acceleration et de deceleration, donc surcourse si la cible est deplacee
//     sur la position courante a pleine vitesse ;
//   - setTargetPositionToStop() vise la position d'arret a la deceleration courante ;
//   - la position PHYSIQUE ne bouge que si le driver est alimente (ENABLE bas), alors que
//     le compteur interne de la bibliotheque avance dans tous les cas. C'est exactement ce
//     qui impose de recaler la position sur le fin de course apres une coupure.
// =========================================================================================

#include <stdint.h>
#include <math.h>
#include <Arduino.h>

class FlexyStepper;
extern FlexyStepper *g_stepper;

// Position PHYSIQUE (mm) des contacts, dans le repere de la machine.
extern float stubMinSwitchPhys;
extern float stubMaxSwitchPhys;
// Position physique reelle du soufflet (mm). Independante du compteur de la bibliotheque.
extern float stubPhysicalPos;
// Compteurs d'instrumentation pour les tests de timing.
extern unsigned long stubStepCount;
// Nombre d'appels a setCurrentPosition() alors que le moteur etait EN MOUVEMENT.
// La vraie bibliotheque documente que cet appel n'est licite qu'a l'arret : ce compteur
// detecte donc directement une violation du contrat, sans heuristique cote test.
extern unsigned long stubIllegalPositionResets;
// Simule un cablage DIR inverse (ou des phases moteur permutees) : la bibliotheque croit
// descendre alors que la machine monte.
extern bool stubInvertPhysicalDirection;

bool stubDriverEnabled();

class FlexyStepper {
public:
    FlexyStepper()
        : stepsPerMM(1.0f), libSteps(0), targetSteps(0), direction(0),
          desiredRate(1.0f), accelRate(1.0f), currentRate(1.0f),
          lastStepTime(0), stopping(false) {
        g_stepper = this;
    }

    void connectToPins(uint8_t, uint8_t) {}

    void setStepsPerMillimeter(float s) { stepsPerMM = (s > 0.0f) ? s : 1.0f; }

    void setSpeedInMillimetersPerSecond(float v) {
        desiredRate = v * stepsPerMM;
        if (desiredRate < 1.0f) desiredRate = 1.0f;
    }

    void setAccelerationInMillimetersPerSecondPerSecond(float a) {
        accelRate = a * stepsPerMM;
        if (accelRate < 1.0f) accelRate = 1.0f;
    }

    void setTargetPositionInMillimeters(float p) { targetSteps = (long)lroundf(p * stepsPerMM); }

    // Semantique de la vraie bibliotheque : ne recale QUE le compteur interne.
    // La machine, elle, ne bouge pas. Appeler ceci en mouvement est une violation du
    // contrat de la bibliotheque : on la comptabilise.
    void setCurrentPositionInMillimeters(float p) {
        if (direction != 0) stubIllegalPositionResets++;
        libSteps = (long)lroundf(p * stepsPerMM);
    }

    float getCurrentPositionInMillimeters() { return (float)libSteps / stepsPerMM; }

    // Vise la position ou le moteur sera arrete en decelerant au taux courant.
    void setTargetPositionToStop() {
        if (direction == 0) { targetSteps = libSteps; return; }
        long brake = (long)((currentRate * currentRate) / (2.0f * accelRate));
        targetSteps = libSteps + (long)direction * brake;
        stopping = true;
    }

    bool motionComplete() { return direction == 0 && libSteps == targetSteps; }

    bool processMovement() {
        if (direction == 0) {
            if (libSteps == targetSteps) { stopping = false; return true; }
            direction = (targetSteps > libSteps) ? 1 : -1;
            currentRate = firstStepRate();
            lastStepTime = micros();
            return false;
        }

        unsigned long now = micros();
        unsigned long period = (unsigned long)(1000000.0f / currentRate);
        if (period == 0) period = 1;
        if ((unsigned long)(now - lastStepTime) < period) return false;
        lastStepTime = now;

        // --- UN seul pas ---
        libSteps += direction;
        stubStepCount++;
        // Le compteur de la bibliotheque avance meme driver coupe ; la machine, non.
        if (stubDriverEnabled()) {
            int physDir = stubInvertPhysicalDirection ? -direction : direction;
            stubPhysicalPos += (float)physDir / stepsPerMM;
        }

        long delta = targetSteps - libSteps;
        float dt = (float)period / 1000000.0f;

        // Doit-on freiner ?
        //  - arret demande, ou
        //  - on s'eloigne de la cible (depassement, ou cible deplacee derriere nous), ou
        //  - la distance restante est inferieure a la distance de freinage.
        int wantDir = (delta > 0) ? 1 : ((delta < 0) ? -1 : 0);
        float brake = (currentRate * currentRate) / (2.0f * accelRate);
        bool mustBrake = stopping || (wantDir != direction) || ((float)labs(delta) <= brake);

        if (mustBrake) {
            currentRate -= accelRate * dt;
            if (currentRate < firstStepRate()) currentRate = firstStepRate();
        } else if (currentRate < desiredRate) {
            currentRate += accelRate * dt;
            if (currentRate > desiredRate) currentRate = desiredRate;
        }

        bool slow = (currentRate <= firstStepRate() * 1.001f);

        if (delta == 0 && slow) { direction = 0; stopping = false; return true; }

        // Depassement : on est passe au-dela de la cible, il faut revenir.
        if (delta != 0 && ((delta > 0) != (direction > 0)) && slow) {
            direction = -direction;
            stopping = false;
        }
        return false;
    }

    // Instrumentation
    float getCurrentRateStepsPerSecond() const { return currentRate; }
    int getDirection() const { return direction; }

private:
    float firstStepRate() const { return sqrtf(2.0f * accelRate); }

    float stepsPerMM;
    long libSteps, targetSteps;
    int direction;
    float desiredRate, accelRate, currentRate;
    unsigned long lastStepTime;
    bool stopping;
};

#endif
