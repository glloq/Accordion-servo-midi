#ifndef AIR_FAULT_H
#define AIR_FAULT_H

// Codes de defaut communs a toutes les sources d'air.
//
// Une source ne produit que les codes qui la concernent : une turbine n'a pas de fin de
// course, une pompe n'a pas de calibration. Regrouper les codes evite a Instrument, aux
// tests et a la documentation de connaitre la source compilee.
//
// Les valeurs ne changent jamais : elles apparaissent telles quelles dans le tableau des
// defauts du README et dans les traces de mise au point.
enum AirFault {
    FAULT_NONE = 0,
    FAULT_HOMING_TIMEOUT,   // Calibration non terminee dans le temps imparti
    FAULT_HOMING_DISTANCE,  // Course maximale parcourue sans rencontrer la butee
    FAULT_ENDSTOP_WIRING,   // Les deux fins de course actifs simultanement
    FAULT_HOMING_DIRECTION, // Butee HAUTE atteinte alors qu'on descend (DIR inverse ?)
    FAULT_ENDSTOP_STUCK,    // Contact toujours actif apres degagement
    FAULT_STOP_TIMEOUT,     // La sequence d'arret ne se termine pas
    FAULT_OVERPRESSURE,     // Plafond de pression depasse (fuite bouchee, valve coincee)
    FAULT_PUMP_OVERRUN      // Pompe en marche continue trop longtemps sans atteindre la consigne
};

// Ancien nom, conserve pour ne pas casser les configurations et les tests existants.
typedef AirFault BellowFault;

#endif
