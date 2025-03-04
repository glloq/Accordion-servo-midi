# Accordion-Servo-MIDI

## Objectif
Convertir un vieil accordéon en un instrument MIDI contrôlé par des servomoteurs et un moteur pas à pas. Seuls le soufflet et les anches seront réutilisés, tandis que les joints et valves devront être refaits.

## Matériel
### Électronique
- **Microcontrôleur** : Arduino Leonardo ou Micro pour gérer la communication MIDI et contrôler les moteurs.
- **Servomoteurs** : 59 servos (58 pour les notes, 1 pour la valve) maintenus fermés par des ressorts.
- **4 x PCA9685 (16 canaux chacun)** : Modules I2C pour piloter les servos.
- **Alimentation 5V 10A** : Pour alimenter tous les servos.
- **Moteur pas à pas NEMA 17 (24V, 1.8°/200 pas/tour)** : Pour actionner le soufflet.
- **Driver moteur pas à pas TMC2209 (24V, silencieux, StealthChop, StallGuard)** : Pour réguler la vitesse et détecter les anomalies.
- **Alimentation 24V 5A** : Pour alimenter le moteur pas à pas.
- **Capteur de pression différentielle (MS4525DO ou MPX5010DP)** : Pour ajuster la force du soufflet.
- **2 x Fin de course optiques ou StallGuard** : Pour limiter le déplacement du soufflet.
- **Soupape de sécurité pneumatique** : Pour éviter une surpression ou un vide excessif.

### Mécanique
- **Guides linéaires** : Axes 8x300mm récupérés d'imprimantes 3D.
- **Transmission** : Tige filetée **pas de 16 mm** actionnée par une courroie fermée (GT2).
- **Support** : Roulements standards + roulements axiaux pour bloquer la vis en rotation.
- **Enclosure** : Boîte en bois hermétique avec cadre avant en tissu fin monté sur aimants.

## Sécurité
### Électrique
- Alimentation 5V et 24V protégée.
- Fusible sur l'alimentation 230V.

### Pneumatique
- Soupapes de sécurité contre vide ou surpression.
- Capteur de pression pour adapter la pression maximale.
- Fins de course min/max pour l'initialisation et correction des pertes de pas.

### Mécanique
- Utilisation du TMC2209 pour détecter les chocs et surintensités.

## Logique du code
### Communication et Contrôle
- **MIDI Handler** : Reçoit les messages MIDI via USB.
- **Instrument Controller** : Gère les notes et communique avec les sous-contrôleurs.
- **Servo Controller** : Active les servos via PCA9685 et gère l'optimisation de l'alimentation.
- **Bellow Controller** : Gère le moteur pas à pas en fonction de la pression et des notes jouées.
- **TMC2209 StallGuard** : Détecte les fins de course et bloque le moteur en cas de surcharge.

### Configuration
Tous les paramètres sont centralisés dans `settings.h`, incluant :
- Mapping des notes MIDI aux sorties PCA.
- Facteur de débit d'air par note (pour ajuster la vitesse du moteur pas à pas).
- Groupes de sorties pour les notes harmonisées.
- Seuils de pression et déclenchement du soufflet en fonction des notes jouées.


