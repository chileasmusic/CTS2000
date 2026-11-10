# CTS2000 — émulation VST3 du Keytek (Siel) CTS-2000

Synthé "Cross Table Sampled" : 2 oscillateurs × 3 tables par voix, 8 voix, 5 zones d'une octave, filtre 24 dB résonant,
3 LFO, enveloppes à segments, 48 presets internes + 48 "cartouche", édition par familles comme sur l'original.

## Obtenir le fichier .vst3 (sans rien installer sur ton PC)

1. Crée un compte gratuit sur github.com, puis **New repository** (privé si tu veux), par exemple `cts2000`.
2. Sur la page du dépôt : **Add file > Upload files**, glisse **tout le contenu de ce dossier**
   (y compris le dossier caché `.github`), puis **Commit changes**.
   *Astuce :* si `.github` n'apparaît pas à l'envoi, installe GitHub Desktop, qui envoie les dossiers cachés.
3. Onglet **Actions** : le workflow *Build VST3* démarre tout seul (10 à 20 min la première fois).
4. Quand il est vert, ouvre-le, descends à **Artifacts**, télécharge **CTS2000-VST3-Windows** (ou macOS).
5. Dézippe, puis **clic droit > Exécuter en tant qu'administrateur** sur `Install-Windows.bat`
   (ou copie le dossier `CTS2000.vst3` dans `C:\Program Files\Common Files\VST3\`).

## Dans FL Studio

*Options > Manage plugins > Find installed plugins* (ou "Verify plugins"), puis dans le Channel rack :
*Add > CTS2000*. Vérifie que `C:\Program Files\Common Files\VST3` est dans les chemins de recherche.

## Compiler soi-même (facultatif)

Visual Studio 2022 (charge de travail C++) + CMake :
`cmake -B build` puis `cmake --build build --config Release --target CTS2000_VST3`
Résultat : `build/CTS2000_artefacts/Release/VST3/CTS2000.vst3`.
Test du moteur seul (sans JUCE) : `g++ -std=c++17 -O2 -ISource tests/engine_test.cpp Source/Engine.cpp -o t && ./t`

## Utilisation

- **Mode Standard** : les 24 boutons rappellent les presets 1-24 ; `BANK A/B`, `INTERNAL/CARTRIDGE`, `UP/DOWN`.
- **PRESET/PARAM.** : passe en mode Edit. Les 24 boutons choisissent alors la **famille** de paramètres
  (comme les numéros de la façade). Les 6 sliders **A à F** pilotent les paramètres de la famille :
  clique-glisse, ou **double-clique la valeur sous le slider pour la taper**. `UP/DOWN` = réglage fin.
- **Zone** : en mode Edit, joue une touche du clavier (écran ou MIDI) pour choisir la zone (octave) à éditer.
  **ZONE/GLOBAL** : en Global, la valeur est relative et s'ajoute aux 5 zones (comme dans le manuel).
- **Sauvegarde** : `WRITE` (après avoir activé l'écriture avec le bouton 21 en mode Edit) puis `ENTER`.
  Le bouton 23 renomme le preset. `INITIALIZE PRESET` puis `ENTER` remet un patch de base.
  `SAVE/LOAD BANK` exportent/importent les 96 presets (fichier `.cts2000`).
- L'état complet est sauvegardé avec ton projet FL Studio.

## Tables d'onde : IMPORTANT

Les 333 tables de la ROM d'origine ne sont publiées nulle part. Le plugin embarque **64 tables de substitution générées**
(sinus, scie, carré, triangle, impulsions, orgue, cloche, cordes, voyelle…), **ce ne sont pas les sons du vrai CTS-2000**.
Pour les remplacer : dépose des fichiers `00.wav` … `63.wav` (un cycle de forme d'onde, mono) dans
`%APPDATA%\CTS2000Tables\`, au prochain chargement du plugin ils prennent la place des tables correspondantes.
Idem pour les presets d'usine : ils ne sont pas disponibles, les 5 patchs `DEMO …` sont des créations originales.

## Ce qui vient du manuel et ce qui est estimé

Fidèle au manuel : les 24 familles, l'assignation des sliders, les plages indiquées `(manuel)` dans `Source/Params.h`,
les zones, le mode global relatif, la machine Delay/Time 1/Break Point/Time 2, le flux Edit/Write/Init.
Estimé (le manuel en ligne ne donne pas ces courbes) : toutes les échelles de temps, la loi du filtre, la plage
de Resonance/Keyboard tracking/Detune, l'ordre exact de certains sliders (familles 2, 5, 6, 13-18) et le
comportement de la pédale. Les familles 19 (MIDI), 22 (Zone copy) et 24 (Compare) ne sont pas implémentées.
Pour corriger une plage ou un nom, il suffit de modifier la ligne correspondante dans `Params.h`.
