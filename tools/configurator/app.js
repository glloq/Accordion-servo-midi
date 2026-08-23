// =========================================================================================
//  Configurateur Accordion-servo-midi
// -----------------------------------------------------------------------------------------
//  Genere accordionV06/config.h a partir d'un formulaire.
//
//  PRINCIPE : le formulaire, les verifications et le fichier genere sont TOUS produits a
//  partir d'une seule description, SCHEMA. Ajouter une option au firmware = ajouter une
//  entree dans SCHEMA ; le champ, son aide, sa validation et sa ligne dans config.h en
//  decoulent. C'est ce qui evite qu'une option existe dans le firmware sans exister dans
//  l'interface, ou l'inverse.
//
//  Aucune dependance externe : la page fonctionne hors ligne, ouverte depuis le disque.
// =========================================================================================

'use strict';

// === Identifiants de airSourceTypes.h ====================================================
// Les valeurs numeriques ne servent qu'aux comparaisons internes : c'est le NOM du macro qui
// est ecrit dans config.h, jamais le nombre.
const AIR_SOURCES = {
    AIR_SOURCE_BELLOW_STEPPER: 0,
    AIR_SOURCE_BELLOW_SERVO: 1,
    AIR_SOURCE_BLOWER_PWM: 2,
    AIR_SOURCE_BLOWER_ESC: 3,
    AIR_SOURCE_PUMP_ONOFF: 4,
};

const is = (cfg, key, value) => cfg[key] === value;

// =========================================================================================
// DESCRIPTION DE LA CONFIGURATION
// =========================================================================================
// Chaque champ :
//   k        nom du macro dans config.h
//   label    libelle affiche
//   type     text | bool | int | float | enum | hexlist | pinlist
//   def      valeur par defaut (celle du depot)
//   doc      explication, reprise telle quelle en commentaire dans config.h
//   unit     unite affichee a droite du champ
//   guard    true  -> encadre par #ifndef, donc surchargeable par -D a la compilation
//   fmt      'truefalse' pour un booleen ecrit true/false plutot que 1/0
//   visible  fonction (cfg) -> bool : masque le champ quand il ne sert pas
//   min/max/step pour les champs numeriques

