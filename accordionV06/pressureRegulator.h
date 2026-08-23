#ifndef PRESSURE_REGULATOR_H
#define PRESSURE_REGULATOR_H

#include "settings.h"

// =========================================================================================
// Capteur de pression et regulation en boucle fermee.
// -----------------------------------------------------------------------------------------
// Sans capteur, tout le systeme fonctionne en boucle ouverte : la demande d'air (somme des
// debits des anches ouvertes, ponderee par la velocite) pilote directement la vitesse du
// soufflet ou le rapport cyclique de la turbine. La pression reelle depend alors de
// l'etancheite, du nombre d'anches ouvertes et de l'usure.
//
// Avec capteur, la demande devient une CONSIGNE de pression et le correcteur ajuste la
// commande. Le correcteur ne remplace pas le calcul en boucle ouverte : il le CORRIGE, en
// produisant un facteur multiplicatif autour de 1.0. Deux raisons :
//   - la boucle ouverte est un bon predicteur (elle connait deja le nombre d'anches
//     ouvertes) : la laisser en avant fait de la regulation un simple rattrapage, avec un
//     integrateur qui reste petit ;
//   - le comportement sans capteur reste exactement celui d'avant, ce qui rend l'ajout du
//     capteur reversible sans retoucher les reglages de vitesse.
//
// Quand PRESSURE_SENSOR_ENABLED vaut 0, toutes les methodes sont vides et scale() renvoie
// 1.0 : le compilateur elimine entierement la classe.
// =========================================================================================
class PressureRegulator {
public:
    PressureRegulator();

    void begin();
    void update();               // Echantillonne et fait avancer le correcteur
    void setDemand(float demand); // Demande d'air courante -> consigne de pression
    void reset();                // Purge l'integrateur (arret, defaut, calibration)

    // Facteur correctif a appliquer a la commande calculee en boucle ouverte.
    // Vaut exactement 1.0 sans capteur ou en boucle ouverte.
    float scale() const;

    // Pression mesuree, en kPa. 0 sans capteur.
    float pressureKpa() const;
    // Consigne courante, en kPa. 0 sans capteur.
    float targetKpa() const;
    // Depassement du plafond de securite : fuite bouchee, valve bloquee fermee, anche
    // coincee. La source d'air doit s'arreter et la valve s'ouvrir.
    bool overPressure() const;

private:
#if PRESSURE_SENSOR_ENABLED
    float filtered;    // Pression filtree (kPa)
    float target;      // Consigne (kPa)
    float integral;    // Terme integral (sans dimension, deja multiplie par KI)
    float lastError;   // Erreur precedente, pour le terme derive
    float correction;  // Dernier facteur correctif produit
    uint32_t lastSampleTime;
    bool primed;       // Le premier echantillon initialise le filtre sans transitoire

    float readKpa() const;
#endif
};

#endif
