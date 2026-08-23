#ifndef AIR_VALVE_H
#define AIR_VALVE_H

#include "settings.h"
#include "servoController.h"

// Valve generale de mise a l'air libre, partagee par toutes les sources d'air.
//
// Ouverte : la pression s'echappe, le soufflet peut bouger sans produire de son, une
// turbine tourne sans charge. C'est la position de securite : demarrage, calibration,
// defaut, repos. Elle est fermee des la premiere note.
//
// Le type de valve (servo sur PCA, electrovanne sur broche, ou aucune) est choisi dans
// config.h et resolu a la compilation. Le reste du firmware ne voit que open()/close().
class AirValve {
public:
    explicit AirValve(ServoController &servoCtrl);

    void begin();
    void open();
    void close();

    bool isOpen() const { return valveOpen; }

private:
#if AIR_VALVE_TYPE == AIR_VALVE_SERVO
    ServoController &servoController;
#endif
    bool valveOpen;

    void apply(bool open);
};

#endif