const SCHEMA = [
{
  id: 'machine', title: 'Machine',
  doc: "Identification de la configuration. Le nom n'a aucun effet sur le firmware : il sert\na s'y retrouver entre plusieurs instruments.",
  fields: [
    {k:'CONFIG_NAME', label:'Nom de la configuration', type:'text', def:'Accordeon 59 anches - soufflet pas a pas',
     doc:"Nom libre. Sert uniquement a s'y retrouver entre plusieurs machines."},
    {k:'DEBUG', label:'Traces de mise au point', type:'bool', def:false, fmt:'truefalse', guard:true,
     doc:"Traces sur le port serie USB. A laisser desactive en jeu : chaque Serial.print bloque\nla boucle principale et fait chuter la frequence de pas."},
  ]
},
{
  id: 'midi', title: 'MIDI',
  doc: "Transports actifs et attribution des notes aux deux mains.",
  fields: [
    {k:'MIDI_TRANSPORT_DIN', label:'Entree MIDI DIN (Serial1)', type:'bool', def:true, guard:true,
     doc:"MIDI serie classique sur Serial1 (broches 0/1 du Leonardo). C'est l'empreinte memoire\nla plus legere."},
    {k:'MIDI_TRANSPORT_USB', label:'Entree MIDI USB native', type:'bool', def:false, guard:true,
     doc:"MIDI USB natif du Leonardo/Micro. Necessite la bibliotheque USB-MIDI (lathoub).\nLes deux transports ensemble tiennent tout juste dans la SRAM d'un ATmega32U4."},
    {k:'MIDI_ROUTING', label:'Attribution des notes', type:'enum', def:'MIDI_ROUTING_CHANNEL', guard:true,
     options:[
       {v:'MIDI_ROUTING_CHANNEL', label:'Un canal MIDI par main',
        hint:"Le plus propre. Demande un sequenceur capable d'emettre sur deux canaux."},
       {v:'MIDI_ROUTING_SPLIT', label:'Point de partage sur la note',
        hint:"Tous canaux confondus. Permet de jouer un fichier MIDI mono-canal."},
       {v:'MIDI_ROUTING_MERGE', label:'Fusionne (main droite prioritaire)',
        hint:"Tous canaux. La main droite est essayee en premier, la gauche recupere le reste."},
     ],
     doc:"Comment une note entrante est attribuee a une main."},
    {k:'MIDI_CHANNEL_LEFT', label:'Canal main gauche', type:'int', def:1, min:1, max:16,
     visible:c=>is(c,'MIDI_ROUTING','MIDI_ROUTING_CHANNEL'),
     doc:"Canal MIDI dedie a la main gauche."},
    {k:'MIDI_CHANNEL_RIGHT', label:'Canal main droite', type:'int', def:2, min:1, max:16,
     visible:c=>is(c,'MIDI_ROUTING','MIDI_ROUTING_CHANNEL'),
     doc:"Canal MIDI dedie a la main droite."},
    {k:'MIDI_SPLIT_NOTE', label:'Note de partage', type:'int', def:54, min:0, max:127,
     visible:c=>is(c,'MIDI_ROUTING','MIDI_ROUTING_SPLIT'),
     doc:"Les notes strictement inferieures vont a la main gauche, les autres a la main droite."},
  ]
},
{
  id: 'poly', title: 'Polyphonie et velocite',
  doc: "La velocite n'agit pas sur les actionneurs — une valve d'anche est ouverte ou fermee —\nmais sur la demande d'air, note par note :\n  debit_total = somme( airFlow_i * poidsTenu(velocity_i) )\n  poidsTenu(v) = VELOCITY_SUSTAIN_MIN + (1 - VELOCITY_SUSTAIN_MIN) * v/127\nL'attaque est un supplement temporaire applique a la SEULE derniere note declenchee.",
  fields: [
    {k:'MAX_SIMULTANEOUS_NOTES', label:'Notes simultanees maximum', type:'int', def:15, min:1, max:64,
     doc:"Protection de l'alimentation. Au-dela, une note entrante ne peut voler que la place\nd'une note STRICTEMENT moins prioritaire, la plus ancienne d'abord."},
    {k:'VELOCITY_SUSTAIN_MIN', label:'Debit relatif a velocite 1', type:'float', def:0.60, min:0, max:1, step:0.05,
     doc:"Facteur de debit pour la velocite la plus faible. A 1.0, la velocite n'a plus d'effet."},
    {k:'VELOCITY_ATTACK_BOOST', label:"Surcroit d'attaque a velocite 127", type:'float', def:0.50, min:0, max:3, step:0.05,
     doc:"Supplement de debit pendant l'attaque, applique a la seule derniere note declenchee."},
    {k:'VELOCITY_ATTACK_MS', label:"Duree de l'attaque", type:'int', def:120, min:0, max:2000, unit:'ms', suffix:'UL',
     doc:"Duree de la phase d'attaque avant retour au niveau tenu."},
  ]
},
{
  id: 'actuators', title: 'Actionneurs des anches et bus I2C',
  doc: "Ce qui ouvre et ferme les valves d'anches, et le bus qui les commande.",
  fields: [
    {k:'NOTE_ACTUATOR', label:'Type d\'actionneur', type:'enum', def:'ACTUATOR_SERVO', guard:true,
     options:[
       {v:'ACTUATOR_SERVO', label:'Servomoteur', hint:"Angle ferme -> angle ouvert. Montage decrit dans le README."},
       {v:'ACTUATOR_SOLENOID', label:'Electroaimant', hint:"Sortie PCA tout-ou-rien, avec courant de maintien reduit."},
     ],
     doc:"Type d'actionneur des valves d'anches."},
    {k:'NUM_PCA_TOTAL', label:'Nombre de PCA9685', type:'int', def:4, min:1, max:16,
     doc:"Doit valoir exactement le nombre d'adresses listees ci-dessous."},
    {k:'PCA_ADDRESS_LIST', label:'Adresses I2C des PCA', type:'hexlist', def:'0x40, 0x41, 0x42, 0x43',
     doc:"Adresses des PCA9685, dans l'ordre. L'index 0 designe le premier de la liste."},
    {k:'I2C_CLOCK_HZ', label:'Frequence du bus I2C', type:'int', def:400000, min:50000, max:1000000, unit:'Hz', suffix:'L',
     doc:"Le PCA9685 supporte 1 MHz ; 400 kHz raccourcit nettement le temps de commande de\ndizaines de servos. Repasser a 100000 en cas de bus long ou bruite."},
    {k:'SERVO_I2C_ERROR_LIMIT', label:'Erreurs I2C tolerees', type:'int', def:8, min:1, max:255,
     doc:"Erreurs consecutives avant passage en defaut. Un PCA9685 qui cesse de repondre en\ncours de jeu laisse des anches ouvertes sans que rien ne le signale."},

    {k:'PCA_OE_MODE', label:'Pilotage de l\'OE', type:'enum', def:'PCA_OE_SHARED', guard:true,
     options:[
       {v:'PCA_OE_SHARED', label:'Une broche pour tous les PCA', hint:"Cablage le plus simple. La valve generale est coupee en meme temps que les anches."},
       {v:'PCA_OE_PER_PCA', label:'Une broche par PCA', hint:"Permet de couper les anches en gardant la valve alimentee."},
       {v:'PCA_OE_NONE', label:'OE cable a la masse', hint:"Les sorties ne sont jamais coupees."},
     ],
     doc:"OE coupe les sorties PWM : les servos cessent de forcer et l'instrument devient\nsilencieux. ATTENTION : OE ne coupe PAS le rail +5 V des servos. Une vraie mise hors\ntension demande un load switch / MOSFET high-side par banc."},
    {k:'PCA_OE_PIN', label:'Broche OE', type:'int', def:4, min:0, max:53,
     visible:c=>is(c,'PCA_OE_MODE','PCA_OE_SHARED'),
     doc:"Broche pilotant l'OE de tous les PCA9685 (actif bas)."},
    {k:'PCA_OE_PIN_LIST', label:'Broches OE (une par PCA)', type:'pinlist', def:'4, 7, 8, 12',
     visible:c=>is(c,'PCA_OE_MODE','PCA_OE_PER_PCA'),
     doc:"Une broche par PCA, dans le meme ordre que les adresses."},
    {k:'PCA_DISABLE_DELAY', label:"Delai avant coupure de l'OE", type:'int', def:500, min:0, max:10000, unit:'ms',
     doc:"Compte depuis la DERNIERE commande et non depuis la derniere note : un servo, la valve\ngenerale notamment, doit avoir le temps d'atteindre sa position avant la coupure."},

    {k:'SERVO_PWM_FREQUENCY', label:'Frequence PWM des servos', type:'int', def:50, min:24, max:1526, unit:'Hz',
     doc:"50 Hz convient a la majorite des servos analogiques."},
    {k:'SERVO_MIN_ANGLE', label:'Angle minimal admis', type:'int', def:0, min:0, max:180, unit:'deg',
     doc:"Bornage de securite : une entree de table erronee ne doit pas produire un PWM hors plage."},
    {k:'SERVO_MAX_ANGLE', label:'Angle maximal admis', type:'int', def:180, min:0, max:270, unit:'deg',
     doc:"Bornage de securite haut."},
    {k:'SERVO_MIN_PWM', label:'Comptes PCA a l\'angle minimal', type:'int', def:150, min:0, max:4095,
     doc:"Valeur PCA9685 correspondant a SERVO_MIN_ANGLE. A mesurer sur les servos reels."},
    {k:'SERVO_MAX_PWM', label:'Comptes PCA a l\'angle maximal', type:'int', def:600, min:0, max:4095,
     doc:"Valeur PCA9685 correspondant a SERVO_MAX_ANGLE."},
    {k:'SERVO_CLOSED_ANGLE', label:'Angle ferme, montage normal', type:'int', def:130, min:0, max:180, unit:'deg',
     visible:c=>is(c,'NOTE_ACTUATOR','ACTUATOR_SERVO'),
     doc:"Angle de repos des servos montes dans le sens normal."},
    {k:'SERVO_CLOSED_ANGLE_MIRROR', label:'Angle ferme, montage miroir', type:'int', def:50, min:0, max:180, unit:'deg',
     visible:c=>is(c,'NOTE_ACTUATOR','ACTUATOR_SERVO'),
     doc:"Angle de repos des servos montes en miroir."},
    {k:'SERVO_OPEN_ANGLE', label:"Debattement d'ouverture", type:'int', def:40, min:1, max:180, unit:'deg',
     visible:c=>is(c,'NOTE_ACTUATOR','ACTUATOR_SERVO'),
     doc:"Ecart applique pour ouvrir la valve. Montage normal : ferme - debattement.\nMontage miroir : ferme + debattement. Les deux doivent aboutir au meme angle ouvert."},
    {k:'SERVO_INIT_STAGGER_MS', label:'Espacement a l\'initialisation', type:'int', def:15, min:0, max:200, unit:'ms',
     doc:"Refermer des dizaines de servos au meme instant provoque un appel de courant que\nl'alimentation n'encaisse pas."},

    {k:'SOLENOID_PULLIN_MS', label:"Duree de l'appel", type:'int', def:40, min:1, max:1000, unit:'ms',
     visible:c=>is(c,'NOTE_ACTUATOR','ACTUATOR_SOLENOID'),
     doc:"Duree pendant laquelle l'electroaimant recoit la pleine puissance avant de retomber au\ncourant de maintien."},
    {k:'SOLENOID_HOLD_PERCENT', label:'Courant de maintien', type:'int', def:45, min:1, max:100, unit:'%',
     visible:c=>is(c,'NOTE_ACTUATOR','ACTUATOR_SOLENOID'),
     doc:"Un electroaimant maintenu a pleine puissance chauffe et finit par bruler."},
  ]
},
{
  id: 'valve', title: 'Valve generale (mise a l\'air libre)',
  doc: "Ouverte, la pression s'echappe : le soufflet peut bouger sans produire de son et une\nturbine tourne sans charge. C'est la position de securite — demarrage, calibration,\ndefaut, repos — et elle est fermee des la premiere note.",
  fields: [
    {k:'AIR_VALVE_TYPE', label:'Type de valve', type:'enum', def:'AIR_VALVE_SERVO', guard:true,
     options:[
       {v:'AIR_VALVE_SERVO', label:'Servo sur un canal PCA'},
       {v:'AIR_VALVE_SOLENOID', label:'Electrovanne sur une broche'},
       {v:'AIR_VALVE_NONE', label:'Aucune valve', hint:"Machine a turbine : la mise a l'air libre est obtenue en arretant la source."},
     ],
     doc:"Organe de mise a l'air libre."},
    {k:'VALVE_PCA_INDEX', label:'PCA portant la valve', type:'int', def:3, min:0, max:15,
     visible:c=>is(c,'AIR_VALVE_TYPE','AIR_VALVE_SERVO'),
     doc:"Index dans la liste des adresses (0 = premier PCA)."},
    {k:'VALVE_PCA_PIN', label:'Canal PCA', type:'int', def:15, min:0, max:15,
     visible:c=>is(c,'AIR_VALVE_TYPE','AIR_VALVE_SERVO'),
     doc:"Canal 0-15. Aucune note ne doit l'utiliser."},
    {k:'VALVE_PCA_ANGLE_OPEN', label:'Angle valve ouverte', type:'int', def:70, min:0, max:180, unit:'deg',
     visible:c=>is(c,'AIR_VALVE_TYPE','AIR_VALVE_SERVO'), doc:"Angle correspondant a la mise a l'air libre."},
    {k:'VALVE_PCA_ANGLE_CLOSE', label:'Angle valve fermee', type:'int', def:120, min:0, max:180, unit:'deg',
     visible:c=>is(c,'AIR_VALVE_TYPE','AIR_VALVE_SERVO'), doc:"Angle correspondant a la mise en pression."},
    {k:'VALVE_SOLENOID_PIN', label:'Broche de l\'electrovanne', type:'int', def:13, min:0, max:53,
     visible:c=>is(c,'AIR_VALVE_TYPE','AIR_VALVE_SOLENOID'), doc:"Sortie tout-ou-rien."},
    {k:'VALVE_SOLENOID_OPEN_LEVEL', label:'Niveau logique ouvrant', type:'enum', def:'1',
     options:[{v:'1', label:'HIGH'}, {v:'0', label:'LOW'}],
     visible:c=>is(c,'AIR_VALVE_TYPE','AIR_VALVE_SOLENOID'),
     doc:"Niveau a ecrire pour OUVRIR la valve. Depend de l'etage de puissance."},
  ]
},
{
  id: 'air', title: "Source d'air",
  doc: "Le choix est fait a la compilation : une seule implementation est embarquee. Les blocs\ndes autres sources restent dans le fichier pour pouvoir changer de systeme sans tout\nreecrire.",
  fields: [
    {k:'AIR_SOURCE', label:"Systeme de production d'air", type:'enum', def:'AIR_SOURCE_BELLOW_STEPPER', guard:true,
     options:[
       {v:'AIR_SOURCE_BELLOW_STEPPER', label:'Soufflet, moteur pas a pas',
        hint:"Le plus fidele : pression produite en poussant ET en tirant, inversion avant les fins de course. Seul systeme avec une course mesuree, donc une calibration."},
       {v:'AIR_SOURCE_BELLOW_SERVO', label:'Soufflet, servomoteur',
        hint:"Meme principe bidirectionnel, position commandee en angle. Ni fin de course ni homing. Course et force limitees."},
       {v:'AIR_SOURCE_BLOWER_PWM', label:'Turbine continue, PWM sur MOSFET',
        hint:"Unidirectionnel, pression continue. Ni course ni inversion. Simple, mais bruyant et sans nuance mecanique."},
       {v:'AIR_SOURCE_BLOWER_ESC', label:'Turbine brushless, ESC',
        hint:"Plus de debit. L'impulsion est produite par un canal PCA libre. Armement obligatoire au demarrage."},
       {v:'AIR_SOURCE_PUMP_ONOFF', label:'Pompe tout-ou-rien',
        hint:"Membrane ou piston avec reservoir tampon. Regulation par hysteresis (avec capteur) ou modulation lente."},
     ],
     doc:"Systeme produisant la pression d'air."},
    {k:'AIR_DIRECTION', label:'Sens de l\'air', type:'enum', def:'AIR_DIRECTION_BLOW', guard:true,
     options:[{v:'AIR_DIRECTION_BLOW', label:'Soufflage (surpression)'},
              {v:'AIR_DIRECTION_DRAW', label:'Aspiration (depression)'}],
     visible:c=>!isBellow(c),
     doc:"Pour les sources unidirectionnelles seulement. Change le signe attendu du capteur de\npression, et donc celui de la regulation."},
    {k:'AIR_INACTIVITY_TIMEOUT', label:'Mise au repos apres', type:'int', def:60000, min:1000, max:3600000, unit:'ms', suffix:'UL',
     doc:"Temps sans aucune note avant arret de la source d'air et ouverture de la valve."},
  ]
},
{
  id: 'stepper', title: 'Soufflet a moteur pas a pas',
  visible: c => is(c,'AIR_SOURCE','AIR_SOURCE_BELLOW_STEPPER'),
  doc: "Driver Step/Dir, transmission, course, vitesses, fins de course et calibration.",
  fields: [
    {k:'STEPPER_STEP_PIN', label:'Broche STEP', type:'int', def:10, min:0, max:53, doc:"Sortie d'impulsions de pas."},
    {k:'STEPPER_DIR_PIN', label:'Broche DIR', type:'int', def:9, min:0, max:53, doc:"Sortie de direction."},
    {k:'STEPPER_EN_PIN', label:'Broche ENABLE', type:'int', def:11, min:0, max:53, doc:"Sortie d'activation du driver."},
    {k:'STEPPER_EN_ACTIVE_LOW', label:'ENABLE actif bas', type:'bool', def:true,
     doc:"Cas de la quasi-totalite des drivers (A4988, DRV8825, TMC2209)."},
    {k:'STEPPER_INVERT_DIR', label:'Inverser le sens', type:'bool', def:false,
     doc:"Inversion logicielle, sans retoucher au cablage. Le firmware continue de raisonner\ndans le repere de la machine (0 = soufflet ferme)."},

    {k:'MOTOR_STEPS_PER_REV', label:'Pas par tour du moteur', type:'int', def:200, min:1, max:10000,
     doc:"NEMA17 1,8 deg => 200 pas/tour."},
    {k:'MICRO_STEP', label:'Micro-pas du driver', type:'enum', def:'16',
     options:[{v:'1',label:'1 (pas entier)'},{v:'2',label:'1/2'},{v:'4',label:'1/4'},{v:'8',label:'1/8'},
              {v:'16',label:'1/16'},{v:'32',label:'1/32'},{v:'64',label:'1/64'},{v:'128',label:'1/128'},{v:'256',label:'1/256'}],
     doc:"DOIT correspondre au reglage PHYSIQUE du driver (cavaliers MS1/MS2, ou configuration\nUART). En mode standalone, MS1=MS2=LOW donne souvent 1/8 et non 1/16 : une erreur ici\nfausse la course d'un facteur 2 a 16."},
    {k:'GEAR_RATIO', label:'Rapport de reduction', type:'float', def:2.0, min:0.01, max:100, step:0.1,
     doc:"Tours moteur pour un tour de sortie. Poulies 1/2 => 2.0."},
    {k:'BELLOW_TRANSMISSION', label:'Transmission', type:'enum', def:'TRANSMISSION_SCREW', guard:true,
     options:[{v:'TRANSMISSION_SCREW', label:'Tige filetee / vis'},
              {v:'TRANSMISSION_BELT', label:'Courroie et poulie'}],
     doc:"Conversion rotation -> translation."},
    {k:'SCREW_LEAD_MM', label:'Pas de la vis', type:'float', def:16.0, min:0.1, max:200, step:0.5, unit:'mm/tour',
     visible:c=>is(c,'BELLOW_TRANSMISSION','TRANSMISSION_SCREW'),
     doc:"Avance en millimetres par tour de vis."},
    {k:'BELT_PITCH_MM', label:'Pas de la courroie', type:'float', def:2.0, min:0.1, max:20, step:0.5, unit:'mm',
     visible:c=>is(c,'BELLOW_TRANSMISSION','TRANSMISSION_BELT'), doc:"GT2 = 2 mm."},
    {k:'BELT_PULLEY_TEETH', label:'Dents de la poulie', type:'int', def:20, min:1, max:200,
     visible:c=>is(c,'BELLOW_TRANSMISSION','TRANSMISSION_BELT'), doc:"Poulie motrice."},

    {k:'BELLOW_MIN_POSITION', label:'Position fermee', type:'int', def:0, min:0, max:1000, unit:'mm',
     doc:"Zero fixe par la calibration."},
    {k:'BELLOW_MAX_POSITION', label:'Ouverture maximale', type:'int', def:200, min:1, max:2000, unit:'mm',
     doc:"Course utile du soufflet, jusqu'au fin de course haut."},
    {k:'BELLOW_REVERSE_THRESHOLD_OPEN', label:"Inversion a l'ouverture", type:'float', def:0.7, min:0.05, max:0.99, step:0.05,
     doc:"Fraction de course a laquelle le soufflet repart en fermeture. Evalue en continu :\nune note tenue ne genere aucun evenement MIDI et doit malgre tout faire osciller le\nsoufflet."},
    {k:'BELLOW_REVERSE_THRESHOLD_CLOSE', label:'Inversion a la fermeture', type:'float', def:0.3, min:0.01, max:0.95, step:0.05,
     doc:"Fraction de course a laquelle le soufflet repart en ouverture."},

    {k:'NORMAL_SPEED', label:'Vitesse de reference', type:'int', def:10, min:1, max:300, unit:'mm/s',
     doc:"Vitesse pour une seule note a debit 1.0. C'est le reglage principal du volume sonore."},
    {k:'STEPPER_MIN_SPEED', label:'Vitesse minimale', type:'int', def:2, min:1, max:300, unit:'mm/s',
     doc:"Plancher de vitesse."},
    {k:'STEPPER_MAX_SPEED', label:'Vitesse maximale', type:'int', def:300, min:1, max:2000, unit:'mm/s',
     doc:"Borne haute de configuration. La vitesse reellement tenable est en plus limitee par la\nfrequence de pas (voir ci-dessous)."},
    {k:'STEPPER_MIN_ACCEL', label:'Acceleration minimale', type:'int', def:5, min:1, max:10000, unit:'mm/s2',
     doc:"Utilisee a faible debit : demarrage doux."},
    {k:'STEPPER_MAX_ACCEL', label:'Acceleration maximale', type:'int', def:100, min:1, max:100000, unit:'mm/s2',
     doc:"Utilisee a fort debit : attaque franche."},
    {k:'STEPPER_MAX_STEP_RATE_HZ', label:'Frequence de pas maximale', type:'float', def:10000.0, min:100, max:200000, step:500, unit:'Hz',
     doc:"Debit de pas reellement tenable par le MCU. FlexyStepper ne produit qu'UN pas par appel\na processMovement() : la frequence est bornee par la periode de la boucle principale, pas\nseulement par le CPU. test_timing mesure la valeur atteinte."},
    {k:'STEPPER_SERVICE_CALLS', label:'Appels au generateur par boucle', type:'int', def:4, min:1, max:32,
     doc:"Auto-limites par micros() : les appels en trop sont quasi gratuits, mais ils rattrapent\nles pas perdus pendant une rafale MIDI ou une ecriture I2C (~110 us)."},
    {k:'STEPPER_EMERGENCY_DECEL', label:"Deceleration d'arret d'urgence", type:'float', def:5000.0, min:100, max:1000000, step:500, unit:'mm/s2',
     doc:"Le driver etant deja coupe, cette rampe ne produit aucun mouvement : elle ramene l'etat\ninterne de FlexyStepper a l'arret, seule condition ou setCurrentPosition() est legitime."},
    {k:'STEPPER_STOP_TIMEOUT_MS', label:"Duree max de la sequence d'arret", type:'int', def:1000, min:10, max:60000, unit:'ms', suffix:'UL',
     doc:"Au-dela, defaut FAULT_STOP_TIMEOUT."},

    {k:'LIMIT_SWITCH_MIN_PIN', label:'Fin de course bas', type:'int', def:5, min:0, max:53,
     doc:"Butee de fermeture. Sert de reference au zero."},
    {k:'LIMIT_SWITCH_MAX_PIN', label:'Fin de course haut', type:'int', def:6, min:0, max:53,
     doc:"Butee d'ouverture maximale."},
    {k:'LIMIT_SWITCH_ACTIVE_LOW', label:'Contact actif bas', type:'bool', def:true,
     doc:"Capteur optique NPN, ou contact mecanique avec pull-up interne. Decocher pour un capteur\na sortie active haute."},
    {k:'ENDSTOP_DEBOUNCE_MS', label:'Debounce des fins de course', type:'int', def:50, min:1, max:500, unit:'ms',
     doc:"Ne retarde JAMAIS l'arret : le contact brut coupe immediatement le driver. Le debounce\nsert seulement a decider ensuite si le contact etait reel ou parasite."},

    {k:'HOMING_SPEED', label:'Approche rapide', type:'float', def:10.0, min:0.1, max:200, step:0.5, unit:'mm/s',
     doc:"Premiere passe : localise grossierement la butee."},
    {k:'HOMING_SLOW_SPEED', label:'Reapproche lente', type:'float', def:1.5, min:0.1, max:50, step:0.1, unit:'mm/s',
     doc:"Seconde passe : c'est elle qui fixe un zero repetable."},
    {k:'HOMING_BACKOFF_MM', label:'Degagement entre les passes', type:'float', def:4.0, min:0.1, max:100, step:0.5, unit:'mm',
     doc:"Sans degagement, la reapproche lente demarrerait sur un contact deja ferme."},
    {k:'HOMING_MAX_DISTANCE', label:'Course max de l\'approche', type:'float', def:250.0, min:1, max:2000, step:10, unit:'mm',
     doc:"Au-dela sans rencontrer la butee : defaut FAULT_HOMING_DISTANCE."},
    {k:'HOMING_TIMEOUT_MS', label:'Duree max de la calibration', type:'int', def:60000, min:1000, max:600000, unit:'ms', suffix:'UL',
     doc:"Couvre les deux passes."},
  ]
},
{
  id: 'servobellow', title: 'Soufflet a servomoteur',
  visible: c => is(c,'AIR_SOURCE','AIR_SOURCE_BELLOW_SERVO'),
  doc: "Le servo porte directement l'ouverture du soufflet. La demande d'air fixe la VITESSE de\nbalayage, et le sens s'inverse avant les angles extremes.",
  fields: [
    {k:'BELLOW_SERVO_PCA_INDEX', label:'PCA portant le servo', type:'int', def:3, min:0, max:15,
     doc:"Index dans la liste des adresses."},
    {k:'BELLOW_SERVO_PCA_PIN', label:'Canal PCA', type:'int', def:12, min:0, max:15,
     doc:"Canal 0-15. Aucune note ni la valve ne doivent l'utiliser."},
    {k:'BELLOW_SERVO_ANGLE_CLOSED', label:'Angle soufflet ferme', type:'int', def:20, min:0, max:180, unit:'deg',
     doc:"Peut etre superieur a l'angle ouvert si le montage est inverse."},
    {k:'BELLOW_SERVO_ANGLE_OPEN', label:'Angle soufflet ouvert', type:'int', def:160, min:0, max:180, unit:'deg', doc:"Fin de course mecanique du servo."},
    {k:'BELLOW_SERVO_MIN_SPEED_DPS', label:'Vitesse de balayage minimale', type:'float', def:5.0, min:0.1, max:500, step:0.5, unit:'deg/s',
     doc:"Vitesse pour une note seule. Equivalent de NORMAL_SPEED du soufflet pas a pas."},
    {k:'BELLOW_SERVO_MAX_SPEED_DPS', label:'Vitesse de balayage maximale', type:'float', def:120.0, min:1, max:1000, step:5, unit:'deg/s',
     doc:"Vitesse a pleine demande."},
    {k:'BELLOW_SERVO_DEMAND_FULL_SCALE', label:'Demande a pleine vitesse', type:'float', def:6.0, min:1.1, max:60, step:0.5,
     doc:"Demande d'air atteignant la vitesse maximale. Une note seule vaut environ 1.0."},
    {k:'BELLOW_SERVO_UPDATE_MS', label:'Periode de consigne', type:'int', def:20, min:5, max:200, unit:'ms',
     doc:"Envoyer une consigne a chaque tour de boucle saturerait le bus I2C : un servo analogique\nne rafraichit sa position qu'a la frequence PWM."},
    {k:'BELLOW_SERVO_REVERSE_OPEN', label:"Inversion a l'ouverture", type:'float', def:0.9, min:0.05, max:1.0, step:0.05,
     doc:"Fraction de course a laquelle le balayage repart en fermeture."},
    {k:'BELLOW_SERVO_REVERSE_CLOSE', label:'Inversion a la fermeture', type:'float', def:0.1, min:0.0, max:0.95, step:0.05,
     doc:"Fraction de course a laquelle le balayage repart en ouverture."},
  ]
},
{
  id: 'blowerpwm', title: 'Turbine PWM',
  visible: c => is(c,'AIR_SOURCE','AIR_SOURCE_BLOWER_PWM'),
  doc: "Ventilateur centrifuge ou soufflante continue sur MOSFET. Le rapport cyclique suit la\ndemande d'air.",
  fields: [
    {k:'BLOWER_PWM_PIN', label:'Broche PWM', type:'int', def:9, min:0, max:53,
     doc:"Doit etre une broche a PWM materiel."},
    {k:'BLOWER_PWM_INVERT', label:'Etage de puissance inverseur', type:'bool', def:false,
     doc:"A cocher pour un driver low-side PNP."},
    {k:'BLOWER_DUTY_MIN', label:'Duty a debit minimal', type:'int', def:60, min:0, max:255,
     doc:"Rapport cyclique produisant le debit utile le plus faible."},
    {k:'BLOWER_DUTY_MAX', label:'Duty a debit maximal', type:'int', def:255, min:1, max:255, doc:"Pleine puissance."},
    {k:'BLOWER_DUTY_IDLE', label:'Duty au repos', type:'int', def:0, min:0, max:255,
     doc:"Superieur a 0, la turbine reste en rotation lente : elle repart sans latence, au prix\nd'un bruit de fond permanent."},
    {k:'BLOWER_SPINUP_DUTY', label:"Duty de l'a-coup de demarrage", type:'int', def:200, min:0, max:255,
     doc:"Un moteur a l'arret ne demarre pas au duty minimal."},
    {k:'BLOWER_SPINUP_MS', label:"Duree de l'a-coup", type:'int', def:120, min:0, max:2000, unit:'ms',
     doc:"Duree de l'impulsion de demarrage avant retour a la commande utile."},
    {k:'BLOWER_DEMAND_FULL_SCALE', label:'Demande a pleine puissance', type:'float', def:6.0, min:0.1, max:60, step:0.5,
     doc:"Demande d'air correspondant au duty maximal. Au-dela le duty sature : c'est la limite\nde polyphonie utile de la turbine."},
  ]
},
{
  id: 'esc', title: 'Turbine brushless sur ESC',
  visible: c => is(c,'AIR_SOURCE','AIR_SOURCE_BLOWER_ESC'),
  doc: "L'ESC attend une impulsion type servo, produite par un canal libre d'un PCA9685. Un ESC\nnon arme ignore toute consigne : la sequence d'armement est obligatoire au demarrage.",
  fields: [
    {k:'ESC_PCA_INDEX', label:'PCA portant l\'ESC', type:'int', def:3, min:0, max:15, doc:"Index dans la liste des adresses."},
    {k:'ESC_PCA_PIN', label:'Canal PCA', type:'int', def:11, min:0, max:15,
     doc:"Canal 0-15. Aucune note ni la valve ne doivent l'utiliser."},
    {k:'ESC_PULSE_MIN_US', label:'Impulsion arret / armement', type:'int', def:1000, min:500, max:2500, unit:'us',
     doc:"Impulsion minimale, celle attendue pour l'armement."},
    {k:'ESC_PULSE_MAX_US', label:'Impulsion pleine puissance', type:'int', def:2000, min:500, max:2500, unit:'us', doc:"Plein gaz."},
    {k:'ESC_PULSE_IDLE_US', label:'Impulsion au repos', type:'int', def:1000, min:500, max:2500, unit:'us',
     doc:"Superieure au minimum pour garder la turbine lancee."},
    {k:'ESC_ARM_MS', label:"Duree de l'armement", type:'int', def:2500, min:100, max:20000, unit:'ms',
     doc:"Les notes sont refusees tant que l'armement n'est pas termine."},
    {k:'ESC_DEMAND_FULL_SCALE', label:'Demande a pleine puissance', type:'float', def:6.0, min:0.1, max:60, step:0.5,
     doc:"Demande d'air correspondant a l'impulsion maximale."},
  ]
},
{
  id: 'pump', title: 'Pompe tout-ou-rien',
  visible: c => is(c,'AIR_SOURCE','AIR_SOURCE_PUMP_ONOFF'),
  doc: "Pompe a membrane ou a piston, typiquement avec un reservoir tampon. Avec capteur :\nregulation par hysteresis. Sans capteur : modulation lente du rapport cyclique. Une pompe\nde ce type ne supporte pas la marche continue.",
  fields: [
    {k:'PUMP_PIN', label:'Broche de commande', type:'int', def:8, min:0, max:53,
     doc:"Sortie tout-ou-rien : aucun timer necessaire."},
    {k:'PUMP_ACTIVE_LEVEL', label:'Niveau logique de marche', type:'enum', def:'1',
     options:[{v:'1', label:'HIGH'}, {v:'0', label:'LOW'}],
     doc:"Niveau qui met la pompe en marche. Le firmware ecrit explicitement le niveau inactif au\ndemarrage : s'en remettre a l'etat par defaut de la broche ferait demarrer la pompe des\nla mise sous tension avec un etage inverseur."},
    {k:'PUMP_HYSTERESIS_KPA', label:'Bande morte (avec capteur)', type:'float', def:0.4, min:0.01, max:20, step:0.1, unit:'kPa',
     doc:"Demi-largeur de la bande d'hysteresis autour de la consigne."},
    {k:'PUMP_CYCLE_MS', label:'Periode de modulation', type:'int', def:200, min:20, max:5000, unit:'ms', suffix:'UL',
     doc:"Sans capteur. Une pompe a membrane ne suit pas un PWM rapide : ses clapets ont une\ninertie mecanique."},
    {k:'PUMP_MIN_DUTY_PERCENT', label:'Duty a debit minimal', type:'int', def:20, min:1, max:100, unit:'%', doc:"Rapport cyclique le plus bas."},
    {k:'PUMP_MAX_DUTY_PERCENT', label:'Duty a debit maximal', type:'int', def:100, min:1, max:100, unit:'%', doc:"Rapport cyclique le plus haut."},
    {k:'PUMP_DEMAND_FULL_SCALE', label:'Demande a plein debit', type:'float', def:6.0, min:0.1, max:60, step:0.5,
     doc:"Demande d'air correspondant au duty maximal."},
    {k:'PUMP_MAX_RUN_MS', label:'Marche continue maximale', type:'int', def:30000, min:1000, max:600000, unit:'ms', suffix:'UL',
     doc:"Protection thermique : au-dela, repos force meme si la demande persiste."},
    {k:'PUMP_REST_MS', label:'Duree du repos force', type:'int', def:5000, min:100, max:600000, unit:'ms', suffix:'UL',
     doc:"Duree pendant laquelle la pompe reste arretee apres un depassement."},
  ]
},
{
  id: 'pressure', title: 'Capteur de pression et regulation',
  doc: "Sans capteur, tout fonctionne en boucle ouverte : la demande d'air pilote directement la\nvitesse ou le rapport cyclique, et la pression reelle depend de l'etancheite, du nombre\nd'anches ouvertes et de l'usure. Avec capteur, la demande devient une CONSIGNE de pression\net le correcteur CORRIGE la commande calculee en boucle ouverte, sans la remplacer.",
  fields: [
    {k:'PRESSURE_SENSOR_ENABLED', label:'Capteur de pression monte', type:'bool', def:false, guard:true,
     doc:"Sans capteur, toute la classe de regulation disparait a la compilation."},
    {k:'PRESSURE_SENSOR_PIN', label:'Entree analogique', type:'text', def:'A0',
     visible:c=>c.PRESSURE_SENSOR_ENABLED, doc:"Broche analogique du capteur."},
    {k:'PRESSURE_ADC_AT_ZERO', label:'Comptes ADC a pression nulle', type:'int', def:102, min:0, max:1023,
     visible:c=>c.PRESSURE_SENSOR_ENABLED,
     doc:"Les capteurs de type MPX5010 sortent 0,2 a 0,5 V a pression nulle : le zero du capteur\nn'est pas le zero de l'ADC. A mesurer, valve ouverte."},
    {k:'PRESSURE_ADC_PER_KPA', label:'Comptes ADC par kPa', type:'float', def:40.9, min:0.1, max:1000, step:0.1,
     visible:c=>c.PRESSURE_SENSOR_ENABLED,
     doc:"Sensibilite. MPX5010 : 4,5 V pleine echelle a 10 kPa sur 1024 comptes => 40,9."},
    {k:'PRESSURE_FILTER_ALPHA', label:'Filtre passe-bas', type:'float', def:0.25, min:0.01, max:1.0, step:0.05,
     visible:c=>c.PRESSURE_SENSOR_ENABLED, doc:"Premier ordre. 1.0 = mesure brute, plus bas = plus lisse et plus lent."},
    {k:'PRESSURE_SAMPLE_MS', label:"Periode d'echantillonnage", type:'int', def:5, min:1, max:200, unit:'ms', suffix:'UL',
     visible:c=>c.PRESSURE_SENSOR_ENABLED, doc:"Periode de lecture et de calcul du correcteur."},
    {k:'PRESSURE_TARGET_KPA', label:'Consigne pour une demande de 1.0', type:'float', def:2.5, min:0.1, max:100, step:0.1, unit:'kPa',
     visible:c=>c.PRESSURE_SENSOR_ENABLED, doc:"Pression visee pour une note seule."},
    {k:'PRESSURE_MAX_KPA', label:'Plafond de securite', type:'float', def:8.0, min:0.2, max:200, step:0.5, unit:'kPa',
     visible:c=>c.PRESSURE_SENSOR_ENABLED,
     doc:"Au-dela : defaut FAULT_OVERPRESSURE, source coupee et valve ouverte. Signale une fuite\nbouchee, une valve bloquee fermee ou une anche coincee."},
    {k:'PRESSURE_CONTROL', label:'Correcteur', type:'enum', def:'PRESSURE_CONTROL_PI', guard:true,
     options:[
       {v:'PRESSURE_CONTROL_OPEN_LOOP', label:'Boucle ouverte (surveillance seule)',
        hint:"Le capteur ne sert qu'a detecter la surpression."},
       {v:'PRESSURE_CONTROL_PI', label:'Proportionnel + integral', hint:"Le choix par defaut."},
       {v:'PRESSURE_CONTROL_PID', label:'Proportionnel + integral + derive',
        hint:"A n'utiliser que si le PI seul est trop lent : la derivee amplifie le bruit du capteur."},
     ],
     visible:c=>c.PRESSURE_SENSOR_ENABLED, doc:"Type de correcteur."},
    {k:'PRESSURE_KP', label:'Gain proportionnel', type:'float', def:0.35, min:0, max:20, step:0.05,
     visible:c=>c.PRESSURE_SENSOR_ENABLED && !is(c,'PRESSURE_CONTROL','PRESSURE_CONTROL_OPEN_LOOP'),
     doc:"L'erreur est RELATIVE a la consigne : les gains sont donc independants du niveau de\npression vise."},
    {k:'PRESSURE_KI', label:'Gain integral', type:'float', def:0.80, min:0, max:20, step:0.05, unit:'/s',
     visible:c=>c.PRESSURE_SENSOR_ENABLED && !is(c,'PRESSURE_CONTROL','PRESSURE_CONTROL_OPEN_LOOP'),
     doc:"Rattrape l'erreur permanente."},
    {k:'PRESSURE_KD', label:'Gain derive', type:'float', def:0.0, min:0, max:20, step:0.01, unit:'s',
     visible:c=>c.PRESSURE_SENSOR_ENABLED && is(c,'PRESSURE_CONTROL','PRESSURE_CONTROL_PID'),
     doc:"Ignore si le correcteur n'est pas un PID."},
    {k:'PRESSURE_INTEGRAL_LIMIT', label:"Bornage de l'integrateur", type:'float', def:1.0, min:0.01, max:20, step:0.1,
     visible:c=>c.PRESSURE_SENSOR_ENABLED && !is(c,'PRESSURE_CONTROL','PRESSURE_CONTROL_OPEN_LOOP'),
     doc:"Anti-emballement. Sans lui, une fuite ferait partir l'integrateur a l'infini et la\nreprise de pression provoquerait un a-coup violent."},
    {k:'PRESSURE_SCALE_MIN', label:'Facteur correctif minimal', type:'float', def:0.25, min:0.01, max:0.99, step:0.05,
     visible:c=>c.PRESSURE_SENSOR_ENABLED && !is(c,'PRESSURE_CONTROL','PRESSURE_CONTROL_OPEN_LOOP'),
     doc:"Borne basse du facteur applique a la commande en boucle ouverte."},
    {k:'PRESSURE_SCALE_MAX', label:'Facteur correctif maximal', type:'float', def:2.50, min:1.01, max:20, step:0.1,
     visible:c=>c.PRESSURE_SENSOR_ENABLED && !is(c,'PRESSURE_CONTROL','PRESSURE_CONTROL_OPEN_LOOP'),
     doc:"Borne haute. Laisser le correcteur aller a l'infini transformerait une derive de\ncapteur en emballement mecanique."},
  ]
},
];

