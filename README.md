# 🎵 Accordion-Servo-MIDI 🎵

Transforme un accordéon acoustique en un instrument MIDI automatisé 🎹🎼

> [!NOTE]
>  je traville avec un viel accordeon recupéré qui ne fonctionne plus
>  l'accordeon a pris chaud, il y a des morceaux de cire partout et les anches ne tienent plus => Ca va me prendre du temps a remettre en etat avant de pouvoir tester :/

## 📌 Objectif

Ce projet convertit un accordéon acoustique en un instrument MIDI piloté par des servomoteurs et un moteur pas à pas, permettant de :
- ✔ Lire des fichiers MIDI via USB.
- ✔ Contrôler chaque note individuellement via des servos.
- ✔ Simuler le jeu d’un accordéoniste avec un soufflet dynamique.
- ✔ Gérer les notes et accords de la main droite et de la main gauche.
- ✔ Réguler automatiquement le débit d’air via airFlowMultiplier (sans capteur de pression).
- ✔ Permettre le passage entre accords majeurs et mineurs via CC MIDI.

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
- 2x Fin de course optiques	=> Limite le déplacement du soufflet sans bruits mecanique
  
### 🔹 Mécanique

- Guides linéaires (axes 8x300mm, récupérés d'imprimantes 3D)	=>  Maintiennent le mouvement du soufflet
- Transmission : Tige filetée pas de 16 mm	=> Transforme la rotation en mouvement linéaire
- Courroie GT2 fermée	=>  Relie le moteur a l'ecrou qui permet de deplacer la vis et ouvrir fermer le soufflet
- Roulements standards + roulements axiaux	=> Stabilisent la tige filetée et repartissent le poid 
- Boîte hermétique en bois	=> Cache les composants et améliore l'esthétique
- Tissu fin sur la façade (monté sur aimants)	=> Protège de la poussière sans gêner le son

## 📌 Logique du Code
### 🔹 Modules Principaux

- MIDI Handler	=> Reçoit les messages MIDI via USB  
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


 #### Tableau des Combinaisons d'Accords pour les 24 Servomoteurs
 
> [!IMPORTANT]
> les differents acords devrons etre géré sur le fichier midi => ca utilise trop de memoire


| **Servo ID** | **Basse Simple (1 Servo Actif)** | **Accord Majeur (3 Servos Actifs)** | **Accord Mineur (3 Servos Actifs)** | **Accord 7ème (4 Servos Actifs)** | **Accord Diminué (3 Servos Actifs)** | **Accord Augmenté (3 Servos Actifs)** | **Accord à 2 Notes (Fondamentale + Quinte)** |
|-------------|-------------|-------------------------|-------------------------|------------------------|--------------------------|--------------------------|----------------------------------|
| 1  | 50  | 50 - 54 - 57  | 50 - 53 - 57  | 50 - 54 - 57 - 60  | 50 - 53 - 56  | 50 - 54 - 58  | 50 - 57 |
| 2  | 49  | 49 - 53 - 56  | 49 - 52 - 56  | 49 - 53 - 56 - 59  | 49 - 52 - 55  | 49 - 53 - 57  | 49 - 56 |
| 3  | 48  | 48 - 52 - 55  | 48 - 51 - 55  | 48 - 52 - 55 - 58  | 48 - 51 - 54  | 48 - 52 - 56  | 48 - 55 |
| 4  | 47  | 47 - 51 - 54  | 47 - 50 - 54  | 47 - 51 - 54 - 57  | 47 - 50 - 53  | 47 - 51 - 55  | 47 - 54 |
| 5  | 46  | 46 - 50 - 53  | 46 - 49 - 53  | 46 - 50 - 53 - 56  | 46 - 49 - 52  | 46 - 50 - 54  | 46 - 53 |
| 6  | 45  | 45 - 49 - 52  | 45 - 48 - 52  | 45 - 49 - 52 - 55  | 45 - 48 - 51  | 45 - 49 - 53  | 45 - 52 |
| 7  | 44  | 44 - 48 - 51  | 44 - 47 - 51  | 44 - 48 - 51 - 54  | 44 - 47 - 50  | 44 - 48 - 52  | 44 - 51 |
| 8  | 43  | 43 - 47 - 50  | 43 - 46 - 50  | 43 - 47 - 50 - 53  | 43 - 46 - 49  | 43 - 47 - 51  | 43 - 50 |
| 9  | 42  | 42 - 46 - 49  | 42 - 45 - 49  | 42 - 46 - 49 - 52  | 42 - 45 - 48  | 42 - 46 - 50  | 42 - 49 |
| 10 | 41  | 41 - 45 - 48  | 41 - 44 - 48  | 41 - 45 - 48 - 51  | 41 - 44 - 47  | 41 - 45 - 49  | 41 - 48 |
| 11 | 40  | 40 - 44 - 47  | 40 - 43 - 47  | 40 - 44 - 47 - 50  | 40 - 43 - 46  | 40 - 44 - 48  | 40 - 47 |
| 12 | 39  | 39 - 43 - 46  | 39 - 42 - 46  | 39 - 43 - 46 - 49  | 39 - 42 - 45  | 39 - 43 - 47  | 39 - 46 |
| 13 | 62  | 62 - 66 - 69  | 62 - 65 - 69  | 62 - 66 - 69 - 72  | 62 - 65 - 68  | 62 - 66 - 70  | 62 - 69 |
| 14 | 61  | 61 - 65 - 68  | 61 - 64 - 68  | 61 - 65 - 68 - 71  | 61 - 64 - 67  | 61 - 65 - 69  | 61 - 68 |
| 15 | 60  | 60 - 64 - 67  | 60 - 63 - 67  | 60 - 64 - 67 - 70  | 60 - 63 - 66  | 60 - 64 - 68  | 60 - 67 |
| 16 | 59  | 59 - 63 - 66  | 59 - 62 - 66  | 59 - 63 - 66 - 69  | 59 - 62 - 65  | 59 - 63 - 67  | 59 - 66 |
| 17 | 58  | 58 - 62 - 65  | 58 - 61 - 65  | 58 - 62 - 65 - 68  | 58 - 61 - 64  | 58 - 62 - 66  | 58 - 65 |
| 18 | 57  | 57 - 61 - 64  | 57 - 60 - 64  | 57 - 61 - 64 - 67  | 57 - 60 - 63  | 57 - 61 - 65  | 57 - 64 |
| 19 | 56  | 56 - 60 - 63  | 56 - 59 - 63  | 56 - 60 - 63 - 66  | 56 - 59 - 62  | 56 - 60 - 64  | 56 - 63 |
| 20 | 55  | 55 - 59 - 62  | 55 - 58 - 62  | 55 - 59 - 62 - 65  | 55 - 58 - 61  | 55 - 59 - 63  | 55 - 62 |
| 21 | 54  | 54 - 58 - 61  | 54 - 57 - 61  | 54 - 58 - 61 - 64  | 54 - 57 - 60  | 54 - 58 - 62  | 54 - 61 |
| 22 | 53  | 53 - 57 - 60  | 53 - 56 - 60  | 53 - 57 - 60 - 63  | 53 - 56 - 59  | 53 - 57 - 61  | 53 - 60 |
| 23 | 52  | 52 - 56 - 59  | 52 - 55 - 59  | 52 - 56 - 59 - 62  | 52 - 55 - 58  | 52 - 56 - 60  | 52 - 59 |
| 24 | 51  | 51 - 55 - 58  | 51 - 54 - 58  | 51 - 55 - 58 - 61  | 51 - 54 - 57  | 51 - 55 - 59  | 51 - 58 |

---

### Explication
- **Chaque servomoteur active une seule note MIDI pour une basse simple.**
- **Les accords sont joués en combinant plusieurs servomoteurs.**
- **Accords Majeurs et Mineurs utilisent 3 servos, les accords 7ème en utilisent 4.**
- **Les accords à 2 notes (fondamentale + quinte) utilisent 2 servos.**


## 🎛️ **Gestion MIDI**
| **Type de Note** | **Canal MIDI** | **Control Change (CC)** |
|-----------------|--------------|--------------------|
| **Basses Simples (24 notes)** | **Canal 2** | `CC 65 = 0` |
| **Accords Majeurs (36 notes)** | **Canal 2** | `CC 65 = 127`, `CC 64 = 0` |
| **Accords Mineurs (36 notes)** | **Canal 2** | `CC 65 = 127`, `CC 64 = 127` |
| **Accords à 2 Notes (Fondamentale + Quinte)** | **Canal 2** | `CC 66 = 127` |
| **Accords 7ème (Dominante 7)** | **Canal 2** | `CC 67 = 127` |
| **Accords Diminués** | **Canal 2** | `CC 68 = 127` |
| **Accords Augmentés** | **Canal 2** | `CC 69 = 127` |



## 📌 Gestion du Débit d’Air et du Soufflet

- ✔ Chaque note a un airFlowMultiplier (les graves consomment plus d'air).
- ✔ Le moteur ajuste sa vitesse en fonction des notes jouées.
- ✔ Si aucune note n’est active, le moteur s’arrête.
- ✔ Si trop de notes sont activées (max 15 servos), elles sont jouées par priorité.
- ✔ Alternance du sens d’ouverture/fermeture du soufflet :

    Si le soufflet dépasse 50% de son ouverture, le prochain cycle (nouvelle noteOn) se fait en sens inverse.

## 📌 Calibration et Sécurité

✔ Calibration automatique (calibrateZero) :

- Ouvre la valve principale.
- Recule jusqu’à la butée fermée (réinitialisation du zéro).
- Referme la valve et ajuste le point de départ.

✔ Détection des limites :

-  Fins de course physiques ou optiques.

## 📌 Optimisation des Performances

- ✔ Servos désactivés dynamiquement si inutilisés (OE des PCA9685).
- ✔ Alimentation optimisée pour éviter de surcharger le circuit 5V.
- ✔ Interpolation des commandes MIDI pour adoucir les transitions.
- ✔ Mode silencieux du TMC2209 (StealthChop) activé.


