# Configurateur

Page autonome qui genere `accordionV06/config.h` : source d'air, actionneurs, MIDI,
tables de notes.

## Utilisation

Ouvrir `index.html` dans un navigateur — depuis le disque, sans serveur. Aucune connexion
reseau, aucune dependance externe : les trois fichiers (`index.html`, `styles.css`,
`app.js`) suffisent.

1. Choisir la **source d'air**. Les sections des systemes non retenus disparaissent : seuls
   les reglages qui influent reellement sur le firmware compile restent affiches.
2. Renseigner les actionneurs, le MIDI et les **tables de notes**.
3. Corriger les erreurs listees en bas de page.
4. Telecharger `config.h` et le deposer dans `accordionV06/`.

Le bouton « Telecharger config.h » reste desactive tant qu'une erreur subsiste. « Copier »
fonctionne toujours, pour inspecter le resultat.

## Ce que l'interface verifie

Elle rejoue les verifications de `settings.h` — celles qui refusent de compiler — et y
ajoute celles qu'une directive `#if` ne sait pas faire :

- broches posees sur SDA/SCL, ou partagees entre deux modules actifs ;
- broche sans PWM materiel pour une turbine modulee ;
- canaux PCA9685 reclames par deux organes a la fois — une anche et la valve generale, le
  servo de soufflet ou le canal ESC ;
- notes MIDI en double dans une meme main, debits d'air nuls, angles hors plage ;
- microstepping incompatible avec la vitesse demandee : a 400 pas/mm, le soufflet plafonne
  a 25 mm/s quoi qu'on demande a `STEPPER_MAX_SPEED` ;
- capteur de pression saturant en dessous du plafond de securite, qui ne serait alors
  jamais atteint ;
- point de partage MIDI rendant des anches injouables.

La carte d'occupation des canaux montre d'un coup d'oeil ce qui est pris, par qui, et ce
qui reste libre.

## Reprendre une configuration existante

La section *Fichier genere* relit un `config.h` deja ecrit, ou un profil JSON. La lecture
d'un `config.h` est tolerante : ce qui n'est pas reconnu garde sa valeur par defaut, et le
nombre de reglages effectivement lus est rapporte.

Le **profil JSON** conserve ce que `config.h` ne sait pas exprimer : le sens de montage
choisi ligne par ligne, et l'index du PCA plutot que son adresse. C'est le format a garder
pour archiver une machine ou la transmettre.

Le travail en cours est aussi conserve dans le navigateur (`localStorage`) et survit a un
rechargement de la page. Il reste local a ce navigateur : ce n'est pas une sauvegarde.

## Ajouter une option

Le formulaire, les verifications et le fichier genere sont tous produits a partir de la
constante `SCHEMA` dans `app.js`. Ajouter une option au firmware se fait en trois endroits :

1. `accordionV06/config.h` — la valeur par defaut et sa documentation ;
2. `accordionV06/settings.h` — la verification a la compilation, si elle est exprimable ;
3. `SCHEMA` dans `app.js` — une entree `{k, label, type, def, doc}`, plus `visible` si
   l'option ne concerne qu'une partie des configurations.

Le champ, son aide, sa ligne dans `config.h` et son marqueur « modifie » en decoulent.