function isBellow(cfg) {
    return is(cfg,'AIR_SOURCE','AIR_SOURCE_BELLOW_STEPPER') || is(cfg,'AIR_SOURCE','AIR_SOURCE_BELLOW_SERVO');
}

// =========================================================================================
// TABLES DE NOTES
// =========================================================================================
// Une ligne = une anche pilotee. `pca` est un INDEX dans la liste d'adresses et non une
// adresse : renumeroter les PCA ne casse donc pas le mapping. `mount` porte le sens de
// montage du servo, d'ou sont deduits l'angle ferme et l'angle ouvert.

const PRIORITIES = [
    {v:'NOTE_PRIORITY_BASS',   label:'Basse (3)'},
    {v:'NOTE_PRIORITY_MELODY', label:'Melodie (2)'},
    {v:'NOTE_PRIORITY_CHORD',  label:'Accord (1)'},
];

const NOTE_NAMES = ['Do','Do#','Re','Re#','Mi','Fa','Fa#','Sol','Sol#','La','La#','Si'];
function noteName(midi) {
    if (midi < 0 || midi > 127) return '?';
    return NOTE_NAMES[midi % 12] + (Math.floor(midi / 12) - 1);
}

function makeRow(note, pca, ch, flow, mount, prio) {
    return {note, pca, ch, flow, mount, angle: 130, dir: true, prio};
}

