> [!NOTE]
>  code non testé => il reste probablement du travail 

# 🎵 Accordion-Servo-MIDI 🎵

Transforme un accordéon acoustique en un instrument MIDI automatisé 🎹🎼

> [!NOTE]
>  je travaille avec un viel accordeon recupéré qui ne fonctionne plus, 
>  il a pris chaud (probablement dans une voiture), il y a des morceaux de cire partout et les anches ne tienent plus => Ca va me prendre du temps a remettre en etat avant de pouvoir tester :/

 # choses a faire :
- ajouter la gestion des pins oe de chaque pca pour desactiver les servos si non utilisé
- ajouter un code avec un capteur et gerer la pression en direct ?
- plans 2D des planches bois 
- plans 3D et stl des fichiers a imprimer
- liste completes des materiaux

## 📌 Objectif

Ce projet convertit un accordéon acoustique en un instrument MIDI piloté par des servomoteurs et un moteur pas à pas, permettant de :
- ✔ Lire et interpreter des messages MIDI via USB.
- ✔ Contrôler chaque note individuellement via des servos.
- ✔ Simuler le jeu d’un accordéoniste avec un soufflet dynamique.
- ✔ Gérer les notes et accords de la main droite et de la main gauche.
- ✔ Réguler automatiquement le débit d’air via airFlowMultiplier (sans capteur de pression).


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



