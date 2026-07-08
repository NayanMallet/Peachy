# Peachy — Roadmap technique (3 mois)

> Document de passation. Destinataire : le prochain modèle/développeur (Opus 4.8).
> État au moment de la rédaction : moteur TD-PSOLA transparent à l'unité (erreur
> relative ~4e-8, testé), drive triode à bias dynamique, UI fidèle à la maquette,
> 11 tests verts, 0 warning. Lisez `docs/MANUAL.md` et `CLAUDE.md` d'abord.

## Architecture actuelle (référence rapide)

```
source/
├── PluginProcessor.{h,cpp}   APVTS (pitch/formant/drive/mix/mode/link),
│                             presets, chaîne: detector → engine → saturator
│                             → warmth shelves → DryWetMixer
├── PluginEditor.{h,cpp}      3 panneaux, LCD presets (< > dé), Link bidirectionnel
├── PeachyLookAndFeel.h       palette, typos (Righteous/ShareTechMono/Oswald),
│                             knobs striés, halos blancs melatonin, PopupMenu LCD
└── dsp/
    ├── PitchDetector.{h,cpp}       YIN, hop 5ms, 60-1000Hz, garde anti-octave
    ├── PitchFormantEngine.{h,cpp}  TD-PSOLA mono, grille verrouillée en phase,
    │                               formants par rééchantillonnage de grain,
    │                               latence 3*maxPeriod (~50ms)
    └── TubeSaturator.{h,cpp}       pre/de-emphasis 1.7kHz, triode asymétrique,
                                    bias dynamique, oversampling 4x, makeup RMS
```

Invariants à ne jamais casser (tous testés dans `tests/DspChain.cpp`) :
1. **Transparence à l'unité** : pitch 0 / formant 0 → sortie = entrée retardée de
   `getLatencySamples()` exactement (test "PSOLA engine is transparent at unity").
2. **Makeup drive** : RMS(drive 100) ≥ 0.9 × RMS(drive 0).
3. **+12 st sur signal harmonique 220Hz → F0 mesuré 440Hz ±6%.**
4. Aucune allocation dans `processBlock` (voir « Dette & règles RT » ci-dessous).

---

## Mois 1 — Contrôle MIDI & justesse (le mode "Vocoder" du Little AlterBoy)