// Mapping par defaut : celui du depot (accordeon 59 anches du README).
function defaultRightHand() {
    const rows = [];
    // 18 servos en montage normal, puis 16 en miroir. Le debit decroit avec la hauteur :
    // les graves consomment plus d'air.
    const flows = [1.00,0.98,0.96,0.94,0.92,0.90,0.88,0.86,0.84,0.82,0.80,0.78,0.76,0.74,0.72,0.70,0.68,0.66,
                   0.66,0.66,0.64,0.62,0.60,0.58,0.56,0.54,0.52,0.50,0.48,0.46,0.44,0.42,0.40,0.38];
    for (let i = 0; i < 16; i++) rows.push(makeRow(54+i, 0, i, flows[i], 'normal', 'NOTE_PRIORITY_MELODY'));
    rows.push(makeRow(70, 3, 13, flows[16], 'normal', 'NOTE_PRIORITY_MELODY'));
    rows.push(makeRow(71, 3, 14, flows[17], 'normal', 'NOTE_PRIORITY_MELODY'));
    for (let i = 0; i < 16; i++) rows.push(makeRow(72+i, 1, i, flows[18+i], 'mirror', 'NOTE_PRIORITY_MELODY'));
    return rows;
}

function defaultLeftHand() {
    const bass  = [36,43,38,45,40,47,42,49,44,51,46,41];
    const chord = [48,55,50,57,52,59,54,61,56,63,58,53];
    const rows = [];
    for (let i = 0; i < 12; i++) rows.push(makeRow(bass[i], 2, i, 1.0, 'normal', 'NOTE_PRIORITY_BASS'));
    for (let i = 0; i < 4; i++)  rows.push(makeRow(chord[i], 2, 12+i, 1.0, 'mirror', 'NOTE_PRIORITY_CHORD'));
    for (let i = 4; i < 12; i++) rows.push(makeRow(chord[i], 3, i-3, 1.0, 'mirror', 'NOTE_PRIORITY_CHORD'));
    return rows;
}

// =========================================================================================
// ETAT
// =========================================================================================
const state = { cfg: {}, right: [], left: [] };

function defaultConfig() {
    const cfg = {};
    for (const section of SCHEMA) for (const f of section.fields) cfg[f.k] = f.def;
    return cfg;
}

function resetAll() {
    state.cfg = defaultConfig();
    state.right = defaultRightHand();
    state.left = defaultLeftHand();
}

function fieldByKey(key) {
    for (const section of SCHEMA) for (const f of section.fields) if (f.k === key) return f;
    return null;
}

function pcaAddresses() {
    return String(state.cfg.PCA_ADDRESS_LIST || '')
        .split(',').map(s => s.trim()).filter(s => s.length);
}

// Angle ferme et angle ouvert effectifs d'une ligne, selon son montage.
function rowAngles(row) {
    const open = Number(state.cfg.SERVO_OPEN_ANGLE);
    if (row.mount === 'normal') {
        const closed = Number(state.cfg.SERVO_CLOSED_ANGLE);
        return {closed, opened: closed - open, dir: true};
    }
    if (row.mount === 'mirror') {
        const closed = Number(state.cfg.SERVO_CLOSED_ANGLE_MIRROR);
        return {closed, opened: closed + open, dir: false};
    }
    const closed = Number(row.angle);
    return {closed, opened: row.dir ? closed - open : closed + open, dir: !!row.dir};
}

// Expression ecrite dans config.h pour l'angle ferme : le nom symbolique quand il s'agit
// d'un montage standard, un litteral sinon.
function rowClosedToken(row) {
    if (row.mount === 'normal') return 'SERVO_CLOSED_ANGLE';
    if (row.mount === 'mirror') return 'SERVO_CLOSED_ANGLE_MIRROR';
    return String(row.angle);
}

// =========================================================================================
// VERIFICATIONS
// =========================================================================================
// Elles reproduisent celles de settings.h — qui refusent de compiler — et y ajoutent celles
// qu'un #if ne sait pas faire : collisions de canaux, doublons de notes, coherence entre
// microstepping et vitesse demandee.

