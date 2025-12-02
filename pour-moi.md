# Guide Complet: OpenCV avec CUDA pour Performance Maximale
## 🚀 De 10 FPS à 70-80 FPS - Guide pour Débutants

**Durée totale**: 3-4 heures  
**Niveau de difficulté**: Moyen  
**Résultat**: Application 7x plus rapide (70-80 FPS)

---

## 📋 Table des Matières

1. [Vue d'ensemble](#vue-densemble)
2. [Prérequis obligatoires](#prérequis-obligatoires)
3. [Étape 1: Télécharger OpenCV](#étape-1-télécharger-opencv)
4. [Étape 2: Configuration avec CMake](#étape-2-configuration-avec-cmake)
5. [Étape 3: Compilation](#étape-3-compilation)
6. [Étape 4: Installation](#étape-4-installation)
7. [Étape 5: Reconstruire l'application](#étape-5-reconstruire-lapplication)
8. [Étape 6: Test de performance](#étape-6-test-de-performance)
9. [Dépannage](#dépannage)

---

## Vue d'ensemble

### Pourquoi cette procédure?

Actuellement, votre application utilise OpenCV **sans support CUDA** → tout s'exécute sur le CPU → **~10 FPS**

Après cette procédure, OpenCV utilisera le **GPU RTX 4070** → **70-80 FPS** ⚡

### Qu'allez-vous faire?

1. Télécharger le code source d'OpenCV
2. Le configurer pour utiliser CUDA
3. Le compiler (transformation du code en programme)
4. Remplacer votre OpenCV actuel par cette nouvelle version
5. Recompiler votre application pour qu'elle utilise le nouveau OpenCV

---

## Prérequis obligatoires

### ✅ Vous DEVEZ avoir installé:

- [x] Visual Studio 2022 Professional (✅ vous l'avez)
- [x] CMake (✅ vous l'avez - dans VS2022)
- [x] CUDA 12.6 (✅ vous l'avez)
- [x] cuDNN 8.9 (✅ vous l'avez - dans `ThirdParty/`)

### 💾 Espace disque nécessaire:

- Source OpenCV: ~500 MB
- Build directory: ~15 GB pendant la compilation
- Installation finale: ~5 GB
- **Total temporaire**: ~20 GB

### ⏱️ Temps par étape:

1. Téléchargement: 5-10 minutes
2. Configuration CMake: 10-15 minutes (vous cliquez, ça calcule)
3. **Compilation: 1.5 - 3 heures** (automatique, vous pouvez faire autre chose)
4. Installation: 5 minutes
5. Recompilation app: 2 minutes

---

## Étape 1: Télécharger OpenCV

### 1.1 Télécharger le code source

**PowerShell** (clic droit sur icône PowerShell → Exécuter en tant qu'administrateur):

```powershell
# Créer un dossier de travail
cd C:\
mkdir opencv_build
cd opencv_build

# Télécharger OpenCV 4.10.0 (source)
Invoke-WebRequest -Uri "https://github.com/opencv/opencv/archive/4.10.0.zip" -OutFile "opencv-4.10.0.zip"

# Télécharger OpenCV Contrib (modules supplémentaires)
Invoke-WebRequest -Uri "https://github.com/opencv/opencv_contrib/archive/4.10.0.zip" -OutFile "opencv_contrib-4.10.0.zip"

# Extraire les fichiers
Expand-Archive -Path "opencv-4.10.0.zip" -DestinationPath "."
Expand-Archive -Path "opencv_contrib-4.10.0.zip" -DestinationPath "."

# Vérifier que tout est là
dir
```

**Vous devriez voir**:
```
opencv-4.10.0/
opencv_contrib-4.10.0/
opencv-4.10.0.zip
opencv_contrib-4.10.0.zip
```

✅ **Check**: Vous avez 2 dossiers et 2 fichiers .zip

---

## Étape 2: Configuration avec CMake

### 2.1 Créer le dossier de build

```powershell
cd C:\opencv_build
mkdir build
cd build
```

### 2.2 Lancer CMake GUI

**Option A - Via VS2022**:
```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\Cmake\bin\cmake-gui.exe"
```

**Option B - Si vous avez CMake standalone**:
```powershell
cmake-gui
```

### 2.3 Configurer dans CMake GUI

**Important**: Suivez EXACTEMENT ces étapes avec captures d'écran mentales!

#### Étape 2.3.1: Spécifier les chemins

Dans CMake GUI:

1. **Where is the source code**: 
   ```
   C:/opencv_build/opencv-4.10.0
   ```
   *(Cliquez sur "Browse Source..." et sélectionnez ce dossier)*

2. **Where to build the binaries**:
   ```
   C:/opencv_build/build
   ```
   *(Cliquez sur "Browse Build..." et sélectionnez ce dossier)*

3. **Cliquez sur "Configure"** (bouton en bas)

#### Étape 2.3.2: Sélectionner le générateur

Une fenêtre va s'ouvrir:

- **Specify the generator**: Sélectionnez **"Visual Studio 17 2022"**
- **Optional platform**: Sélectionnez **"x64"**
- **Use default native compilers**: ✅ Coché
- Cliquez **"Finish"**

⏳ **Attendez 2-3 minutes** - CMake va analyser votre système

#### Étape 2.3.3: Activer CUDA et configurer

Vous allez voir une liste rouge de paramètres. **Utilisez la barre de recherche en haut!**

**🔴 CRITIQUE - Options à ACTIVER (cocher)**:

Dans la barre de recherche, tapez chaque nom et cochez:

1. Recherchez `WITH_CUDA` → ✅ **Cochez**
2. Recherchez `OPENCV_DNN_CUDA` → ✅ **Cochez**
3. Recherchez `BUILD_opencv_world` → ✅ **Cochez**
4. Recherchez `WITH_CUBLAS` → ✅ **Cochez**
5. Recherchez `ENABLE_FAST_MATH` → ✅ **Cochez**
6. Recherchez `CUDA_FAST_MATH` → ✅ **Cochez**

**🔴 CRITIQUE - Options à MODIFIER**:

7. Recherchez `CUDA_ARCH_BIN`
   - Par défaut il y a plein de chiffres
   - **Supprimez tout** et mettez seulement: **`8.9`**
   - *(C'est l'architecture de votre RTX 4070)*

8. Recherchez `OPENCV_EXTRA_MODULES_PATH`
   - Cliquez sur la valeur
   - Mettez: `C:/opencv_build/opencv_contrib-4.10.0/modules`

9. Recherchez `CMAKE_INSTALL_PREFIX`
   - Mettez: `C:/opencv_cuda`

**🔴 CRITIQUE - Options à DÉSACTIVER (décocher)**:

10. Recherchez `BUILD_PERF_TESTS` → ❌ **Décochez**
11. Recherchez `BUILD_TESTS` → ❌ **Décochez**
12. Recherchez `BUILD_EXAMPLES` → ❌ **Décochez**

#### Étape 2.3.4: Re-configurer

1. **Cliquez à nouveau sur "Configure"** (bouton en bas)
2. ⏳ Attendez 1-2 minutes
3. Vous devriez voir en bas: **"Configuring done"** ✅

#### Étape 2.3.5: Vérification FINALE

**Scroll down** dans la liste et cherchez une section qui dit:

```
CUDA:                        YES (ver 12.6, CUFFT CUBLAS FAST_MATH)
cuDNN:                       YES (ver 8.9.7)
```

✅ Si vous voyez ça, **PARFAIT!**

❌ Si vous voyez `CUDA: NO`, quelque chose ne va pas - voir section Dépannage

#### Étape 2.3.6: Générer

1. **Cliquez sur "Generate"** (bouton en bas)
2. ⏳ Attendez 30 secondes
3. Vous devez voir: **"Generating done"** ✅

🎉 **Configuration terminée!** Fermez CMake GUI.

---

## Étape 3: Compilation

### 🚨 ATTENTION: Cette étape prend 1.5 à 3 HEURES

C'est **automatique** - vous pouvez faire autre chose pendant ce temps.

### 3.1 Lancer la compilation

**PowerShell** (en administrateur):

```powershell
cd C:\opencv_build\build

# Compiler (utilisera tous vos CPU cores)
& "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build . --config Release -j 8
```

**Ce qui va se passer**:

- Des tonnes de messages vont défiler
- Vous verrez des pourcentages: `[5%]`, `[10%]`, etc.
- **Durée**: 
  - Fast PC (i9-13900H): ~1.5 heure
  - Medium PC: ~2.5 heures
  - Slow PC: ~3 heures

**🟢 Signes que ça marche bien**:
```
[  5%] Building CXX object ...
[ 10%] Building CXX object ...
[ 15%] Building CXX object ...
```

**🔴 Signes de problème**:
```
error C2065: ...
error LNK2019: ...
FAILED: ...
```

Si vous voyez **"FAILED"**, arrêtez et allez à la section Dépannage.

### 3.2 Pendant la compilation

**Vous pouvez**:
- Utiliser d'autres programmes (navigateur, etc.)
- Laisser tourner pendant la nuit
- Utiliser Netflix 😊

**NE FAITES PAS**:
- ❌ Fermer la fenêtre PowerShell
- ❌ Éteindre le PC
- ❌ Mettre le PC en veille (désactivez-la temporairement)

### 3.3 Compilation terminée

Après 1.5-3 heures, vous devriez voir:

```
[100%] Built target opencv_world
```

✅ **SUCCÈS!** 🎉

---

## Étape 4: Installation

### 4.1 Installer OpenCV compilé

```powershell
cd C:\opencv_build\build

# Installer
& "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --install . --config Release
```

⏳ **5 minutes**

Ça va copier tous les fichiers dans `C:\opencv_cuda\`

### 4.2 Vérifier l'installation

```powershell
# Vérifier que les fichiers sont là
dir "C:\opencv_cuda\x64\vc17\bin\opencv_world4100.dll"
```

✅ Vous devriez voir la DLL

### 4.3 Configurer les variables d'environnement

**PowerShell en ADMINISTRATEUR**:

```powershell
# Définir OpenCV_DIR
[System.Environment]::SetEnvironmentVariable("OpenCV_DIR", "C:\opencv_cuda", "Machine")

# Ajouter au PATH
$currentPath = [System.Environment]::GetEnvironmentVariable("PATH", "Machine")
$newPath = $currentPath + ";C:\opencv_cuda\x64\vc17\bin"
[System.Environment]::SetEnvironmentVariable("PATH", $newPath, "Machine")

# Vérifier
echo $env:OpenCV_DIR
```

**⚠️ IMPORTANT**: Fermez et rouvrez PowerShell pour que les changements prennent effet!

---

## Étape 5: Reconstruire l'application

### 5.1 Nettoyer l'ancien build

```powershell
cd C:\Users\basti\source\repos\RealsenseBodyPose

# Supprimer l'ancien build
Remove-Item -Recurse -Force build
mkdir build
```

### 5.2 Reconfigurer avec CMake

```powershell
cd build

& "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" .. -G "Visual Studio 17 2022" -A x64 -DTENSORRT_DIR="C:/Users/basti/source/repos/RealsenseBodyPose/ThirdParty/TensorRT-8.6.1.6"
```

**Vérifiez dans la sortie**:
```
-- Found OpenCV: C:/opencv_cuda (found version "4.10.0")
```

✅ Parfait!

### 5.3 Recompiler l'application

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build . --config Release -j 8
```

⏳ **2 minutes**

---

## Étape 6: Test de performance

### 6.1 Lancer l'application

```powershell
cd C:\Users\basti\source\repos\RealsenseBodyPose\build\bin\Release

.\RealsenseBodyPose.exe --model ..\..\..\models\yolov8n-pose.onnx --width 1280 --height 720
```

### 6.2 Vérifier le GPU dans la console

Vous devriez voir au démarrage:

```
[INFO] ✅ Using CUDA backend for inference
[INFO] ✅ GPU: NVIDIA GeForce RTX 4070 Laptop GPU
```

✅ **C'est BON!** Le GPU est utilisé!

### 6.3 Vérifier le FPS

Regardez dans la console toutes les 100 frames:

```
[INFO] Frame processing time: 14.5 ms (68.9 FPS max)
```

**Performance attendue**:
- **60-90 FPS** en 1280x720
- **80-120 FPS** en 640x480

🎉 **OBJECTIF ATTEINT!**

---

## Dépannage

### Problème 1: CMake ne trouve pas CUDA

**Symptôme**: `CUDA: NO` dans CMake

**Solution**:
```powershell
# Vérifier CUDA
dir "C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.6"

# Définir manuellement
$env:CUDA_PATH = "C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.6"
```

Puis relancez CMake.

---

### Problème 2: Erreur de compilation "out of memory"

**Symptôme**:
```
LINK : fatal error LNK1102: out of memory
```

**Solution**:
```powershell
# Compiler avec moins de threads
cmake --build . --config Release -j 4  # Au lieu de -j 8
```

---

### Problème 3: cuDNN non trouvé

**Symptôme**: `cuDNN: NO` dans CMake

**Solution**:

Vous avez déjà cuDNN dans `ThirdParty/`. Copions-le:

```powershell
# Copier cuDNN dans CUDA
xcopy "C:\Users\basti\source\repos\RealsenseBodyPose\ThirdParty\cudnn-windows-x86_64-8.9.7.29_cuda12\bin\*.dll" "C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.6\bin\" /Y /I

xcopy "C:\Users\basti\source\repos\RealsenseBodyPose\ThirdParty\cudnn-windows-x86_64-8.9.7.29_cuda12\include\*.h" "C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.6\include\" /Y /I

xcopy "C:\Users\basti\source\repos\RealsenseBodyPose\ThirdParty\cudnn-windows-x86_64-8.9.7.29_cuda12\lib\x64\*.lib" "C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.6\lib\x64\" /Y /I
```

*(Nécessite PowerShell en administrateur)*

---

### Problème 4: Application toujours lente après rebuild

**Vérifications**:

1. **Est-ce que CUDA est vraiment utilisé?**
   ```powershell
   # Lancer l'app et chercher cette ligne:
   # [INFO] ✅ Using CUDA backend for inference
   ```

2. **Vérifier la DLL OpenCV**:
   ```powershell
   # L'app doit utiliser la nouvelle DLL
   dir "C:\Users\basti\source\repos\RealsenseBodyPose\build\bin\Release\opencv_world4100.dll"
   
   # Vérifier le timestamp - doit être récent
   ```

3. **Forcer le rebuild complet**:
   ```powershell
   cd C:\Users\basti\source\repos\RealsenseBodyPose
   Remove-Item -Recurse -Force build
   # Puis refaire Étape 5
   ```

---

## 📊 Récapitulatif des performances

| Configuration | FPS | Utilisation |
|---------------|-----|-------------|
| **Avant (OpenCV sans CUDA)** | 7-10 FPS | CPU 100% |
| **Après (OpenCV avec CUDA)** | 60-90 FPS | GPU 60-80% |
| **Gain** | **~8x plus rapide** | ⚡ GPU accéléré |

---

## 🎓 Qu'avez-vous appris?

✅ Compiler du code source (OpenCV)  
✅ Configurer avec CMake  
✅ Activer le support GPU (CUDA)  
✅ Lier des bibliothèques  
✅ Optimiser pour la performance

---

## 📞 Besoin d'aide?

Si vous êtes bloqué:

1. **Notez l'erreur exacte** (copier-coller le message)
2. **Notez à quelle étape** (numéro de section)
3. **Faites une capture d'écran** si c'est dans CMake GUI
4. Demandez de l'aide avec ces informations!

---

## ✅ Checklist finale

Avant de commencer, assurez-vous:

- [ ] ~20 GB d'espace disque libre
- [ ] Connection internet stable
- [ ] 3-4 heures de disponibilité (pour la compilation)
- [ ] PowerShell en mode Administrateur
- [ ] Café ☕ (optionnel mais recommandé)

**Bonne chance!** 🚀

---

*Guide créé pour votre projet RealSense 3D Skeletal Tracking*  
*Version: 1.0 - Décembre 2025*