### 1.1 Entrée MIDI (semaine 1)
- `CMakeLists.txt` : `NEEDS_MIDI_INPUT TRUE` dans `juce_add_plugin`.
- `PluginProcessor` : consommer `juce::MidiBuffer` dans `processBlock` (ne plus
  l'ignorer). Nouvelle classe **`MidiNoteTracker`** (source/dsp/) :
  - état : note active (dernière note-on prioritaire), pitch-bend (±2 st),
    legato : `float getTargetMidiNote()` retourne note + bend, `-1` si aucune.
  - monophonique : note-off ne coupe que si c'est la note active (last-note priority).
- Nouveau choix dans le paramètre `mode` : **"MIDI"** (index 3). ⚠ Étendre un
  `AudioParameterChoice` casse la compatibilité de sessions → créer le paramètre
  avec les 4 choix dès maintenant et versionner (`ParameterID{"mode", 2}`).
- Logique (dans `processBlock`, calquée sur Robot) :
  `targetTransposeSemitones = midiNote - smoothedDetectedMidi` quand une note MIDI
  est tenue ; sinon retenir la dernière correction (comme unvoiced actuel).
- UI : 4e bouton "MIDI" dans le panneau MODE (même mécanique ParameterAttachment).

### 1.2 Scale/Key pour Quantize (semaines 2-3)
- Nouveaux paramètres : `key` (AudioParameterChoice, 12 choix C..B) et `scale`
  (Chromatic / Major / Minor — chromatic = comportement actuel).
- **`ScaleQuantizer`** (header-only, source/dsp/) :
  `float snapToScale (float midiNote, int key, int scale)` — table de masques de
  12 bits par gamme, recherche du degré le plus proche (distance en demi-tons,
  ties → vers le bas). Remplace `std::round(midiNote)` dans le mode Quantize.
- UI : deux petits ComboBox LCD-style sous le panneau MODE (ou un second écran
  LCD compact) — réutiliser le style PopupMenu existant.

### 1.3 Retune Speed (semaine 4)
- Paramètre `retune` (0..100%, défaut selon mode). Mapper vers la constante de
  lissage : `retuneSeconds = jmap(retune, 0.0015f, 0.080f)`.
  Remplace les constantes codées en dur (0.0015/0.003/0.005) de `processBlock`.
- Presets à mettre à jour (les hard-tune restent à ~0, les naturels vers 30-50%).

Livrable fin M1 : hard-tune jouable au clavier MIDI (l'usage « vocoder-ish » du
manuel AlterBoy), gammes, vitesse de correction réglable. Tests : MidiNoteTracker
(note on/off/legato/bend), ScaleQuantizer (table exhaustive C majeur), un test
d'intégration MIDI → transpose.

---

## Mois 2 — Qualité sonore & fonctionnalités de production

### 2.1 Amélioration PSOLA (semaines 1-2)
- **Marquage GCI** : remplacer le raffinement par corrélation par une détection
  d'instants de fermeture glottale — algorithme SEDREAMS simplifié :
  1. signal de résidu approx. = dérivée du signal filtré passe-bas 800Hz ;
  2. minima du résidu dans une fenêtre autour de la position attendue.
  Classe : méthode privée `refineMarkGCI()` dans `PitchFormantEngine`, A/B avec
  l'existant derrière un flag compile-time `PEACHY_USE_GCI`.
- **Fenêtre asymétrique** optionnelle (Hann gauche = période précédente, droite =
  suivante) pour les transitions de période — réduit le souffle sur les glides.
- Benchmark : ajouter un cas dans `benchmarks/Benchmarks.cpp` qui traite 10s de
  voix synthétique et mesure le temps (garde-fou de régression CPU).

### 2.2 Sortie & gain staging (semaine 3)
- Paramètres `inputTrim` (-12..+12 dB) et `output` (-24..+12 dB), appliqués en
  `juce::dsp::Gain` (rampe intégrée). Position : trim avant le détecteur,
  output après le DryWetMixer.
- UI : deux petits potards (réutiliser `setupKnob` avec `numTicks` réduit) dans
  le footer élargi, ou colonne étroite à droite du panneau COLOR & OUTPUT.

### 2.3 Presets utilisateur (semaine 4)
- **`PresetManager`** (source/, message thread uniquement) :
  - dossier : `File::getSpecialLocation(userApplicationDataDirectory)/Peachy/Presets`,
    fichiers `.peachy` = XML de l'APVTS (réutiliser copyState/replaceState) ;
  - API : `saveCurrent(name)`, `loadFile(f)`, `scan()` → `StringArray` ;
  - le PopupMenu du LCD gagne une section "User" + "Save preset…"
    (AlertWindow LCD-style pour le nom).
- Le `currentProgram` reste la banque factory ; les user presets vivent à côté
  (comme Soundtoys sépare Factory/User).

---

## Mois 3 — Finition produit

### 3.1 Redimensionnement UI (semaine 1)
- `setResizable(true, true)` + `setResizeLimits` + `AudioProcessorEditor::
  setScaleFactor` piloté par un facteur global — l'UI est déjà en coordonnées
  fixes ; envelopper `paint`/`resized` avec `juce::AffineTransform::scale` via
  `Component::setTransform` (chemin le plus court avec ce code).
- Persister le facteur dans l'état du plugin (propriété du ValueTree racine).

### 3.2 CI, signature, installeurs (semaines 2-3)
- Réactiver la matrice GitHub Actions (le template Pamplejuce fournit tout) :
  macOS universal (arm64+x86_64 : `-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"`),
  Windows, Linux (tests seulement).
- Signature macOS : compte Apple Developer requis — guide melatonin.dev déjà
  référencé dans CLAUDE.md. Windows : Azure Trusted Signing.
- Pluginval en CI : `pluginval --strictness-level 8` sur VST3/AU.

### 3.3 QA & bêta (semaine 4)
- Checklist DAW : Logic (AU, PDC, automation), Ableton (VST3, freeze),
  Reaper (mono→stereo, oversampling hôte), FL (blocs variables).
- Cas limites : SR 44.1/48/96/192k, buffers 32→4096, bypass hôte, automation
  rapide de mode, sessions sauvegardées/rechargées (versionner les ParameterID !).

---

## Dette & règles RT (à maintenir)

Fait :
- `processBlock` : zéro allocation (rings préalloués, marks en anneau fixe,
  String/getProgramName jamais appelés côté audio, AsyncUpdater au lieu de
  MessageManager::callAsync dans le listener de paramètres).

À surveiller / reste à faire :
1. `PitchFormantEngine::process` : la somme mono passe par `monoScratch` — ne pas
   introduire de `AudioBuffer` temporaire ici.
2. `TubeSaturator` : le probe makeup fait 16 `std::sin` par bloc — OK, mais si on
   ajoute des étages, précalculer la table de sinus (constexpr array 16).
3. Toute nouvelle communication UI→DSP : uniquement `std::atomic` ou APVTS ;
   DSP→UI : atomics lus par un `juce::Timer` (pattern déjà en place, 4Hz).
4. Ne jamais appeler `getProgramName`/`setCurrentProgram` depuis l'audio thread.
5. Les `melatonin::DropShadow` doivent rester membres (cache du blur) — pas de
   temporaires dans les `paint()` chauds (un seul temporaire subsiste dans
   `drawGlowingLed`, acceptable car la LED est petite ; à membrer si profilage UI).
6. Restructuration si le projet grossit : `source/ui/` (LookAndFeel, LcdDisplay,
   composants), `source/dsp/` (déjà fait), `source/state/` (PresetManager,
   ParamIDs). Garder `SharedCode` INTERFACE et le glob CONFIGURE_DEPENDS.

## Algorithmes de référence (pour les évolutions)

- TD-PSOLA : Moulines & Charpentier (1990). Notre variante : grille de synthèse
  verrouillée en phase sur la grille d'analyse (voir commentaire dans
  `renderReadyGrains()`), formants par rééchantillonnage de grain (facteur =
  `exp2(formant/12)`), unvoiced à ratio 1.
- GCI : Drugman & Dutoit, "Glottal Closure and Opening Instant Detection from
  Speech Signals" (SEDREAMS).
- YIN : de Cheveigné & Kawahara (2002) — implémenté, seuil 0.15, CMNDF,
  interpolation parabolique, garde de continuité d'octave.