function validate() {
    const cfg = state.cfg;
    const errors = [];
    const warnings = [];
    const err = (id, msg) => errors.push({id, msg});
    const warn = (id, msg) => warnings.push({id, msg});

    const sda = 2, scl = 3; // Leonardo / Micro
    const isI2cPin = p => (p === sda || p === scl);

    // --- MIDI ---
    if (!cfg.MIDI_TRANSPORT_DIN && !cfg.MIDI_TRANSPORT_USB)
        err('midi', "Aucun transport MIDI actif : l'instrument ne recevrait jamais de note.");
    if (cfg.MIDI_TRANSPORT_DIN && cfg.MIDI_TRANSPORT_USB)
        warn('midi', "DIN et USB ensemble tiennent tout juste dans la SRAM d'un ATmega32U4. " +
                     "L'environnement leonardo_din_usb est marque experimental.");

    // --- PCA ---
    const addrs = pcaAddresses();
    if (addrs.length !== Number(cfg.NUM_PCA_TOTAL))
        err('actuators', `NUM_PCA_TOTAL vaut ${cfg.NUM_PCA_TOTAL} mais ${addrs.length} adresse(s) sont listees.`);
    const seenAddr = new Set();
    for (const a of addrs) {
        const v = parseInt(a, 16);
        if (isNaN(v) && isNaN(Number(a))) { err('actuators', `Adresse I2C illisible : "${a}".`); continue; }
        const n = a.toLowerCase().startsWith('0x') ? parseInt(a, 16) : Number(a);
        if (n < 0x40 || n > 0x7f)
            warn('actuators', `L'adresse ${a} est hors de la plage habituelle du PCA9685 (0x40-0x7F).`);
        if (seenAddr.has(n)) err('actuators', `Adresse I2C ${a} declaree deux fois.`);
        seenAddr.add(n);
    }
    if (Number(cfg.SERVO_MIN_PWM) >= Number(cfg.SERVO_MAX_PWM))
        err('actuators', 'SERVO_MIN_PWM doit etre strictement inferieur a SERVO_MAX_PWM.');
    if (Number(cfg.SERVO_MIN_ANGLE) >= Number(cfg.SERVO_MAX_ANGLE))
        err('actuators', 'SERVO_MIN_ANGLE doit etre strictement inferieur a SERVO_MAX_ANGLE.');

    if (cfg.NOTE_ACTUATOR === 'ACTUATOR_SERVO') {
        const n = Number(cfg.SERVO_CLOSED_ANGLE) - Number(cfg.SERVO_OPEN_ANGLE);
        const m = Number(cfg.SERVO_CLOSED_ANGLE_MIRROR) + Number(cfg.SERVO_OPEN_ANGLE);
        if (n !== m)
            warn('actuators', `Montages normal et miroir n'ouvrent pas au meme angle (${n} vs ${m} deg) : ` +
                              `les deux blocs d'anches ne s'ouvriront pas pareil.`);
        if (n < Number(cfg.SERVO_MIN_ANGLE) || m > Number(cfg.SERVO_MAX_ANGLE))
            err('actuators', "Un angle d'ouverture sort de la plage servo admise.");
    }

    // --- Broches ---
    // On ne confronte que les broches de la configuration REELLEMENT compilee : les
    // parametres des sources inutilisees n'ont pas a etre coherents.
    const pins = []; // {pin, who, section}
    const addPin = (pin, who, section) => { if (pin !== null && pin !== undefined && pin !== '') pins.push({pin:Number(pin), who, section}); };

    if (cfg.PCA_OE_MODE === 'PCA_OE_SHARED') addPin(cfg.PCA_OE_PIN, 'OE des PCA', 'actuators');
    if (cfg.PCA_OE_MODE === 'PCA_OE_PER_PCA') {
        const list = String(cfg.PCA_OE_PIN_LIST || '').split(',').map(s => s.trim()).filter(s => s.length);
        if (list.length !== Number(cfg.NUM_PCA_TOTAL))
            err('actuators', `PCA_OE_PIN_LIST contient ${list.length} broche(s) pour ${cfg.NUM_PCA_TOTAL} PCA.`);
        list.forEach((p, i) => addPin(p, `OE du PCA ${i}`, 'actuators'));
    }
    if (cfg.AIR_VALVE_TYPE === 'AIR_VALVE_SOLENOID') addPin(cfg.VALVE_SOLENOID_PIN, 'Electrovanne', 'valve');

    if (cfg.AIR_SOURCE === 'AIR_SOURCE_BELLOW_STEPPER') {
        addPin(cfg.STEPPER_STEP_PIN, 'STEP', 'stepper');
        addPin(cfg.STEPPER_DIR_PIN, 'DIR', 'stepper');
        addPin(cfg.STEPPER_EN_PIN, 'ENABLE', 'stepper');
        addPin(cfg.LIMIT_SWITCH_MIN_PIN, 'Fin de course bas', 'stepper');
        addPin(cfg.LIMIT_SWITCH_MAX_PIN, 'Fin de course haut', 'stepper');
    } else if (cfg.AIR_SOURCE === 'AIR_SOURCE_BLOWER_PWM') {
        addPin(cfg.BLOWER_PWM_PIN, 'PWM de la turbine', 'blowerpwm');
    } else if (cfg.AIR_SOURCE === 'AIR_SOURCE_PUMP_ONOFF') {
        addPin(cfg.PUMP_PIN, 'Commande de la pompe', 'pump');
    }

    for (const p of pins) {
        if (isI2cPin(p.pin))
            err(p.section, `${p.who} est sur la broche D${p.pin}, qui est ${p.pin === sda ? 'SDA' : 'SCL'} ` +
                           `sur Leonardo/Micro : le bus des PCA9685 serait perdu.`);
    }
    for (let i = 0; i < pins.length; i++)
        for (let j = i + 1; j < pins.length; j++)
            if (pins[i].pin === pins[j].pin)
                err(pins[j].section, `${pins[i].who} et ${pins[j].who} partagent la broche D${pins[i].pin}.`);

    // Broches a PWM materiel du Leonardo / Micro. analogWrite() sur une autre broche ne
    // module rien : elle bascule simplement a 0 ou 255.
    const LEONARDO_PWM = [3, 5, 6, 9, 10, 11, 13];
    if (cfg.AIR_SOURCE === 'AIR_SOURCE_BLOWER_PWM' && !LEONARDO_PWM.includes(Number(cfg.BLOWER_PWM_PIN)))
        warn('blowerpwm', `D${cfg.BLOWER_PWM_PIN} n'a pas de PWM materiel sur Leonardo/Micro ` +
                          `(3, 5, 6, 9, 10, 11, 13) : la turbine ne serait pas modulable.`);

    // --- Canaux PCA ---
    const occupancy = new Map(); // "pca:ch" -> description
    const claim = (pcaIndex, ch, who, section) => {
        if (pcaIndex >= addrs.length)
            { err(section, `${who} designe le PCA d'index ${pcaIndex}, hors de la liste (${addrs.length} declares).`); return; }
        if (ch < 0 || ch > 15) { err(section, `${who} utilise le canal ${ch}, hors de la plage 0-15.`); return; }
        const key = pcaIndex + ':' + ch;
        if (occupancy.has(key))
            err(section, `Canal ${ch} du PCA ${pcaIndex} (${addrs[pcaIndex]}) reclame par ${occupancy.get(key)} ET ${who}.`);
        else occupancy.set(key, who);
    };

    const seenNote = {right: new Set(), left: new Set()};
    for (const [handName, rows, label] of [['right', state.right, 'main droite'], ['left', state.left, 'main gauche']]) {
        rows.forEach((row, i) => {
            if (seenNote[handName].has(row.note))
                err('notes', `Note MIDI ${row.note} (${noteName(row.note)}) presente deux fois en ${label}.`);
            seenNote[handName].add(row.note);
            if (row.note < 0 || row.note > 127) err('notes', `Note MIDI ${row.note} hors plage (0-127).`);
            if (!(Number(row.flow) > 0)) err('notes', `Debit d'air nul ou negatif pour la note ${row.note} (${label}).`);
            claim(row.pca, row.ch, `${label} ligne ${i + 1} (note ${row.note})`, 'notes');
            if (cfg.NOTE_ACTUATOR === 'ACTUATOR_SERVO') {
                const a = rowAngles(row);
                if (a.opened < 0 || a.opened > 180 || a.closed < 0 || a.closed > 180)
                    err('notes', `Angles hors plage pour la note ${row.note} (${label}) : ferme ${a.closed}, ouvert ${a.opened}.`);
            }
        });
    }
    if (state.right.length + state.left.length === 0)
        err('notes', "L'instrument n'a aucune note : au moins une anche doit etre pilotee.");

    if (cfg.AIR_VALVE_TYPE === 'AIR_VALVE_SERVO')
        claim(Number(cfg.VALVE_PCA_INDEX), Number(cfg.VALVE_PCA_PIN), 'la valve generale', 'valve');
    if (cfg.AIR_SOURCE === 'AIR_SOURCE_BELLOW_SERVO')
        claim(Number(cfg.BELLOW_SERVO_PCA_INDEX), Number(cfg.BELLOW_SERVO_PCA_PIN), 'le servo de soufflet', 'servobellow');
    if (cfg.AIR_SOURCE === 'AIR_SOURCE_BLOWER_ESC')
        claim(Number(cfg.ESC_PCA_INDEX), Number(cfg.ESC_PCA_PIN), "l'ESC", 'esc');

    if (state.right.length + state.left.length > Number(cfg.NUM_PCA_TOTAL) * 16)
        err('notes', `${state.right.length + state.left.length} notes pour ${cfg.NUM_PCA_TOTAL} PCA ` +
                     `(${Number(cfg.NUM_PCA_TOTAL) * 16} canaux) : il en manque.`);

    if (Number(cfg.MAX_SIMULTANEOUS_NOTES) > state.right.length + state.left.length)
        warn('poly', "MAX_SIMULTANEOUS_NOTES depasse le nombre d'anches : la limite ne servira jamais.");

    // --- Routage ---
    if (cfg.MIDI_ROUTING === 'MIDI_ROUTING_CHANNEL' && cfg.MIDI_CHANNEL_LEFT === cfg.MIDI_CHANNEL_RIGHT)
        err('midi', 'Les deux mains sont sur le meme canal MIDI : le routage par canal ne peut pas les distinguer.');
    if (cfg.MIDI_ROUTING === 'MIDI_ROUTING_SPLIT') {
        const badLeft = state.left.filter(r => r.note >= Number(cfg.MIDI_SPLIT_NOTE)).length;
        const badRight = state.right.filter(r => r.note < Number(cfg.MIDI_SPLIT_NOTE)).length;
        if (badLeft || badRight)
            warn('midi', `Le point de partage (${cfg.MIDI_SPLIT_NOTE}) rend injouables ${badLeft} note(s) de la ` +
                         `main gauche et ${badRight} de la main droite : elles sont routees vers la mauvaise main.`);
    }

    // --- Soufflet pas a pas ---
    if (cfg.AIR_SOURCE === 'AIR_SOURCE_BELLOW_STEPPER') {
        const travel = cfg.BELLOW_TRANSMISSION === 'TRANSMISSION_SCREW'
            ? Number(cfg.SCREW_LEAD_MM)
            : Number(cfg.BELT_PITCH_MM) * Number(cfg.BELT_PULLEY_TEETH);
        const stepsPerMm = (Number(cfg.MOTOR_STEPS_PER_REV) * Number(cfg.MICRO_STEP) * Number(cfg.GEAR_RATIO)) / travel;
        const rateLimited = Number(cfg.STEPPER_MAX_STEP_RATE_HZ) / stepsPerMm;
        state.derived = {stepsPerMm, rateLimited};

        if (!(travel > 0)) err('stepper', 'La transmission donne une avance nulle ou negative par tour.');
        if (Number(cfg.BELLOW_MAX_POSITION) <= Number(cfg.BELLOW_MIN_POSITION))
            err('stepper', 'BELLOW_MAX_POSITION doit depasser BELLOW_MIN_POSITION.');
        if (Number(cfg.STEPPER_MIN_SPEED) >= Number(cfg.STEPPER_MAX_SPEED))
            err('stepper', 'STEPPER_MIN_SPEED doit etre strictement inferieure a STEPPER_MAX_SPEED.');
        if (!(Number(cfg.BELLOW_REVERSE_THRESHOLD_CLOSE) > 0 &&
              Number(cfg.BELLOW_REVERSE_THRESHOLD_CLOSE) < Number(cfg.BELLOW_REVERSE_THRESHOLD_OPEN) &&
              Number(cfg.BELLOW_REVERSE_THRESHOLD_OPEN) < 1))
            err('stepper', "Les seuils d'inversion doivent verifier 0 < fermeture < ouverture < 1.");
        if (Number(cfg.HOMING_MAX_DISTANCE) <= Number(cfg.HOMING_BACKOFF_MM))
            err('stepper', 'HOMING_MAX_DISTANCE doit depasser le degagement.');
        if (Number(cfg.HOMING_MAX_DISTANCE) < Number(cfg.BELLOW_MAX_POSITION))
            warn('stepper', "HOMING_MAX_DISTANCE est inferieure a la course du soufflet : si le soufflet " +
                            "demarre ouvert, la calibration abandonnera avant d'atteindre la butee.");
        if (rateLimited < Number(cfg.NORMAL_SPEED))
            err('stepper', `A ${stepsPerMm.toFixed(0)} pas/mm, la vitesse maximale tenable est ` +
                           `${rateLimited.toFixed(1)} mm/s, en dessous de NORMAL_SPEED (${cfg.NORMAL_SPEED} mm/s) : ` +
                           `meme une note seule ferait decrocher le moteur. Reduire le microstepping.`);
        else if (rateLimited < Number(cfg.NORMAL_SPEED) * 3)
            warn('stepper', `A ${stepsPerMm.toFixed(0)} pas/mm, la vitesse plafonne a ${rateLimited.toFixed(1)} mm/s, ` +
                            `soit ${(rateLimited / Number(cfg.NORMAL_SPEED)).toFixed(1)}x NORMAL_SPEED : les accords ` +
                            `satureront le debit d'air. Reduire MICRO_STEP pour gagner de la vitesse.`);
        if (Number(cfg.HOMING_SPEED) > rateLimited)
            err('stepper', `HOMING_SPEED (${cfg.HOMING_SPEED} mm/s) depasse la vitesse tenable ` +
                           `(${rateLimited.toFixed(1)} mm/s) : la calibration decrocherait.`);
    } else {
        state.derived = null;
    }

    // --- Soufflet a servo ---
    if (cfg.AIR_SOURCE === 'AIR_SOURCE_BELLOW_SERVO') {
        if (Number(cfg.BELLOW_SERVO_ANGLE_CLOSED) === Number(cfg.BELLOW_SERVO_ANGLE_OPEN))
            err('servobellow', "Le soufflet a servo n'a aucune course : angles ouvert et ferme identiques.");
        if (Number(cfg.BELLOW_SERVO_MIN_SPEED_DPS) > Number(cfg.BELLOW_SERVO_MAX_SPEED_DPS))
            err('servobellow', 'La vitesse de balayage minimale depasse la maximale.');
        if (!(Number(cfg.BELLOW_SERVO_REVERSE_CLOSE) < Number(cfg.BELLOW_SERVO_REVERSE_OPEN)))
            err('servobellow', "Les seuils d'inversion doivent verifier fermeture < ouverture.");
    }

    // --- Turbines et pompe ---
    if (cfg.AIR_SOURCE === 'AIR_SOURCE_BLOWER_PWM') {
        if (Number(cfg.BLOWER_DUTY_MIN) >= Number(cfg.BLOWER_DUTY_MAX))
            err('blowerpwm', 'BLOWER_DUTY_MIN doit etre strictement inferieur a BLOWER_DUTY_MAX.');
        if (Number(cfg.BLOWER_SPINUP_DUTY) < Number(cfg.BLOWER_DUTY_MIN))
            warn('blowerpwm', "L'a-coup de demarrage est plus faible que le duty minimal : il ne servira a rien.");
    }
    if (cfg.AIR_SOURCE === 'AIR_SOURCE_BLOWER_ESC') {
        if (Number(cfg.ESC_PULSE_MIN_US) >= Number(cfg.ESC_PULSE_MAX_US))
            err('esc', 'ESC_PULSE_MIN_US doit etre strictement inferieure a ESC_PULSE_MAX_US.');
        if (Number(cfg.ESC_PULSE_IDLE_US) < Number(cfg.ESC_PULSE_MIN_US) ||
            Number(cfg.ESC_PULSE_IDLE_US) > Number(cfg.ESC_PULSE_MAX_US))
            err('esc', "L'impulsion de repos doit rester entre le minimum et le maximum.");
        // A 50 Hz la periode vaut 20 ms : une impulsion de 2 ms y tient largement.
        const periodUs = 1000000 / Number(cfg.SERVO_PWM_FREQUENCY);
        if (Number(cfg.ESC_PULSE_MAX_US) > periodUs)
            err('esc', `A ${cfg.SERVO_PWM_FREQUENCY} Hz la periode vaut ${periodUs.toFixed(0)} us : ` +
                       `une impulsion de ${cfg.ESC_PULSE_MAX_US} us n'y tient pas.`);
    }
    if (cfg.AIR_SOURCE === 'AIR_SOURCE_PUMP_ONOFF') {
        if (Number(cfg.PUMP_MIN_DUTY_PERCENT) > Number(cfg.PUMP_MAX_DUTY_PERCENT))
            err('pump', 'PUMP_MIN_DUTY_PERCENT depasse PUMP_MAX_DUTY_PERCENT.');
        if (Number(cfg.PUMP_REST_MS) < Number(cfg.PUMP_MAX_RUN_MS) / 10)
            warn('pump', 'Le repos force est tres court devant la duree de marche autorisee : ' +
                         'la protection thermique risque de ne pas suffire.');
        if (!cfg.PRESSURE_SENSOR_ENABLED)
            warn('pump', "Sans capteur, la pompe module son rapport cyclique a l'aveugle. Une pompe a " +
                         "reservoir tire tout son interet d'une regulation par hysteresis.");
    }

    // --- Pression ---
    if (cfg.PRESSURE_SENSOR_ENABLED) {
        if (Number(cfg.PRESSURE_MAX_KPA) <= Number(cfg.PRESSURE_TARGET_KPA))
            err('pressure', 'Le plafond de securite doit laisser de la marge au-dessus de la consigne.');
        if (!(Number(cfg.PRESSURE_FILTER_ALPHA) > 0 && Number(cfg.PRESSURE_FILTER_ALPHA) <= 1))
            err('pressure', 'Le filtre doit rester dans ]0 ; 1].');
        if (!(Number(cfg.PRESSURE_SCALE_MIN) < 1 && Number(cfg.PRESSURE_SCALE_MAX) > 1))
            err('pressure', 'Le facteur correctif doit encadrer 1.0 (minimum < 1 < maximum).');
        if (Number(cfg.PRESSURE_ADC_PER_KPA) > 0) {
            const full = (1023 - Number(cfg.PRESSURE_ADC_AT_ZERO)) / Number(cfg.PRESSURE_ADC_PER_KPA);
            if (full < Number(cfg.PRESSURE_MAX_KPA))
                err('pressure', `Le capteur sature a ${full.toFixed(1)} kPa, en dessous du plafond de securite ` +
                                `(${cfg.PRESSURE_MAX_KPA} kPa) : la surpression ne serait jamais detectee.`);
        }
    }

    return {errors, warnings};
}

