# Peachy — Manuel utilisateur

*Vocal Processor · VST3 / AU / CLAP / Standalone*

---

## Bienvenue

Peachy est un processeur vocal monophonique temps réel : pitch shifting, formant
shifting, hard-tune, voix robot et saturation à lampe, dans une seule façade.
Il descend en droite ligne des boîtes hardware de studio — un moteur temporel
synchronisé sur la hauteur de la voix, pas un time-stretch générique. C'est ce
qui lui donne son grain lisse et « analogique » : à réglages neutres, Peachy est
transparent ; poussé, il devient un instrument.

**La règle d'or** (elle vaut pour tous les processeurs de cette famille) :
Peachy est conçu pour **une seule voix, propre et sèche**. Pas d'accords, pas de
chœurs, pas de réverbe en entrée. Placez-le premier dans la chaîne d'insert,
avant compression lourde et effets d'espace.

## Tour de la façade

### PITCH (−12 à +12 demi-tons)
Transpose la voix. Grâce à la préservation des formants, un shift d'une octave
change la *note*, pas le *personnage* : le timbre du chanteur reste en place.
- ±2-3 st : harmonies discrètes, doublures.
- +12 / −12 : octaves parallèles (utilisez MIX ~40-50 %).
- En mode ROBOT, PITCH devient l'offset de la note cible.

### FORMANT (−12 à +12 demi-tons)
Déplace l'enveloppe spectrale — la « taille » du conduit vocal — sans toucher à
la note. C'est le réglage le plus magique de la façade :
- +2 à +4 : plus jeune, plus léger, plus « féminin ».
- −2 à −4 : plus large, plus sombre, plus « masculin ».
- Extrêmes : dessin animé ou géant de film fantastique.

### LINK
Verrouille FORMANT sur PITCH (et réciproquement — le lien est bidirectionnel).
Utilisez-le pour les effets « bande accélérée » (chipmunk/monstre), où pitch et
timbre doivent bouger ensemble. Laissez-le éteint pour tout le reste : c'est la
séparation pitch/formant qui fait sonner naturel.

### MODE
- **TRANSPOSE** — le shifter pur : PITCH et FORMANT contrôlent directement le
  moteur. Le mode « musical » par défaut.
- **QUANTIZE** — hard-tune : chaque note chantée est aimantée au demi-ton le
  plus proche, quasi instantanément. L'effet pop/hip-hop signature. PITCH ajoute
  un offset après la correction (ex : +12 pour un hard-tune à l'octave).
- **ROBOT** — écrase la mélodie : la voix est verrouillée sur une hauteur fixe
  (Do central + PITCH). Le vibrato disparaît, la diction reste — la voix machine
  classique.

### DRIVE (0 à 100 %)
Saturation à lampe, héritée des étages triode : asymétrique, réactive à
l'enveloppe de la voix (le bias « respire » avec le chant), avec un tilt
hauts-médiums qui la rend agressive et chaude à la fois. Le gain de sortie est
compensé automatiquement : montez le drive, le niveau perçu ne bouge pas, seule
la densité harmonique monte. À 100 %, c'est sale — volontairement.

### MIX (0 à 100 %)
Balance traité/direct, compensée en latence (pas de filtrage en peigne).
- 100 % : l'effet remplace la voix (hard-tune, robot).
- 30-50 % : doublures, octaves, saturation parallèle.

### L'écran LCD
- La ligne du haut affiche le **preset**, la ligne du bas le **mode** actif.
- Cliquez sur le texte pour ouvrir la **liste des presets**.
- `<` / `>` : preset précédent / suivant.
- **Le dé** : lance des valeurs aléatoires *musicales* (pitch ±7, formant ±4,
  drive ≤60, mix ≥40). Inspiration instantanée, sans jamais rien casser.
- Les valeurs sous les potards sont éditables : double-clic, tapez, Entrée.

## Les presets d'usine

| Groupe | Presets | Usage |
|---|---|---|
| Utilitaires | Init (Flat), Tube Warmth, Parallel Crush | neutre / chaleur / NY-style |
| Caractère | Chest & Body, Bright & Young, Gender Flip Fem/Masc | timbre sans changer la note |
| Harmonies | Octave Stack, Sub Octave, Fifth Harmony, Natural Double | couches sous le lead |
| Hard-tune | Clean Auto-Tune, Trap Ad-Lib, Drill Grit, Pop Tune Sheen | du propre au crade |
| Robot | Classic Robot, Deep Machine, High Droid | voix machine |
| FX | Chipmunk, Monster | pitch+formant liés |

## Recettes rapides

**Hard-tune moderne** — QUANTIZE, mix 100 %, drive 10-20 %. Chantez legato en
visant les notes : l'aimantation fait le reste. Trop d'effet ? C'est le but.

**Doublure naturelle** — TRANSPOSE, pitch +0.2, formant −0.8, mix ~45 %.
Dupliquez la piste et panoramiquez pour un double stéréo.

**Radio / téléphone méchant** — formant −3, drive 40 %, mix 100 %.

**Robot de fin de morceau** — ROBOT, pitch −5, drive 30 %. Automatisez MIX pour
faire entrer la machine progressivement.

## Notes techniques

- **Latence** : ~50 ms (compensée automatiquement par votre DAW). En prise de
  voix en direct, utilisez le monitoring direct de votre interface.
- **Entrée** : mono ou stéréo ; le traitement est monophonique par conception,
  la sortie est identique sur les deux canaux (phase parfaitement solide).
- **Plage de détection** : 60 Hz à 1 kHz — de la basse profonde au soprano.
- **Signal idéal** : voix seule, sèche, sans bruit de fond lourd. Les consonnes
  et les respirations passent intactes (elles ne sont jamais transposées).
- **Formats** : VST3, AU, AUv3, CLAP, Standalone · macOS (Intel/Apple Silicon),
  Windows, Linux · 44.1 à 192 kHz.

---

*Peachy Audio · « It's peachy. »*
