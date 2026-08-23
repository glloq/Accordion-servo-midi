> [!NOTE]
>  code non testé => il reste probablement du travail 

# 🎵 Accordion-Servo-MIDI 🎵

Transforme un accordéon acoustique en un instrument MIDI automatisé 🎹🎼

> [!NOTE]
>  je travaille avec un viel accordeon recupéré qui ne fonctionne plus, 
>  il a pris chaud (probablement dans une voiture), il y a des morceaux de cire partout et les anches ne tienent plus => Ca va me prendre du temps a remettre en etat avant de pouvoir tester :/

 # choses a faire :

**Cote electronique / mecanique** (le firmware est pret, le cablage ne l'est pas)
- separer les pins OE de chaque PCA — le firmware sait deja le faire
  (`PCA_OE_MODE = PCA_OE_PER_PCA` : il coupe alors les bancs d'anches au repos en gardant
  la valve generale alimentee), il reste a tirer les fils
- cabler un capteur de pression — le firmware sait deja le lire et reguler
  (`PRESSURE_SENSOR_ENABLED`), il reste a monter le capteur et a l'etalonner
- alimenter les servos par bancs (fusible + condensateur + load switch par PCA)

**Cote logiciel**
- piloter le TMC2209 en UART (courant RMS, microsteps, StealthChop, detection de blocage)
- generateur STEP sur timer materiel, independant de la boucle MIDI/I2C

**Cote projet**
- validation sur le mecanisme reel (hardware-in-the-loop)
- plans 2D des planches bois 
- plans 3D et stl des fichiers a imprimer
- liste completes des materiaux

## 📌 Objectif

Ce projet convertit un accordéon acoustique en un instrument MIDI piloté par des servomoteurs, permettant de :
- ✔ Lire et interpreter des messages MIDI (DIN/UART par défaut, USB natif en option).
- ✔ Contrôler chaque note individuellement via des servos (ou des électroaimants).
- ✔ Produire l'air par le système de votre choix : soufflet à moteur pas à pas, soufflet à
     servo, turbine PWM, turbine brushless sur ESC, ou pompe tout-ou-rien.
- ✔ Simuler le jeu d’un accordéoniste avec un soufflet dynamique.
- ✔ Gérer les notes et accords de la main droite et de la main gauche.
- ✔ Réguler le débit d’air via airFlowMultiplier en boucle ouverte, ou en boucle fermée
     avec un capteur de pression (PI/PID).

**Tout se configure dans un seul fichier**, `accordionV06/config.h`, de préférence généré
par l'interface : `tools/configurator/index.html`. Voir
[Configurer sa machine](#-configurer-sa-machine).

> [!WARNING]
> **À vérifier impérativement avant le premier essai mécanique :**
> - `MICRO_STEP` dans `config.h` doit correspondre au réglage **physique** du TMC2209
>   (cavaliers MS1/MS2). En mode standalone, MS1=MS2=LOW donne souvent 1/8 et non 1/16.
>   `STEPS_PER_MM` en est déduit : une erreur ici fausse la course d'un facteur 2 à 16.
> - Les fins de course sont sur **D5 et D6**. Ne jamais les câbler sur D2/D3, qui sont
>   SDA/SCL (bus I²C des PCA9685) sur Leonardo/Micro. `settings.h` refuse désormais de
>   compiler dans ce cas, et le configurateur le signale avant même la compilation.

## 📌 Configurer sa machine

Toute la description de l'instrument tient dans **`accordionV06/config.h`** : c'est le seul
fichier à modifier. `settings.h` n'en contient plus aucun réglage — uniquement les valeurs
qui en découlent (pas/mm, angle d'ouverture effectif, vitesse réellement tenable) et les
vérifications qui refusent de compiler une configuration impossible.

Le plus simple est de le **générer** :

```
tools/configurator/index.html     # à ouvrir dans un navigateur, hors ligne
```

L'interface n'affiche que les réglages du système choisi, tient à jour une carte
d'occupation des canaux PCA9685, et attrape ce qu'une directive `#if` ne sait pas voir :
canaux réclamés par deux organes, notes MIDI en double, microstepping incompatible avec la
vitesse demandée, capteur saturant sous le seuil de sécurité. Détails dans
[`tools/configurator/README.md`](tools/configurator/README.md).

### 🔹 Ce qui est configurable

| Domaine | Choix possibles |
|---|---|
| **Source d'air** | soufflet pas à pas · soufflet à servo · turbine PWM · turbine brushless (ESC) · pompe tout-ou-rien |
| **Sens de l'air** | soufflage ou aspiration (sources unidirectionnelles) |
| **Actionneurs d'anches** | servomoteur · électroaimant (appel puis maintien réduit) |
| **Valve générale** | servo sur canal PCA · électrovanne sur broche · aucune |
| **OE des PCA9685** | une broche partagée · une broche par PCA · câblé à la masse |
| **Routage MIDI** | un canal par main · point de partage sur la note · fusionné |
| **Transport MIDI** | DIN/UART · USB natif · les deux |
| **Transmission du soufflet** | tige filetée · courroie et poulie |
| **Régulation** | boucle ouverte · PI · PID, avec capteur de pression |
| **Tables de notes** | nombre d'anches, notes MIDI, canaux, débits, angles et priorités libres — une main peut être vide (instrument mélodie seule, ou basses seules) |

### 🔹 Les cinq sources d'air

Le choix est fait **à la compilation** : une seule implémentation est embarquée. Sur
ATmega32U4 (2560 octets de SRAM), embarquer cinq pilotes pour n'en utiliser qu'un serait
indéfendable — et aucune machine n'a deux sources d'air à la fois.

| Source | Principe | Calibration | Remarques |
|---|---|---|---|
| `AIR_SOURCE_BELLOW_STEPPER` | Soufflet sur vis ou courroie, moteur pas à pas | Homing 2 passes | Le plus fidèle : pression produite en poussant **et** en tirant. Seul système à course mesurée. |
| `AIR_SOURCE_BELLOW_SERVO` | Soufflet entraîné par un servo | Aucune | Même principe bidirectionnel, position commandée en angle. Ni fin de course ni homing. Course et force limitées. |
| `AIR_SOURCE_BLOWER_PWM` | Turbine continue sur MOSFET | À-coup de démarrage | Unidirectionnel, pression continue. Simple, bruyant, sans nuance mécanique. |
| `AIR_SOURCE_BLOWER_ESC` | Turbine brushless, impulsion produite par un canal PCA libre | Armement de l'ESC | Plus de débit. Les notes sont refusées tant que l'armement n'est pas terminé. |
| `AIR_SOURCE_PUMP_ONOFF` | Pompe à membrane ou piston, réservoir tampon | Aucune | Hystérésis avec capteur, modulation lente sans. Protection thermique obligatoire : ces pompes n'ont pas de service continu. |

Ajouter une sixième source (pompe à pied, compresseur, soufflerie d'orgue) se fait sans
toucher au reste du firmware : il suffit d'implémenter l'interface commune décrite en tête
de [`accordionV06/airSource.h`](accordionV06/airSource.h).

### 🔹 Régulation de pression (optionnelle)

Sans capteur, tout reste en boucle ouverte : la demande d'air pilote directement la vitesse
ou le rapport cyclique, et la pression réelle dépend de l'étanchéité, du nombre d'anches
ouvertes et de l'usure.

Avec capteur (`PRESSURE_SENSOR_ENABLED`), la demande devient une **consigne de pression**.
Le correcteur ne remplace pas le calcul en boucle ouverte, il le **corrige** : il produit un
facteur multiplicatif autour de 1,0, borné. Deux raisons :

- la boucle ouverte est déjà un bon prédicteur — elle connaît le nombre d'anches ouvertes —
  ce qui réduit la régulation à un rattrapage, avec un intégrateur qui reste petit ;
- le comportement sans capteur reste **exactement** celui d'avant, ce qui rend l'ajout du
  capteur réversible sans retoucher aux réglages de vitesse.

Le capteur sert aussi de sécurité : au-delà de `PRESSURE_MAX_KPA`, l'instrument passe en
`FAULT_OVERPRESSURE`, coupe la source d'air et ouvre la valve. C'est le seul moyen de
détecter une valve bloquée fermée ou une sortie bouchée.


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

C'est le matériel de **ma** machine. Le firmware en accepte d'autres, sans modification de
code — seulement de `config.h` :

- **à la place du moteur pas à pas et de son driver** : un servo de forte course, une
  turbine sur MOSFET, une turbine brushless avec son ESC, ou une pompe à membrane avec un
  réservoir tampon. Les fins de course, le TMC2209 et son alimentation 24 V ne servent alors
  plus à rien ;
- **à la place des servomoteurs d'anches** : des électroaimants, pilotés par les mêmes
  PCA9685 avec un courant de maintien réduit ;
- **en plus** : un capteur de pression analogique (type MPX5010) pour passer en boucle
  fermée et détecter les surpressions ;
- **moins de PCA9685** si l'instrument a moins d'anches, ou davantage s'il en a plus.

### 🔹 Mécanique

- Guides linéaires (axes 8x300mm, récupérés d'imprimantes 3D)	=>  Maintiennent le mouvement du soufflet
- Transmission : Tige filetée pas de 16 mm	=> Transforme la rotation en mouvement linéaire
- Courroie GT2 fermée	=>  Relie le moteur a l'ecrou qui permet de deplacer la vis et ouvrir fermer le soufflet
- Roulements standards + roulements axiaux	=> Stabilisent la tige filetée et repartissent le poid 
- Boîte hermétique en bois	=> Cache les composants et améliore l'esthétique
- Tissu fin sur la façade (monté sur aimants)	=> Protège de la poussière sans gêner le son

## 📌 Logique du Code
### 🔹 Modules Principaux

| Fichier | Rôle |
|---|---|
| `config.h` | **Toute** la description de la machine. Le seul fichier à modifier. |
| `airSourceTypes.h` | Identifiants des systèmes sélectionnables dans `config.h`. |
| `settings.h` | Valeurs dérivées et vérifications à la compilation. Aucun réglage. |
| `midiHandler` | Routeur multi-transports (DIN/UART sur Serial1, USB natif en option). Un seul message par tour de boucle. |
| `instrument` | Attribue les notes aux deux mains selon `MIDI_ROUTING`, gère polyphonie, vol de voix, sustain, attaque, inactivité et défauts. |
| `handController` | Une instance par main. Ouvre et ferme les valves, calcule la demande d'air pondérée par la vélocité. |
| `servoController` | Accès unique aux PCA9685 : angles servo, impulsions ESC, rapports cycliques d'électroaimant, broches OE, détection de bus mort. |
| `noteMapping` | Tables de notes, construites depuis les X-macros de `config.h`. En PROGMEM sur AVR. |
| `airSource.h` | Sélectionne la source d'air compilée et documente l'interface commune. |
| `bellowController` | Source d'air : soufflet pas à pas. Homing deux passes, arrêt sur fin de course, inversion 30/70 %. |
| `servoBellowController` | Source d'air : soufflet à servomoteur. |
| `blowerController` | Sources d'air : turbine PWM, turbine ESC, pompe tout-ou-rien. |
| `airValve` | Valve générale de mise à l'air libre, partagée par toutes les sources. |
| `pressureRegulator` | Capteur de pression et correcteur PI/PID. Disparaît entièrement à la compilation sans capteur. |

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

Cette section décrit le comportement du **soufflet à moteur pas à pas**, la source d'air par
défaut. Le calcul de la demande d'air (points 1, 3 et 4) est commun à toutes les sources ;
seule sa traduction change — vitesse de déplacement pour un soufflet, rapport cyclique pour
une turbine, hystérésis pour une pompe.

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
- `HOMING` couvre la mise en route de **toute** source d'air, pas seulement la recherche du
  zéro : c'est aussi l'armement d'un ESC, pendant lequel les notes doivent être refusées
  puisqu'un ESC non armé ignore toute consigne. Les sources sans mise en route (soufflet à
  servo, turbine PWM, pompe) passent directement à `READY`.

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
| `FAULT_OVERPRESSURE`     | Plafond de pression dépassé : fuite bouchée, valve bloquée fermée, anche coincée. Toutes sources, capteur requis |
| `FAULT_PUMP_OVERRUN`     | Pompe en marche continue trop longtemps sans approcher la consigne : fuite, clapet ou membrane percée |

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

Pour la machine réelle, c'est **`config.h` qui fait foi** et `leonardo_din` (ou
`leonardo_usb`) qu'il faut utiliser. Les autres environnements de `platformio.ini`
(`bellow_servo`, `blower_pwm`, `blower_esc`, `pump_onoff`, `pressure_pi`, `solenoid`,
`routing_split`, `oe_per_pca`…) ne servent qu'à vérifier que **chaque variante tient sur la
cible** : une combinaison qui déborde la FLASH ou la SRAM doit se voir en intégration
continue, pas au moment du montage.

Les tables de notes vivent en **PROGMEM** sur AVR (~580 octets de SRAM libérés sur les
2560 de l'ATmega32U4) : tout accès passe obligatoirement par les accesseurs de
`noteMapping.h`. La cible `test_behavior_progmem` rejoue toute la suite en compilant ce
chemin de code, pour qu'un accès direct aux tables ne puisse pas passer inaperçu.

Les tests rejouent le firmware sur une machine simulée. Le stub `FlexyStepper` modélise
**un pas par appel**, les rampes d'accélération, la persistance de la direction, et le
fait que la position **physique** ne bouge que si le driver est alimenté — alors que le
compteur interne de la bibliothèque avance dans tous les cas. Les fins de course sont
déduits de la position physique, comme de vrais capteurs.

Chaque **source d'air** et chaque **variante de configuration** structurante a son propre
binaire de test : ce sont des chemins de code compilés conditionnellement, et n'en tester
qu'un reviendrait à ne pas tester les autres du tout.

| Suite | Couvre |
|-------|--------|
| `test_note_mapping` | Disposition du clavier gauche, collisions de canaux PCA, angles. Rejouée pour les configurations qui réservent un canal supplémentaire (servo de soufflet, ESC) |
| `test_stepper_stop` | Arrêt sur fin de course, surcourse, recalage à l'arrêt, DIR inversé |
| `test_timing`       | Fréquence de pas atteinte, rafale MIDI, coût I²C d'un accord |
| `test_behavior`     | Homing et défauts, gating MIDI, 30/70 %, CC7, sustain, polyphonie, PCA absent |
| `test_air_servo_bellow` | Soufflet à servo : prêt sans calibration, oscillation, arrêt sur CC7 |
| `test_air_blower_pwm`   | Turbine : à-coup de démarrage, duty proportionnel au nombre d'anches, coupure en défaut |
| `test_air_blower_esc`   | ESC : notes refusées pendant l'armement, gaz proportionnels ensuite |
| `test_air_pump`         | Pompe : modulation lente, protection thermique, sortie inactive au démarrage |
| `test_pressure`         | Conversion ADC→kPa, sens et bornage du correcteur, anti-emballement, défaut de surpression |
| `test_variant_split` / `_merge` | Routages MIDI alternatifs, y compris une note jouable par les deux mains |
| `test_variant_solenoid` | Électroaimants : appel pleine puissance puis retombée au maintien, sans commande répétée |
| `test_variant_oe_per_pca` | OE séparés : les bancs d'anches sont coupés au repos, celui de la valve reste alimenté, et un banc coupé se réactive à la commande suivante |

Deux défauts réels ont été trouvés en écrivant ces tests : la broche de la pompe n'était
jamais forcée à son niveau inactif au démarrage — avec un étage de puissance inverseur, la
pompe démarrait dès la mise sous tension — et les canaux PCA par défaut du servo de soufflet
et de l'ESC tombaient sur des canaux déjà occupés par des notes.

`test_stepper_stop` commence par vérifier que le **stub lui-même** reproduit le défaut :
l'ancien motif `setTargetPosition(position courante)` produit bien ~9,8 mm de surcourse à
10 mm/s (`v²/2a`), là où le firmware actuel coupe à 0,002 mm du contact. Un stub trop
idéalisé validerait n'importe quoi.

Le projet reste compilable tel quel dans l'IDE Arduino (dossier `accordionV06/`).
Bibliothèques requises : *Adafruit PWM Servo Driver*, *MIDI Library*, *FlexyStepper*
(uniquement pour le soufflet à moteur pas à pas), plus *USB-MIDI* (lathoub) si
`MIDI_TRANSPORT_USB` est activé.

### 🔹 Empreinte mémoire

Une seule source d'air est compilée : les autres n'existent pas dans le binaire. Même chose
pour la régulation de pression, qui disparaît entièrement sans capteur. Mesures faites avec
`avr-g++ -Os` pour ATmega32U4, **sur le code du projet seul** — les bibliothèques réelles et
le cœur Arduino sont remplacés par les stubs de test, donc ces chiffres servent à comparer
les variantes entre elles, pas à prédire la taille finale.

| Configuration | FLASH | SRAM |
|---|---:|---:|
| Soufflet pas à pas (défaut) | 10 706 o | 1 412 o |
| Soufflet à servo | 6 508 o | 1 339 o |
| Turbine PWM | 6 456 o | 1 416 o |
| Turbine ESC | 6 486 o | 1 352 o |
| Pompe tout-ou-rien | 6 702 o | 1 352 o |
| Soufflet pas à pas + capteur PI | 11 604 o | 1 500 o |
| Électroaimants au lieu des servos | 10 978 o | 1 422 o |
| Une broche OE par PCA | 10 902 o | 1 416 o |

Toute cette modularité coûte **466 octets de FLASH et 6 octets de SRAM** par rapport à la
version qui ne savait piloter qu'un soufflet pas à pas (10 240 o / 1 406 o dans les mêmes
conditions). C'est le prix du choix à la compilation plutôt qu'à l'exécution : une classe de
base virtuelle avec cinq implémentations embarquées aurait coûté plusieurs kilo-octets et de
la SRAM, pour un instrument qui n'a de toute façon qu'une seule source d'air.

> [!NOTE]
> Ces mesures n'ont **pas** pu être faites avec le cœur Arduino et les bibliothèques
> réelles : la politique réseau de l'environnement de développement bloque le registre
> PlatformIO. C'est l'intégration continue qui vérifie que chaque variante tient réellement
> sur la cible.

### 🔹 Ce que la compilation refuse

`settings.h` arrête la compilation sur une configuration impossible plutôt que de la laisser
atteindre la mécanique : broche posée sur SDA/SCL, deux modules actifs sur la même broche,
index de PCA hors de la liste déclarée, seuils d'inversion incohérents, `NUM_NOTES_*` ne
correspondant pas au nombre de lignes réellement écrites, valve et ESC sur le même canal,
ESC dont l'impulsion ne tient pas dans la période PWM choisie.

Le configurateur signale les mêmes erreurs **avant** la compilation, et y ajoute celles
qu'une directive `#if` ne sait pas exprimer — collisions de canaux avec les notes, doublons
de notes MIDI, microstepping incompatible avec la vitesse demandée.