// =========================================================================================
// GENERATION DE config.h
// =========================================================================================
// Le fichier produit est un remplacant complet de accordionV06/config.h : memes macros,
// memes blocs, y compris ceux des sources d'air non selectionnees — pouvoir changer de
// systeme sans reecrire le fichier fait partie du contrat.

function wrapComment(text, indent) {
    return String(text).split('\n').map(l => indent + '// ' + l).join('\n');
}

function formatFloat(v) {
    let s = String(Number(v));
    if (!s.includes('.') && !s.includes('e')) s += '.0';
    return s + 'f';
}

function formatValue(field, value) {
    switch (field.type) {
        case 'text':
            // CONFIG_NAME est une chaine C ; les broches analogiques (A0) sont des jetons.
            return field.k === 'CONFIG_NAME' ? JSON.stringify(String(value)) : String(value);
        case 'bool':
            if (field.fmt === 'truefalse') return value ? 'true' : 'false';
            return value ? '1' : '0';
        case 'float':
            return formatFloat(value);
        case 'int':
            return String(Math.round(Number(value))) + (field.suffix || '');
        case 'enum':
            return String(value);
        case 'hexlist':
        case 'pinlist':
            return String(value).split(',').map(s => s.trim()).filter(s => s.length).join(', ');
        default:
            return String(value);
    }
}

function generateNoteList(macroName, rows, title) {
    const out = [];
    out.push(`#define ${macroName}(X) \\`);
    if (rows.length === 0) return `// ${title} : aucune anche.\n`;
    rows.forEach((row, i) => {
        const a = rowAngles(row);
        const addr = pcaAddresses()[row.pca] || '0x40';
        // Alignement identique a celui d'un fichier ecrit a la main : le fichier genere
        // doit pouvoir remplacer l'existant sans produire de diff de mise en forme.
        const closed = (rowClosedToken(row) + ',').padEnd(27);
        const dir = ((a.dir ? 'true' : 'false') + ',').padEnd(7);
        const last = (i === rows.length - 1);
        out.push('    X(' + String(row.note).padStart(3) + ', ' + addr.padStart(4) + ', ' +
                 String(row.ch).padStart(2) + ', ' + Number(row.flow).toFixed(2) + 'f, ' +
                 closed + dir + row.prio + ')' + (last ? '' : ' \\'));
    });
    return out.join('\n');
}

function generateConfigH() {
    const cfg = state.cfg;
    const L = [];

    L.push('#ifndef CONFIG_H');
    L.push('#define CONFIG_H');
    L.push('');
    L.push('// =========================================================================================');
    L.push('//  CONFIGURATION DE LA MACHINE  --  C\'EST LE SEUL FICHIER A MODIFIER');
    L.push('// -----------------------------------------------------------------------------------------');
    L.push('//  Genere par tools/configurator/index.html. Regenerer plutot que d\'editer a la main :');
    L.push('//  l\'interface verifie ce qu\'un fichier ecrit a la main ne verifie pas — collisions de');
    L.push('//  canaux PCA, doublons de notes, broches I2C reutilisees, vitesse incompatible avec le');
    L.push('//  microstepping choisi.');
    L.push('//');
    L.push('//  Contraintes : uniquement des directives du preprocesseur, aucun #include autre que');
    L.push('//  airSourceTypes.h, aucun type Arduino. Ce fichier est inclus par noteMapping.h, qui doit');
    L.push('//  rester compilable sur PC pour les tests natifs.');
    L.push('//');
    L.push('//  Les valeurs DERIVEES (pas/mm, angle d\'ouverture effectif, vitesse reellement tenable)');
    L.push('//  ne sont pas ici : elles sont calculees et verifiees dans settings.h.');
    L.push('// =========================================================================================');
    L.push('');
    L.push('#include "airSourceTypes.h"');
    L.push('');
    L.push('// Les choix structurants sont proteges par un #ifndef : ils peuvent donc etre surcharges');
    L.push('// depuis la ligne de compilation (-DAIR_SOURCE=AIR_SOURCE_BLOWER_PWM). C\'est ce qui permet');
    L.push('// a l\'integration continue de compiler et de tester toutes les combinaisons a partir d\'un');
    L.push('// seul fichier de configuration.');
    L.push('');

    let sectionNumber = 0;
    for (const section of SCHEMA) {
        sectionNumber++;
        L.push('//===========================================================================================');
        L.push(`// ${sectionNumber}. ${section.title.toUpperCase()}`);
        L.push('//===========================================================================================');
        if (section.doc) L.push(wrapComment(section.doc, ''));
        L.push('');
        for (const f of section.fields) {
            if (f.doc) L.push(wrapComment(f.doc, ''));
            const line = `#define ${f.k} ${formatValue(f, cfg[f.k])}`;
            if (f.guard) {
                L.push(`#ifndef ${f.k}`);
                L.push(line);
                L.push('#endif');
            } else {
                L.push(line);
            }
            L.push('');
        }
    }

    // --- Tables de notes ---
    L.push('//===========================================================================================');
    L.push(`// ${sectionNumber + 1}. TABLES DE NOTES`);
    L.push('//===========================================================================================');
    L.push(wrapComment(
        "Une ligne par anche pilotee, sous forme de X-macro :\n" +
        "\n" +
        "  X(midiNote, pcaAddress, pcaChannel, airFlow, closedAngle, openDirection, priority)\n" +
        "\n" +
        "  midiNote      numero de note MIDI, explicite : la disposition Stradella de la main\n" +
        "                gauche n'est pas chromatique, aucune continuite n'est supposee\n" +
        "  pcaAddress    adresse I2C du PCA9685 qui porte l'actionneur\n" +
        "  pcaChannel    canal 0-15 sur ce PCA\n" +
        "  airFlow       consommation d'air relative de l'anche (les graves consomment plus)\n" +
        "  closedAngle   angle de repos du servo (ignore pour des electroaimants)\n" +
        "  openDirection true  = ouverture a closedAngle - SERVO_OPEN_ANGLE (montage normal)\n" +
        "                false = ouverture a closedAngle + SERVO_OPEN_ANGLE (montage miroir)\n" +
        "  priority      BASS (3) > MELODY (2) > CHORD (1). Quand MAX_SIMULTANEOUS_NOTES est\n" +
        "                atteint, une note entrante ne peut voler que la place d'une note\n" +
        "                STRICTEMENT moins prioritaire, la plus ancienne d'abord.\n" +
        "\n" +
        "NUM_NOTES_RIGHT / NUM_NOTES_LEFT doivent valoir exactement le nombre de lignes :\n" +
        "noteMapping.cpp le verifie a la compilation. Une main peut avoir 0 note (instrument\n" +
        "melodie seule, ou basses seules).", ''));
    L.push('');
    L.push(`#define NUM_NOTES_RIGHT ${state.right.length}`);
    L.push(`#define NUM_NOTES_LEFT  ${state.left.length}`);
    L.push('');
    L.push('// --- Main droite ---------------------------------------------------------------------');
    L.push(generateNoteList('RIGHT_HAND_NOTE_LIST', state.right, 'Main droite'));
    L.push('');
    L.push('// --- Main gauche ---------------------------------------------------------------------');
    L.push(generateNoteList('LEFT_HAND_NOTE_LIST', state.left, 'Main gauche'));
    L.push('');
    L.push('#endif');
    L.push('');

    return L.join('\n');
}

// =========================================================================================
// PROFILS JSON
// =========================================================================================
// Le JSON conserve ce que config.h ne sait pas exprimer : le sens de montage choisi ligne
// par ligne, et l'index de PCA plutot que l'adresse. C'est le format a garder pour reprendre
// une configuration plus tard.

function exportJson() {
    return JSON.stringify({
        format: 'accordion-servo-midi/config',
        version: 1,
        cfg: state.cfg,
        right: state.right,
        left: state.left,
    }, null, 2);
}

function importJson(text) {
    const data = JSON.parse(text);
    if (!data || typeof data !== 'object' || !data.cfg) throw new Error('Profil illisible : champ "cfg" absent.');
    // Fusion sur les valeurs par defaut : un profil ecrit par une version anterieure ne
    // connait pas les options ajoutees depuis, elles doivent garder leur valeur par defaut.
    state.cfg = Object.assign(defaultConfig(), data.cfg);
    if (Array.isArray(data.right)) state.right = data.right.map(normalizeRow);
    if (Array.isArray(data.left)) state.left = data.left.map(normalizeRow);
}

function normalizeRow(r) {
    return {
        note: Number(r.note) || 0,
        pca: Number(r.pca) || 0,
        ch: Number(r.ch) || 0,
        flow: Number(r.flow) || 1.0,
        mount: (r.mount === 'mirror' || r.mount === 'custom') ? r.mount : 'normal',
        angle: Number(r.angle) || 130,
        dir: r.dir !== false,
        prio: r.prio || 'NOTE_PRIORITY_MELODY',
    };
}

// =========================================================================================
// RELECTURE D'UN config.h EXISTANT
// =========================================================================================
// Permet de repartir d'une machine deja configuree sans tout ressaisir. La lecture est
// volontairement tolerante : ce qui n'est pas reconnu garde sa valeur par defaut, et le
// nombre de lignes reellement lues est rapporte pour que l'ecart se voie.

function parseConfigH(text) {
    const report = {read: 0, ignored: [], notes: {right: 0, left: 0}};
    const cfg = defaultConfig();

    // Retire les commentaires de fin de ligne sans casser les chaines entre guillemets.
    const stripComment = (s) => {
        let inString = false;
        for (let i = 0; i < s.length - 1; i++) {
            if (s[i] === '"') inString = !inString;
            if (!inString && s[i] === '/' && s[i + 1] === '/') return s.slice(0, i);
        }
        return s;
    };

    const defineRe = /^\s*#define\s+([A-Z_][A-Z0-9_]*)\s+(.+)$/;
    for (const rawLine of text.split('\n')) {
        const line = stripComment(rawLine);
        const m = line.match(defineRe);
        if (!m) continue;
        const [, key, rawValue] = m;
        const value = rawValue.trim();
        if (key.endsWith('_NOTE_LIST(X)') || value.endsWith('\\')) continue;

        const field = fieldByKey(key);
        if (!field) { if (!key.startsWith('NUM_NOTES')) report.ignored.push(key); continue; }

        switch (field.type) {
            case 'bool':
                cfg[key] = (value === '1' || value === 'true'); break;
            case 'int':
                cfg[key] = parseInt(value.replace(/[UL]+$/i, ''), 10); break;
            case 'float':
                cfg[key] = parseFloat(value.replace(/f$/i, '')); break;
            case 'text':
                cfg[key] = value.startsWith('"') ? value.slice(1, -1) : value; break;
            default:
                cfg[key] = value; break;
        }
        report.read++;
    }
    state.cfg = cfg;

    // Tables de notes : les lignes X(...) des deux listes.
    const addrs = pcaAddresses();
    const parseList = (macroName) => {
        const start = text.indexOf('#define ' + macroName + '(X)');
        if (start < 0) return null;
        // La liste s'arrete a la premiere ligne qui ne se termine pas par une continuation.
        const lines = text.slice(start).split('\n');
        const rows = [];
        for (let i = 1; i < lines.length; i++) {
            const line = lines[i];
            const m = line.match(/X\(([^)]*)\)/);
            if (m) {
                const parts = m[1].split(',').map(s => s.trim());
                if (parts.length >= 7) {
                    const addr = parts[1];
                    let pca = addrs.findIndex(a => a.toLowerCase() === addr.toLowerCase());
                    if (pca < 0) pca = 0;
                    const closed = parts[4];
                    let mount = 'custom', angle = parseInt(closed, 10) || 130;
                    if (closed === 'SERVO_CLOSED_ANGLE') { mount = 'normal'; }
                    else if (closed === 'SERVO_CLOSED_ANGLE_MIRROR') { mount = 'mirror'; }
                    rows.push({
                        note: parseInt(parts[0], 10),
                        pca, ch: parseInt(parts[2], 10),
                        flow: parseFloat(parts[3]),
                        mount, angle,
                        dir: parts[5].trim() === 'true',
                        prio: parts[6],
                    });
                }
            }
            if (!lines[i].trimEnd().endsWith('\\')) break;
        }
        return rows;
    };

    const right = parseList('RIGHT_HAND_NOTE_LIST');
    const left = parseList('LEFT_HAND_NOTE_LIST');
    if (right) { state.right = right; report.notes.right = right.length; }
    if (left)  { state.left = left;   report.notes.left = left.length; }

    return report;
}

