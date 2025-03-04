# Accordion-Servo-MIDI

## Objectif
Convertir un vieil accordéon en un instrument MIDI contrôlé par des servomoteurs et un moteur pas à pas.
Seuls le soufflet et les anches seront réutilisés, tandis que les joints et valves devront être refaits.

## Matériel
### Électronique
- **Microcontrôleur** : Arduino Leonardo ou Micro pour recevoir les messages MIDI via USB
- **Servomoteurs** : 59 servos (58 pour les notes, 1 pour la valve), maintenus fermés par des ressorts
- **Driver moteur pas à pas** : TMC2209 pour un fonctionnement silencieux et détection des surintensités
- **Moteur pas à pas** : NEMA 17 (24V)
- **Modules de commande** : PCA9685 pour gérer les servomoteurs
- **Capteurs** : Capteur de pression, fins de course (optique ou mécanique)

### Mécanique
- **Guides linéaires** : Axes 8x300mm récupérés d'imprimantes 3D
- **Transmission** : Tige filetée actionnée par une courroie fermée (GT2)
- **Support** : Roulements standards + roulements axiaux pour bloquer la vis en rotation
- **Enclosure** : Boîte en bois hermétique avec cadre avant en tissu fin monté sur aimants

## Sécurité
### Électrique
- Alimentation 5V et 24V protégée
- Fusible sur l'alimentation 230V

### Pneumatique
- Soupapes de sécurité contre vide ou surpression
- Capteur de pression pour adapter la pression maximale
- Fins de course min/max pour l'initialisation et correction des pertes de pas

### Mécanique
- Utilisation du TMC2209 pour détecter les chocs et surintensités

## Logique du code
### Communication et Contrôle
- **MIDI Handler** : Reçoit les messages MIDI via USB
- **Instrument Controller** : Gère les notes et communique avec les sous-contrôleurs
- **Servo Controller** : Active les servos via PCA9685 et gère l'optimisation de l'alimentation
- **Bellow Controller** : Gère le moteur pas à pas en fonction de la pression et des notes jouées

### Configuration
Tous les paramètres sont centralisés dans `settings.h`, incluant :
- Mapping des notes MIDI aux sorties PCA
- Facteur de débit d'air par note (pour ajuster la vitesse du moteur pas à pas)
- Groupes de sorties pour les notes harmonisées

