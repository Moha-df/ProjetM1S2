# Projet M1 S2 — Sujet libre, jeu Unreal Engine

Projet réalisé dans le cadre du Master 1, Semestre 2 (sujet libre).
Il s'agit d'un jeu développé sous **Unreal Engine 5.7**.

## Auteurs

- **De Franceschi Mohamed**
- **Omar Elsemry**

## Stack technique

- Unreal Engine **5.7**
- C++ (module `ProjetM1S2`)
- Versionnage : Git + **Git LFS** (pour les assets binaires `.uasset`, `.umap`, textures, modèles 3D, etc.)

## Structure du projet

```
ProjetM1S2/
├── Config/          # Fichiers de configuration Unreal (.ini)
├── Content/         # Assets du jeu (.uasset, .umap, textures...) — versionnés via Git LFS
├── Source/          # Code source C++
│   └── ProjetM1S2/  # Module principal du jeu
├── ProjetM1S2.uproject  # Fichier projet Unreal
└── README.md
```

## Où trouver le code ?

- **Code C++** : dossier `Source/ProjetM1S2/`
- **Blueprints, niveaux et assets** : dossier `Content/` (visible depuis l'éditeur Unreal)
- **Configuration du projet** : dossier `Config/`

## Récupérer le projet

> Git LFS est requis pour récupérer les assets binaires.

```bash
# 1. Installer Git LFS (une seule fois sur la machine)
git lfs install

# 2. Cloner le dépôt
git clone https://github.com/Moha-df/ProjetM1S2.git

# 3. Récupérer les fichiers LFS
cd ProjetM1S2
git lfs pull
```

## Lancer le projet

### Prérequis

- **Unreal Engine 5.7** installé via l'Epic Games Launcher
- **Visual Studio 2022** (Windows) avec la charge de travail *"Développement de jeux en C++"* et le composant *"Outils Unreal Engine pour Visual Studio"*
- **Git LFS** installé

### Étapes

1. Faire un **clic droit** sur `ProjetM1S2.uproject` → **Generate Visual Studio project files**
   (cela régénère le `.sln` et les dossiers `Intermediate/` adaptés à votre machine).
2. Ouvrir `ProjetM1S2.sln` dans Visual Studio, sélectionner la configuration **Development Editor / Win64**, puis **Build** (Ctrl+Shift+B).
3. Lancer le projet :
   - soit en double-cliquant sur `ProjetM1S2.uproject`,
   - soit depuis Visual Studio en lançant le débogueur (F5).