// =========================================================================================
// INTERFACE
// =========================================================================================

const el = (tag, attrs, children) => {
    const node = document.createElement(tag);
    if (attrs) for (const [k, v] of Object.entries(attrs)) {
        if (k === 'class') node.className = v;
        else if (k === 'text') node.textContent = v;
        else if (k.startsWith('on')) node.addEventListener(k.slice(2), v);
        else node.setAttribute(k, v);
    }
    if (children) for (const c of [].concat(children)) if (c) node.appendChild(c);
    return node;
};

const fieldNodes = new Map();   // macro -> {row, input, badge}
const sectionNodes = new Map(); // id -> element

function buildField(f) {
    const id = 'f_' + f.k;
    let input;

    if (f.type === 'bool') {
        input = el('input', {type: 'checkbox', id});
    } else if (f.type === 'enum') {
        input = el('select', {id});
        for (const o of f.options) {
            const opt = el('option', {value: o.v, text: o.label});
            input.appendChild(opt);
        }
    } else if (f.type === 'int' || f.type === 'float') {
        input = el('input', {type: 'number', id});
        if (f.min !== undefined) input.min = f.min;
        if (f.max !== undefined) input.max = f.max;
        input.step = f.step !== undefined ? f.step : (f.type === 'float' ? 'any' : 1);
    } else {
        input = el('input', {type: 'text', id});
    }

    input.addEventListener('input', () => {
        if (f.type === 'bool') state.cfg[f.k] = input.checked;
        else if (f.type === 'int') state.cfg[f.k] = input.value === '' ? '' : Number(input.value);
        else if (f.type === 'float') state.cfg[f.k] = input.value === '' ? '' : Number(input.value);
        else state.cfg[f.k] = input.value;
        refresh();
    });

    const badge = el('span', {class: 'badge', text: 'modifie'});
    const label = el('label', {for: id}, [document.createTextNode(f.label), badge]);
    const hint = el('div', {class: 'hint'});
    // L'aide du champ, et celle de l'option choisie pour un menu deroulant.
    hint.textContent = f.doc ? f.doc.replace(/\n/g, ' ') : '';

    const control = el('div', {class: 'control'}, [input]);
    if (f.unit) control.appendChild(el('span', {class: 'unit', text: f.unit}));

    const optionHint = el('div', {class: 'hint option-hint'});
    const row = el('div', {class: 'field'}, [label, control, hint, optionHint]);
    row.dataset.macro = f.k;

    fieldNodes.set(f.k, {row, input, badge, optionHint, field: f});
    return row;
}

function buildSections(container) {
    for (const section of SCHEMA) {
        const body = el('div', {class: 'fields'});
        for (const f of section.fields) body.appendChild(buildField(f));

        const node = el('section', {id: 'sec_' + section.id}, [
            el('h2', {text: section.title}),
            section.doc ? el('p', {class: 'section-doc', text: section.doc}) : null,
            body,
        ]);
        // Zone d'affichage des grandeurs calculees, remplie par refresh().
        const derived = el('div', {class: 'derived', id: 'derived_' + section.id});
        node.appendChild(derived);
        container.appendChild(node);
        sectionNodes.set(section.id, node);
    }
}

function buildNav(container) {
    for (const section of SCHEMA) {
        const link = el('a', {href: '#sec_' + section.id, text: section.title});
        link.dataset.section = section.id;
        container.appendChild(link);
    }
    const notesLink = el('a', {href: '#sec_notes', text: 'Tables de notes'});
    notesLink.dataset.section = 'notes';
    container.appendChild(notesLink);
    const outLink = el('a', {href: '#sec_output', text: 'Fichier genere'});
    container.appendChild(outLink);
}

// --- Tables de notes ---------------------------------------------------------------------

function buildNoteTable(handKey, title, container) {
    const wrap = el('div', {class: 'hand'});
    wrap.appendChild(el('h3', {text: title}));

    const toolbar = el('div', {class: 'toolbar'});
    toolbar.appendChild(el('button', {type: 'button', text: '+ Ajouter une anche',
        onclick: () => { addRow(handKey); }}));
    toolbar.appendChild(el('button', {type: 'button', text: 'Remplir une plage chromatique...',
        onclick: () => { fillRange(handKey); }}));
    toolbar.appendChild(el('button', {type: 'button', text: 'Reattribuer les canaux',
        onclick: () => { reassignChannels(handKey); }}));
    toolbar.appendChild(el('button', {type: 'button', class: 'danger', text: 'Vider',
        onclick: () => { if (confirm('Supprimer toutes les anches de cette main ?')) { state[handKey] = []; rebuildNotes(); } }}));
    wrap.appendChild(toolbar);

    const table = el('table', {class: 'notes', id: 'table_' + handKey});
    wrap.appendChild(table);
    container.appendChild(wrap);
}

function renderNoteTable(handKey) {
    const table = document.getElementById('table_' + handKey);
    table.textContent = '';
    const addrs = pcaAddresses();

    const head = el('tr', null, [
        el('th', {text: '#'}),
        el('th', {text: 'Note MIDI'}),
        el('th', {text: 'Hauteur'}),
        el('th', {text: 'PCA'}),
        el('th', {text: 'Canal'}),
        el('th', {text: "Debit d'air"}),
        el('th', {text: 'Montage'}),
        el('th', {text: 'Angle ferme'}),
        el('th', {text: 'Ouvert'}),
        el('th', {text: 'Priorite'}),
        el('th', {text: ''}),
    ]);
    table.appendChild(el('thead', null, [head]));

    const body = el('tbody');
    state[handKey].forEach((row, index) => {
        const tr = el('tr');
        const num = (value, onchange, min, max, step) => {
            const i = el('input', {type: 'number'});
            i.value = value;
            if (min !== undefined) i.min = min;
            if (max !== undefined) i.max = max;
            if (step !== undefined) i.step = step;
            i.addEventListener('input', () => { onchange(i.value); refresh(); });
            return i;
        };

        tr.appendChild(el('td', {class: 'idx', text: String(index + 1)}));
        tr.appendChild(el('td', null, [num(row.note, v => { row.note = Number(v); renderNoteTable(handKey); }, 0, 127)]));
        tr.appendChild(el('td', {class: 'pitch', text: noteName(row.note)}));

        const pcaSel = el('select');
        addrs.forEach((a, i) => pcaSel.appendChild(el('option', {value: String(i), text: `${i} (${a})`})));
        pcaSel.value = String(Math.min(row.pca, Math.max(0, addrs.length - 1)));
        pcaSel.addEventListener('change', () => { row.pca = Number(pcaSel.value); refresh(); });
        tr.appendChild(el('td', null, [pcaSel]));

        tr.appendChild(el('td', null, [num(row.ch, v => { row.ch = Number(v); }, 0, 15)]));
        tr.appendChild(el('td', null, [num(row.flow, v => { row.flow = Number(v); }, 0.01, 10, 0.01)]));

        const mountSel = el('select');
        [['normal', 'Normal'], ['mirror', 'Miroir'], ['custom', 'Personnalise']]
            .forEach(([v, l]) => mountSel.appendChild(el('option', {value: v, text: l})));
        mountSel.value = row.mount;
        mountSel.addEventListener('change', () => { row.mount = mountSel.value; renderNoteTable(handKey); refresh(); });
        tr.appendChild(el('td', null, [mountSel]));

        const angles = rowAngles(row);
        if (row.mount === 'custom') {
            const cell = el('td');
            cell.appendChild(num(row.angle, v => { row.angle = Number(v); renderNoteTable(handKey); }, 0, 180));
            const dirSel = el('select');
            dirSel.appendChild(el('option', {value: 'true', text: '- (normal)'}));
            dirSel.appendChild(el('option', {value: 'false', text: '+ (miroir)'}));
            dirSel.value = row.dir ? 'true' : 'false';
            dirSel.addEventListener('change', () => { row.dir = dirSel.value === 'true'; renderNoteTable(handKey); refresh(); });
            cell.appendChild(dirSel);
            tr.appendChild(cell);
        } else {
            tr.appendChild(el('td', {class: 'ro', text: String(angles.closed)}));
        }
        tr.appendChild(el('td', {class: 'ro', text: String(angles.opened)}));

        const prioSel = el('select');
        PRIORITIES.forEach(p => prioSel.appendChild(el('option', {value: p.v, text: p.label})));
        prioSel.value = row.prio;
        prioSel.addEventListener('change', () => { row.prio = prioSel.value; refresh(); });
        tr.appendChild(el('td', null, [prioSel]));

        const actions = el('td', {class: 'actions'});
        actions.appendChild(el('button', {type: 'button', title: 'Dupliquer', text: '⧉',
            onclick: () => { state[handKey].splice(index + 1, 0, Object.assign({}, row)); rebuildNotes(); }}));
        actions.appendChild(el('button', {type: 'button', class: 'danger', title: 'Supprimer', text: '✕',
            onclick: () => { state[handKey].splice(index, 1); rebuildNotes(); }}));
        tr.appendChild(actions);

        body.appendChild(tr);
    });
    table.appendChild(body);
}

function firstFreeChannel() {
    const used = new Set();
    for (const rows of [state.right, state.left]) for (const r of rows) used.add(r.pca + ':' + r.ch);
    const reserve = (i, c) => used.add(i + ':' + c);
    if (state.cfg.AIR_VALVE_TYPE === 'AIR_VALVE_SERVO') reserve(Number(state.cfg.VALVE_PCA_INDEX), Number(state.cfg.VALVE_PCA_PIN));
    if (state.cfg.AIR_SOURCE === 'AIR_SOURCE_BELLOW_SERVO') reserve(Number(state.cfg.BELLOW_SERVO_PCA_INDEX), Number(state.cfg.BELLOW_SERVO_PCA_PIN));
    if (state.cfg.AIR_SOURCE === 'AIR_SOURCE_BLOWER_ESC') reserve(Number(state.cfg.ESC_PCA_INDEX), Number(state.cfg.ESC_PCA_PIN));

    const count = pcaAddresses().length;
    for (let p = 0; p < count; p++)
        for (let c = 0; c < 16; c++)
            if (!used.has(p + ':' + c)) return {pca: p, ch: c};
    return null;
}

function addRow(handKey) {
    const free = firstFreeChannel();
    if (!free) { alert('Plus aucun canal PCA libre : ajouter un PCA9685 dans la section Actionneurs.'); return; }
    const rows = state[handKey];
    const lastNote = rows.length ? rows[rows.length - 1].note + 1 : 60;
    const prio = handKey === 'left' ? 'NOTE_PRIORITY_BASS' : 'NOTE_PRIORITY_MELODY';
    rows.push(makeRow(Math.min(127, lastNote), free.pca, free.ch, 1.0, 'normal', prio));
    rebuildNotes();
}

// Remplissage d'une plage chromatique : c'est la disposition d'un clavier de melodie, ou
// d'une main gauche chromatique. Les canaux libres sont attribues dans l'ordre.
function fillRange(handKey) {
    const first = parseInt(prompt('Premiere note MIDI (ex. 54 pour Fa#3) :', '54'), 10);
    if (isNaN(first)) return;
    const count = parseInt(prompt("Nombre d'anches :", '34'), 10);
    if (isNaN(count) || count < 1) return;
    const flowHigh = parseFloat(prompt("Debit d'air de la note la plus GRAVE :", '1.00'));
    const flowLow = parseFloat(prompt("Debit d'air de la note la plus AIGUE :", '0.40'));
    if (isNaN(flowHigh) || isNaN(flowLow)) return;

    const prio = handKey === 'left' ? 'NOTE_PRIORITY_BASS' : 'NOTE_PRIORITY_MELODY';
    state[handKey] = [];
    for (let i = 0; i < count; i++) {
        const free = firstFreeChannel();
        if (!free) { alert(`Canaux PCA epuises apres ${i} anches : ajouter des PCA9685.`); break; }
        // Interpolation lineaire du debit : les graves consomment plus d'air.
        const t = count > 1 ? i / (count - 1) : 0;
        const flow = Math.round((flowHigh + t * (flowLow - flowHigh)) * 100) / 100;
        state[handKey].push(makeRow(first + i, free.pca, free.ch, flow, 'normal', prio));
    }
    rebuildNotes();
}

// Reattribue les canaux dans l'ordre des lignes, en sautant ceux qui sont reserves.
function reassignChannels(handKey) {
    const other = handKey === 'right' ? state.left : state.right;
    const used = new Set(other.map(r => r.pca + ':' + r.ch));
    const reserve = (i, c) => used.add(i + ':' + c);
    if (state.cfg.AIR_VALVE_TYPE === 'AIR_VALVE_SERVO') reserve(Number(state.cfg.VALVE_PCA_INDEX), Number(state.cfg.VALVE_PCA_PIN));
    if (state.cfg.AIR_SOURCE === 'AIR_SOURCE_BELLOW_SERVO') reserve(Number(state.cfg.BELLOW_SERVO_PCA_INDEX), Number(state.cfg.BELLOW_SERVO_PCA_PIN));
    if (state.cfg.AIR_SOURCE === 'AIR_SOURCE_BLOWER_ESC') reserve(Number(state.cfg.ESC_PCA_INDEX), Number(state.cfg.ESC_PCA_PIN));

    const count = pcaAddresses().length;
    let p = 0, c = 0;
    for (const row of state[handKey]) {
        while (p < count && used.has(p + ':' + c)) { c++; if (c > 15) { c = 0; p++; } }
        if (p >= count) { alert('Canaux PCA epuises : la reattribution est incomplete.'); break; }
        row.pca = p; row.ch = c;
        used.add(p + ':' + c);
    }
    rebuildNotes();
}

