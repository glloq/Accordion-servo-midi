> [!NOTE]
>  code non testé => il reste probablement du travail 

# 🎵 Accordion-Servo-MIDI 🎵

Transforme un accordéon acoustique en un instrument MIDI automatisé 🎹🎼

> [!NOTE]
>  je travaille avec un viel accordeon recupéré qui ne fonctionne plus, 
>  il a pris chaud (probablement dans une voiture), il y a des morceaux de cire partout et les anches ne tienent plus => Ca va me prendre du temps a remettre en etat avant de pouvoir tester :/

 # choses a faire :
- separer les pins OE de chaque PCA (aujourd'hui un seul OE pour les 4, valve comprise)
- ajouter un capteur de pression et une regulation PI/PID (aujourd'hui tout est en boucle ouverte)
- piloter le TMC2209 en UART (courant RMS, microsteps, StealthChop, detection de blocage)
- alimenter les servos par bancs (fusible + condensateur + load switch par PCA)
- generateur STEP sur timer materiel, independant de la boucle MIDI/I2C
- validation sur le mecanisme reel (hardware-in-the-loop)
- plans 2D des planches bois 
- plans 3D et stl des fichiers a imprimer
- liste completes des materiaux

## 📌 Objectif

Ce projet convertit un accordéon acoustique en un instrument MIDI piloté par des servomoteurs et un moteur pas à pas, permettant de :
- ✔ Lire et interpreter des messages MIDI (DIN/UART par défaut, USB natif en option).
- ✔ Contrôler chaque note individuellement via des servos.
- ✔ Simuler le jeu d’un accordéoniste avec un soufflet dynamique.
- ✔ Gérer les notes et accords de la main droite et de la main gauche.
- ✔ Réguler automatiquement le débit d’air via airFlowMultiplier (sans capteur de pression).

> [!WARNING]
> **À vérifier impérativement avant le premier essai mécanique :**
> - `MICRO_STEP` dans `settings.h` doit correspondre au réglage **physique** du TMC2209
>   (cavaliers MS1/MS2). En mode standalone, MS1=MS2=LOW donne souvent 1/8 et non 1/16.
>   `STEPS_PER_MM` en est déduit : une erreur ici fausse la course d'un facteur 2 à 16.
> - Les fins de course sont sur **D5 et D6**. Ne jamais les câbler sur D2/D3, qui sont
>   SDA/SCL (bus I²C des PCA9685) sur Leonardo/Micro.


## Schema de principe

![schematics of the idea](https://github.com/glloq/Accordion-servo-midi/blob/main/img/schemas%20principe.png)



## 📌 Matériel
### 🔹 Électronique
- Arduino Leonardo / Micro	=> Reçoit les messages MIDI et contrôle les moteurs
- Servomoteurs (59x)	=> Ouvrent et ferment les soupapes des anches
- PCA9685 (4x, 16 canaux)	=> Contrôle les servos en I2C
- Alimentation 5V 10A	=>  Fournit l'énergie aux servos
- Moteur pas à pas NEMA 17 (24V, 1.8°/200 pas/tour)	=>  Actionne le soufflet
- Driver TMC2209 (StealthChop)	=>  Contrôle précis du moteur pas à pas, silencieux
- Alimentation 24V 5A	=>  Alimente le moteur pas à pas
- 2x Fin de course optiques	=> Limite le déplacement du soufflet sans bruits mecanique (D5 / D6)
  
### 🔹 Mécanique

- Guides linéaires (axes 8x300mm, récupérés d'imprimantes 3D)	=>  Maintiennent le mouvement du soufflet
- Transmission : Tige filetée pas de 16 mm	=> Transforme la rotation en mouvement linéaire
- Courroie GT2 fermée	=>  Relie le moteur a l'ecrou qui permet de deplacer la vis et ouvrir fermer le soufflet
- Roulements standards + roulements axiaux	=> Stabilisent la tige filetée et repartissent le poid 
- Boîte hermétique en bois	=> Cache les composants et améliore l'esthétique
- Tissu fin sur la façade (monté sur aimants)	=> Protège de la poussière sans gêner le son

## 📌 Logique du Code
### 🔹 Modules Principaux

- MIDI Handler	=> Routeur multi-transports (DIN/UART sur Serial1, USB natif en option)  
- Instrument Controller	=> Interprète les notes et attribut les notes aux mains droite et gauche et gere le mouvement du soufflet
- LeftHandController => gere les notes pour le canal midi 1 de la main gauche
- RightHandControlelr => gere les notes pour le canal midi 2 de la main droite
- Servo Controller	=> Active les servos via PCA9685 pour gerer l'ouverture/fermeture des valves ou desactiver l'alimentation quand inactif (reduire le bruit)  
- Bellow Controller	=> Gère le moteur pas à pas en fonction du airFlowMultiplier de la velocité et du volume  
- Settings => regroupe tout les reglages pour adapter le systeme  

## 📌 Gestion des Notes

### 🔹 Main Droite (Mélodie, 34 Notes Chromatiques)

🎹 Chaque note active 1 servo unique.
🎹 Ces notes MIDI sont continues et se suivent sans combinaisons.

### 🔹 Main Gauche (Basses et Accords)

#### 🎵 Numéros MIDI des 24 Notes de la Main Gauche 🎵

| **Rangée Basse (Grave)** | **36** | **43** | **38** | **45** | **40** | **47** | **42** | **49** | **44** | **51** | **46** | **41** |
|-------------------------|------|------|------|------|------|------|------|------|------|------|------|------|
| **Rangée Aiguë**       | **48** | **55** | **50** | **57** | **52** | **59** | **54** | **61** | **56** | **63** | **58** | **53** |


## 📌 Gestion du Débit d’Air et du Soufflet

- ✔ Chaque note a un airFlowMultiplier (les graves consomment plus d'air).
- ✔ Le moteur ajuste sa vitesse en fonction des notes jouées.
- ✔ Si aucune note n’est active, le moteur s’arrête. `CC7 = 0` coupe réellement la pression.
- ✔ La vélocité MIDI agit sur le **débit d'air** (attaque brève puis niveau tenu), pas sur
  les servos : une valve d'anche est ouverte ou fermée, sans nuance possible. Elle est
  appliquée **par note** — une note douce ajoutée à un accord fort ne fait pas chuter le
  débit de tout l'accord, et l'attaque ne concerne que la dernière note déclenchée.
- ✔ Alternance du sens d’ouverture/fermeture du soufflet :

    Le sens s'inverse à **70 % d'ouverture** et **30 % de fermeture**. Ces seuils sont
    évalués **en continu** dans la boucle principale, pas seulement sur événement MIDI :
    une note tenue fait donc osciller le soufflet sans jamais atteindre les fins de course.

### 🔹 Polyphonie et priorité (max 15 notes)

Quand la limite est atteinte, une note entrante ne prend la place que d'une note
**strictement moins prioritaire**, la plus ancienne d'abord :

| Priorité | Voix                                   |
|---------:|----------------------------------------|
|        3 | Basses fondamentales (rangée grave G)  |
|        2 | Mélodie (main droite)                  |
|        1 | Accords (rangée aiguë main gauche)     |

Les notes retenues uniquement par la pédale de sustain sont sacrifiées en premier.
Si aucune voix moins prioritaire n'existe, la note entrante est ignorée.

## 📌 Calibration et Sécurité

### 🔹 Machine à états

```
BOOT → SERVO_INIT → HOMING → READY ⇄ (inactivité)
                       │        │
                       └────────┴──→ FAULT
```

- Les `NoteOn` sont **refusées** tant que l'état n'est pas `READY` (init servos, homing,
  défaut). Le MIDI Panic (CC120/CC123) reste accepté dans tous les états.
- L'arrêt sur inactivité et la coupure de l'OE des PCA sont inhibés hors de `READY` :
  couper le driver ou la valve pendant le homing bloquait le soufflet.

### 🔹 Arrêt sur fin de course

Le contact est lu **brut**, sans debounce, et déclenche immédiatement la coupure de
`ENABLE` : à 25 mm/s, attendre les 50 ms de debounce représenterait 1,25 mm de surcourse.
Le debounce ne sert qu'à *interpréter* ensuite le contact (réel ou parasite).

`FlexyStepper` conserve son propre état de direction et de rampe : changer la position
courante ou la cible **n'arrête pas** le moteur, et la bibliothèque documente que
`setCurrentPosition()` ne doit être appelée qu'à l'arrêt. D'où la séquence :

```
contact brut ──► coupure ENABLE (arrêt physique, immédiat)
             ──► purge de l'état cinématique, driver déjà coupé (aucun mouvement)
             ──► attente de la confirmation par debounce
             ──► recalage de position, puis reprise ou défaut
```

### 🔹 Calibration automatique (non bloquante, deux passes)

```
approche rapide (10 mm/s) ──► contact ──► dégagement 4 mm ──► réapproche lente (1,5 mm/s) ──► zéro
```

Le premier contact ne sert qu'à localiser grossièrement la butée : la surcourse y est
inconnue puisque le driver est coupé sans rampe maîtrisée. Seule la seconde approche,
lente, fixe un zéro répétable. Un fin de course déjà enfoncé au démarrage est détecté
immédiatement.

### 🔹 Défauts détectés

| Code                     | Cause                                                     |
|--------------------------|-----------------------------------------------------------|
| `FAULT_HOMING_TIMEOUT`   | Homing non terminé dans `HOMING_TIMEOUT_MS`                |
| `FAULT_HOMING_DISTANCE`  | Course maximale parcourue sans rencontrer la butée         |
| `FAULT_HOMING_DIRECTION` | Butée **haute** atteinte pendant le homing (DIR inversé ?) |
| `FAULT_ENDSTOP_WIRING`   | Les deux fins de course actifs simultanément               |
| `FAULT_ENDSTOP_STUCK`    | Contact toujours actif après le dégagement                 |
| `FAULT_STOP_TIMEOUT`     | La séquence d'arrêt ne se termine pas                      |

Côté servos, l'instrument refuse aussi de démarrer si un PCA9685 ne répond pas
(`INST_FAULT_PCA_MISSING`), et passe en défaut si le bus I²C accumule
`SERVO_I2C_ERROR_LIMIT` erreurs consécutives en cours de jeu (`INST_FAULT_PCA_BUS`) —
un PCA muet laisserait sinon des anches ouvertes sans que rien ne le signale.

En défaut : moteur coupé, valve ouverte (pression libérée), toutes les notes fermées,
servos maintenus alimentés pour que la valve tienne sa position. L'état est verrouillé.

> [!NOTE]
> La broche `OE` des PCA9685 ne coupe **que les sorties PWM**. Le rail +5 V des 59 servos
> reste alimenté. Une vraie mise hors tension demanderait un load switch / MOSFET
> high-side par banc de servos.

## 📌 Compilation et tests

```bash
# Firmware (PlatformIO)
pio run -e leonardo_din       # MIDI DIN sur Serial1 (défaut, empreinte la plus légère)
pio run -e leonardo_usb       # MIDI USB natif seul
pio run -e leonardo_din_usb   # les deux (expérimental : SRAM très juste sur ATmega32U4)
pio run -e leonardo_din -t upload

# Tests (g++ seul, ni toolchain AVR ni matériel)
make -C test
```

Les tables de notes vivent en **PROGMEM** sur AVR (~580 octets de SRAM libérés sur les
2560 de l'ATmega32U4) : tout accès passe obligatoirement par les accesseurs de
`noteMapping.h`. La cible `test_behavior_progmem` rejoue toute la suite en compilant ce
chemin de code, pour qu'un accès direct aux tables ne puisse pas passer inaperçu.

Les tests rejouent le firmware sur une machine simulée. Le stub `FlexyStepper` modélise
**un pas par appel**, les rampes d'accélération, la persistance de la direction, et le
fait que la position **physique** ne bouge que si le driver est alimenté — alors que le
compteur interne de la bibliothèque avance dans tous les cas. Les fins de course sont
déduits de la position physique, comme de vrais capteurs.

| Suite | Couvre |
|-------|--------|
| `test_note_mapping` | Disposition du clavier gauche, collisions de canaux PCA, angles |
| `test_stepper_stop` | Arrêt sur fin de course, surcourse, recalage à l'arrêt, DIR inversé |
| `test_timing`       | Fréquence de pas atteinte, rafale MIDI, coût I²C d'un accord |
| `test_behavior`     | Homing et défauts, gating MIDI, 30/70 %, CC7, sustain, polyphonie, PCA absent |

`test_stepper_stop` commence par vérifier que le **stub lui-même** reproduit le défaut :
l'ancien motif `setTargetPosition(position courante)` produit bien ~9,8 mm de surcourse à
10 mm/s (`v²/2a`), là où le firmware actuel coupe à 0,002 mm du contact. Un stub trop
idéalisé validerait n'importe quoi.

Le projet reste compilable tel quel dans l'IDE Arduino (dossier `accordionV06/`).
Bibliothèques requises : *Adafruit PWM Servo Driver*, *MIDI Library*, *FlexyStepper*,
plus *USB-MIDI* (lathoub) si `MIDI_TRANSPORT_USB` est activé.



