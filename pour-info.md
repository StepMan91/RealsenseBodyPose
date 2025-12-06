# Documentation Complète du Projet : RealSense 3D Body Pose

Ce document explique en détail l'architecture, les fichiers, les outils et les technologies utilisés dans ce projet de suivi de squelette 3D en temps réel.

---

## 1. Objectif du Projet

Créer une application C++ capable de :
1.  Capturer des images RGB et Profondeur depuis une caméra **Intel RealSense**.
2.  Détecter les squelettes humains (poses) en temps réel avec une IA (**YOLOv8-Pose**).
3.  Utiliser l'accélération **GPU (CUDA)** pour atteindre une haute performance (60+ FPS).
4.  Projeter les points 2D (pixels) en coordonnées 3D (mètres) dans l'espace.
5.  Visualiser le résultat et envoyer les données pour la robotique.

---

## 2. Architecture des Fichiers

Voici l'arborescence simplifiée de votre projet et le rôle de chaque dossier :

```text
RealsenseBodyPose/
├── CMakeLists.txt              # Le "chef d'orchestre" de la compilation (recette de cuisine)
├── pour-moi.md                 # Guide de compilation OpenCV CUDA (votre bible)
├── pour-info.md                # Ce fichier (documentation générale)
├── QUICKSTART.md               # Guide de démarrage rapide
│
├── src/                        # CODE SOURCE (Le cœur du programme)
│   ├── main.cpp                # Point d'entrée. Initialise tout et lance la boucle principale.
│   ├── RealSenseCamera.cpp/.h  # Gère la caméra (connexion, config 60FPS, alignement RGB-Depth).
│   ├── PoseEstimator.cpp/.h    # Le cerveau IA. Charge le modèle ONNX et fait l'inférence sur GPU.
│   ├── SkeletonProjector.cpp/.h# Le mathématicien. Convertit (x,y) pixels -> (x,y,z) mètres.
│   ├── Visualizer.cpp/.h       # L'artiste. Dessine les squelettes sur l'image.
│   └── Utils.h                 # Outils divers (chronomètre, structures de données).
│
├── models/                     # MODÈLES IA
│   ├── yolov8n-pose.onnx       # Le modèle neuronal entraîné (format universel ONNX).
│   └── (scripts python...)     # Scripts pour convertir/tester les modèles.
│
├── ThirdParty/                 # DÉPENDANCES EXTERNES (Outils tiers)
│   ├── TensorRT-8.6.../        # Moteur d'inférence NVIDIA (optimisation extrême).
│   └── cudnn-windows.../       # Librairie de réseaux de neurones pour CUDA.
│
└── build/                      # ZONE DE CHANTIER (Généré par CMake)
    └── bin/Release/            # Le résultat final !
        ├── RealsenseBodyPose.exe   # VOTRE APPLICATION
        ├── opencv_world4100.dll    # Moteur OpenCV (avec CUDA)
        ├── realsense2.dll          # Pilote caméra
        └── (dlls nvidia...)        # Pilotes GPU
```

---

## 3. Détail des Composants Clés

### A. Le Code Source (`src/`)

*   **`main.cpp`** :
    *   **Rôle** : C'est le patron. Il lit les arguments (ex: `--model`), crée les objets (Camera, AI, Visualizer) et fait tourner la boucle infinie : *Capture -> Détection -> Projection -> Affichage*.
    *   **Pourquoi ?** : Sans lui, rien ne se lance.

*   **`RealSenseCamera`** :
    *   **Rôle** : Parle au driver Intel. Récupère l'image couleur et l'image de profondeur (distance de chaque pixel).
    *   **Spécial** : Elle "aligne" la profondeur sur la couleur pour qu'un pixel (x,y) corresponde au même point physique sur les deux images.

*   **`PoseEstimator`** :
    *   **Rôle** : Prend une image, la donne à manger au modèle YOLOv8 via OpenCV DNN (backend CUDA), et récupère une liste de points (nez, épaules, coudes...).
    *   **Pourquoi CUDA ?** : Sur CPU, ça prend 150ms (7 FPS). Sur GPU, ça prend 15ms (60+ FPS).