function rebuildNotes() {
    renderNoteTable('right');
    renderNoteTable('left');
    refresh();
}

// --- Carte d'occupation des canaux PCA ---------------------------------------------------
// Une configuration valide se lit d'un coup d'oeil ici : chaque case est un canal, et sa
// couleur dit ce qui l'occupe. C'est aussi la vue qui rend visible un PCA a moitie vide.

function renderChannelMap() {
    const host = document.getElementById('channelmap');
    host.textContent = '';
    const addrs = pcaAddresses();

    const owner = new Map();
    state.right.forEach(r => owner.set(r.pca + ':' + r.ch, {cls: 'right', label: `Main droite, note ${r.note} (${noteName(r.note)})`}));
    state.left.forEach(r => {
        const key = r.pca + ':' + r.ch;
        const existing = owner.get(key);
        owner.set(key, existing
            ? {cls: 'clash', label: existing.label + ' ET main gauche, note ' + r.note}
            : {cls: 'left', label: `Main gauche, note ${r.note} (${noteName(r.note)})`});
    });
    const special = (pca, ch, cls, label) => {
        const key = pca + ':' + ch;
        const existing = owner.get(key);
        owner.set(key, existing ? {cls: 'clash', label: existing.label + ' ET ' + label} : {cls, label});
    };
    if (state.cfg.AIR_VALVE_TYPE === 'AIR_VALVE_SERVO')
        special(Number(state.cfg.VALVE_PCA_INDEX), Number(state.cfg.VALVE_PCA_PIN), 'special', 'Valve generale');
    if (state.cfg.AIR_SOURCE === 'AIR_SOURCE_BELLOW_SERVO')
        special(Number(state.cfg.BELLOW_SERVO_PCA_INDEX), Number(state.cfg.BELLOW_SERVO_PCA_PIN), 'special', 'Servo de soufflet');
    if (state.cfg.AIR_SOURCE === 'AIR_SOURCE_BLOWER_ESC')
        special(Number(state.cfg.ESC_PCA_INDEX), Number(state.cfg.ESC_PCA_PIN), 'special', 'ESC de la turbine');

    addrs.forEach((addr, p) => {
        const line = el('div', {class: 'pcaline'});
        line.appendChild(el('div', {class: 'pcaname', text: `PCA ${p} · ${addr}`}));
        const grid = el('div', {class: 'pcagrid'});
        let free = 0;
        for (let c = 0; c < 16; c++) {
            const info = owner.get(p + ':' + c);
            if (!info) free++;
            const cell = el('div', {class: 'chan ' + (info ? info.cls : 'free'), text: String(c)});
            cell.title = info ? `Canal ${c} — ${info.label}` : `Canal ${c} — libre`;
            grid.appendChild(cell);
        }
        line.appendChild(grid);
        line.appendChild(el('div', {class: 'pcafree', text: `${free} libre${free > 1 ? 's' : ''}`}));
        host.appendChild(line);
    });
}

// --- Rafraichissement --------------------------------------------------------------------
// Les champs sont construits une seule fois ; refresh() ne fait que les remettre a jour,
// masquer ceux qui ne servent pas dans la configuration courante, et rejouer les
// verifications. Reconstruire le formulaire a chaque frappe ferait perdre le focus.

let refreshPending = false;

function refresh() {
    if (refreshPending) return;
    refreshPending = true;
    // Regroupe les rafraichissements declenches par une meme interaction.
    requestAnimationFrame(() => { refreshPending = false; doRefresh(); });
}

function doRefresh() {
    const cfg = state.cfg;

    // 1. Valeurs et visibilite des champs
    for (const [key, node] of fieldNodes) {
        const f = node.field;
        const visible = !f.visible || f.visible(cfg);
        node.row.classList.toggle('hidden', !visible);

        const value = cfg[key];
        if (f.type === 'bool') node.input.checked = !!value;
        else if (document.activeElement !== node.input) node.input.value = value;

        node.badge.classList.toggle('on', String(value) !== String(f.def));

        if (f.type === 'enum') {
            const opt = f.options.find(o => o.v === value);
            node.optionHint.textContent = (opt && opt.hint) ? opt.hint : '';
            node.optionHint.classList.toggle('hidden', !(opt && opt.hint));
        }
    }

    // 2. Visibilite des sections
    for (const section of SCHEMA) {
        const visible = !section.visible || section.visible(cfg);
        sectionNodes.get(section.id).classList.toggle('hidden', !visible);
        const link = document.querySelector(`nav a[data-section="${section.id}"]`);
        if (link) link.classList.toggle('dim', !visible);
    }

    // 3. Grandeurs calculees
    const {errors, warnings} = validate();
    renderDerived();
    renderMessages(errors, warnings);
    renderChannelMap();

    // 4. Fichier genere
    document.getElementById('output').value = generateConfigH();
    document.getElementById('summary').textContent =
        `${state.right.length} anche(s) main droite · ${state.left.length} main gauche · ` +
        `${pcaAddresses().length} PCA9685 · ${errors.length} erreur(s), ${warnings.length} avertissement(s)`;

    document.getElementById('btn-download').disabled = errors.length > 0;
    save();
}

function renderDerived() {
    for (const section of SCHEMA) {
        const host = document.getElementById('derived_' + section.id);
        if (host) host.textContent = '';
    }

    const cfg = state.cfg;
    if (cfg.AIR_SOURCE === 'AIR_SOURCE_BELLOW_STEPPER' && state.derived) {
        const {stepsPerMm, rateLimited} = state.derived;
        const host = document.getElementById('derived_stepper');
        const sweep = (Number(cfg.BELLOW_MAX_POSITION) - Number(cfg.BELLOW_MIN_POSITION)) *
                      (Number(cfg.BELLOW_REVERSE_THRESHOLD_OPEN) - Number(cfg.BELLOW_REVERSE_THRESHOLD_CLOSE));
        host.appendChild(el('h4', {text: 'Calcule a partir des valeurs ci-dessus'}));
        const list = el('dl');
        const add = (k, v, note) => {
            list.appendChild(el('dt', {text: k}));
            list.appendChild(el('dd', null, [document.createTextNode(v),
                note ? el('span', {class: 'note', text: ' — ' + note}) : null]));
        };
        add('Pas par millimetre', stepsPerMm.toFixed(1),
            "c'est STEPS_PER_MM, calcule par settings.h et jamais code en dur");
        add('Vitesse maximale tenable', rateLimited.toFixed(1) + ' mm/s',
            'bornee par la frequence de pas, pas par STEPPER_MAX_SPEED');
        add('Marge sur une note seule', (rateLimited / Math.max(1, Number(cfg.NORMAL_SPEED))).toFixed(1) + 'x',
            'nombre d\'anches simultanees avant saturation du debit');
        add('Course entre inversions', sweep.toFixed(0) + ' mm',
            `soit ${(sweep / Math.max(0.1, Number(cfg.NORMAL_SPEED))).toFixed(1)} s par balayage a NORMAL_SPEED`);
        host.appendChild(list);
    }

    if (cfg.AIR_SOURCE === 'AIR_SOURCE_BELLOW_SERVO') {
        const host = document.getElementById('derived_servobellow');
        const span = Math.abs(Number(cfg.BELLOW_SERVO_ANGLE_OPEN) - Number(cfg.BELLOW_SERVO_ANGLE_CLOSED));
        const used = span * (Number(cfg.BELLOW_SERVO_REVERSE_OPEN) - Number(cfg.BELLOW_SERVO_REVERSE_CLOSE));
        host.appendChild(el('h4', {text: 'Calcule a partir des valeurs ci-dessus'}));
        const list = el('dl');
        list.appendChild(el('dt', {text: 'Course entre inversions'}));
        list.appendChild(el('dd', {text: `${used.toFixed(0)} deg sur ${span.toFixed(0)} deg de debattement`}));
        list.appendChild(el('dt', {text: 'Duree d\'un balayage a une note'}));
        list.appendChild(el('dd', {text: `${(used / Math.max(0.1, Number(cfg.BELLOW_SERVO_MIN_SPEED_DPS))).toFixed(1)} s`}));
        host.appendChild(list);
    }

    if (cfg.PRESSURE_SENSOR_ENABLED) {
        const host = document.getElementById('derived_pressure');
        const full = (1023 - Number(cfg.PRESSURE_ADC_AT_ZERO)) / Math.max(0.001, Number(cfg.PRESSURE_ADC_PER_KPA));
        host.appendChild(el('h4', {text: 'Calcule a partir des valeurs ci-dessus'}));
        const list = el('dl');
        list.appendChild(el('dt', {text: 'Pleine echelle du capteur'}));
        list.appendChild(el('dd', {text: `${full.toFixed(1)} kPa`}));
        list.appendChild(el('dt', {text: 'Resolution'}));
        list.appendChild(el('dd', {text: `${(1000 / Math.max(0.001, Number(cfg.PRESSURE_ADC_PER_KPA))).toFixed(0)} Pa par compte ADC`}));
        host.appendChild(list);
    }
}

function renderMessages(errors, warnings) {
    const host = document.getElementById('messages');
    host.textContent = '';

    if (!errors.length && !warnings.length) {
        host.appendChild(el('div', {class: 'msg ok', text:
            'Configuration coherente. Le fichier genere devrait compiler et les verifications de settings.h passer.'}));
        return;
    }
    const line = (m, cls) => {
        const node = el('div', {class: 'msg ' + cls});
        node.appendChild(el('strong', {text: cls === 'err' ? 'Erreur — ' : 'Attention — '}));
        node.appendChild(document.createTextNode(m.msg));
        if (m.id) {
            const link = el('a', {href: '#sec_' + m.id, text: ' aller au reglage'});
            node.appendChild(link);
        }
        host.appendChild(node);
    };
    errors.forEach(m => line(m, 'err'));
    warnings.forEach(m => line(m, 'warn'));
}

// --- Persistance -------------------------------------------------------------------------
// Le travail en cours survit a un rechargement de la page. Le stockage peut echouer
// (navigation privee, site data bloque) : l'interface doit rester utilisable.

const STORAGE_KEY = 'accordion-servo-midi.config.v1';

function save() {
    try { localStorage.setItem(STORAGE_KEY, exportJson()); } catch (e) { /* sans effet */ }
}

function load() {
    try {
        const raw = localStorage.getItem(STORAGE_KEY);
        if (!raw) return false;
        importJson(raw);
        return true;
    } catch (e) { return false; }
}

// --- Entrees / sorties -------------------------------------------------------------------

function download(filename, text) {
    const blob = new Blob([text], {type: 'text/plain;charset=utf-8'});
    const url = URL.createObjectURL(blob);
    const a = el('a', {href: url, download: filename});
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    setTimeout(() => URL.revokeObjectURL(url), 1000);
}

async function copyToClipboard(text, button) {
    const original = button.textContent;
    try {
        await navigator.clipboard.writeText(text);
        button.textContent = 'Copie';
    } catch (e) {
        // Le presse-papiers peut etre refuse (page ouverte depuis file://, permission
        // absente) : on selectionne le texte pour que Ctrl+C fonctionne quand meme.
        const out = document.getElementById('output');
        out.focus(); out.select();
        button.textContent = 'Selectionne : Ctrl+C';
    }
    setTimeout(() => { button.textContent = original; }, 2000);
}

function wireButtons() {
    document.getElementById('btn-copy').addEventListener('click', ev => {
        copyToClipboard(generateConfigH(), ev.target);
    });
    document.getElementById('btn-download').addEventListener('click', () => {
        download('config.h', generateConfigH());
    });
    document.getElementById('btn-json').addEventListener('click', () => {
        download('accordion-config.json', exportJson());
    });
    document.getElementById('btn-reset').addEventListener('click', () => {
        if (!confirm('Revenir a la configuration par defaut du depot ? Le travail en cours sera perdu.')) return;
        resetAll();
        rebuildNotes();
    });
    document.getElementById('btn-import').addEventListener('click', () => {
        const text = document.getElementById('importbox').value.trim();
        if (!text) { alert('Coller d\'abord un profil JSON ou un fichier config.h.'); return; }
        try {
            if (text.startsWith('{')) {
                importJson(text);
                alert('Profil JSON charge.');
            } else {
                const report = parseConfigH(text);
                alert(`config.h relu : ${report.read} reglage(s), ` +
                      `${report.notes.right} anche(s) main droite, ${report.notes.left} main gauche.` +
                      (report.ignored.length ? `\n\nMacros non reconnues (valeurs par defaut conservees) :\n` +
                       report.ignored.join(', ') : ''));
            }
            rebuildNotes();
        } catch (e) {
            alert('Lecture impossible : ' + e.message);
        }
    });
    document.getElementById('btn-file').addEventListener('change', ev => {
        const file = ev.target.files && ev.target.files[0];
        if (!file) return;
        const reader = new FileReader();
        reader.onload = () => {
            document.getElementById('importbox').value = String(reader.result);
            document.getElementById('btn-import').click();
        };
        reader.readAsText(file);
        ev.target.value = '';
    });
}

function init() {
    resetAll();
    load(); // Reprend le travail en cours s'il y en a un

    buildNav(document.getElementById('nav'));
    buildSections(document.getElementById('sections'));

    const notesHost = document.getElementById('notes-host');
    buildNoteTable('right', 'Main droite (melodie)', notesHost);
    buildNoteTable('left', 'Main gauche (basses et accords)', notesHost);

    wireButtons();
    rebuildNotes();
}

document.addEventListener('DOMContentLoaded', init);