*   **`SkeletonProjector`** :
    *   **Rôle** : Prend un point 2D (ex: pixel 300, 200) et regarde la valeur de profondeur à cet endroit (ex: 1.5 mètres). Il utilise les "intrinsèques" de la caméra (sa focale) pour calculer la vraie position 3D (X, Y, Z).

### B. Les Outils (`Tools`)

*   **CMake** :
    *   C'est un générateur de projet. Il ne compile pas lui-même, mais il crée les fichiers pour Visual Studio (`.sln`, `.vcxproj`). Il trouve les librairies installées sur votre PC.

*   **Visual Studio 2022 (MSVC)** :
    *   C'est le compilateur. Il transforme le code C++ (texte) en code machine (binaire `.exe`).

*   **PowerShell** :
    *   Votre terminal de commande pour piloter CMake et lancer l'application.

---

## 4. Les Dépendances (Pourquoi elles sont là ?)

Pour que ce projet fonctionne, il s'appuie sur des géants :

1.  **OpenCV (Open Source Computer Vision)** :
    *   **Usage** : Traitement d'image, lecture/écriture de fichiers, et surtout **moteur d'inférence IA (DNN)**.
    *   **Spécificité ici** : Nous l'avons compilé manuellement pour activer **CUDA** (NVIDIA). La version standard téléchargée sur internet ne contient QUE le support CPU (trop lent).

2.  **CUDA (Compute Unified Device Architecture)** :
    *   **Usage** : Permet d'utiliser les cœurs de votre carte graphique RTX 4070 pour faire des calculs mathématiques parallèles (matrices) au lieu d'utiliser le processeur central.

3.  **cuDNN (CUDA Deep Neural Network)** :
    *   **Usage** : Une couche au-dessus de CUDA, optimisée spécifiquement pour les opérations d'IA (convolutions). OpenCV l'utilise pour aller encore plus vite.

4.  **Intel RealSense SDK 2.0** :
    *   **Usage** : La librairie officielle d'Intel pour communiquer avec la caméra D455/D435.

---

## 5. Avant / Après (L'évolution du projet)

| Caractéristique | AVANT (Début du projet) | APRÈS (Maintenant) | Gain / Changement |
| :--- | :--- | :--- | :--- |
| **Moteur IA** | OpenCV (Standard) | OpenCV (Custom Build) | Support GPU activé |
| **Matériel** | CPU (Intel Core) | GPU (RTX 4070) | Accélération massive |
| **Vitesse** | ~7 - 10 FPS | ~32 FPS (Limité par caméra) | **x3 à x4** (Potentiel x10) |
| **Latence** | ~150 ms | ~15 ms | Temps réel fluide |
| **Caméra** | 30 FPS | 60 FPS | Mouvements fluides |
| **Modules** | Base uniquement | + ArUco, Tracking, RGBD | Prêt pour robotique |

---

## 6. Pistes d'Amélioration (Pour aller plus loin)

Si vous voulez encore améliorer ce projet pour votre robot humanoïde :

1.  **TensorRT Natif** :
    *   Actuellement, nous utilisons OpenCV qui utilise CUDA. Utiliser TensorRT *directement* (sans passer par OpenCV DNN) pourrait gagner encore 10-20% de perf (mais c'est beaucoup plus complexe à coder).

2.  **Multi-Caméra** :
    *   Le code actuel gère 1 caméra. Pour un robot, on pourrait vouloir fusionner 2 caméras pour voir à 360°.

3.  **Communication Robot (ROS2)** :
    *   Actuellement, on affiche les coordonnées. L'étape suivante est d'envoyer ces X,Y,Z via le réseau (UDP ou ROS2) au contrôleur du robot pour qu'il bouge en conséquence.

4.  **Lissage (Smoothing)** :
    *   Les coordonnées brutes peuvent "trembler" un peu. Ajouter un filtre (Kalman ou OneEuroFilter) rendrait les mouvements du robot plus doux.

---

*Document généré pour Bastien Caspani - Projet RealSense Body Pose*
