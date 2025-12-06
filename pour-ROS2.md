# Architecture ROS2 : Communication & Contrôle Robot (G1)

Ce document décrit la stratégie technique pour connecter votre application de vision (Windows/C++/CUDA) à votre robot G1 (ROS2/Linux).

---

## 1. Le Défi Technique (Pourquoi un plan spécifique ?)

Nous avons deux mondes qui ne se parlent pas naturellement :
1.  **Monde Vision (Windows)** : Votre application actuelle. Très performante, utilise Visual Studio, CUDA, et des drivers Windows pour RealSense.
2.  **Monde Robotique (ROS2)** : Votre projet `mobile-robotics-ws`. Tourne standardement sous Linux (Ubuntu) ou WSL, car c'est l'écosystème natif de ROS2 et des drivers Unitree.

**Problème** : Vouloir intégrer les librairies C++ de ROS2 (`rclcpp`) directement dans votre projet Visual Studio actuel est **très risqué**. Cela impliquerait de gérer des conflits de compilateur, de versions de Python, et de DLLs, transformant votre projet stable en usine à gaz.

**La Solution "Expert"** : Le découplage via un **Pont Réseau Léger (UDP)**.

---

## 2. Architecture Proposée : Le "Pont UDP"

L'idée est de laisser chaque système faire ce qu'il fait de mieux, et de les relier par un simple fil réseau.

```mermaid
graph LR
    subgraph Windows [PC Windows (Vision)]
        A[App C++ RealSense] -- Capture --> B(Detection Squelette)
        B -- Coordonnées X,Y,Z --> C[Module UDP Sender]
    end

    subgraph Network [Réseau Local / Localhost]
        C -. JSON/Binary via UDP .-> D
    end

    subgraph RobotEnv [WSL ou Robot (ROS2)]
        D[Pont Python (Receiver)] -- Traduction --> E(ROS2 Publisher)
        E -- /human/target_pose --> F[Contrôleur Robot G1]
    end
```

### Avantages :
*   **Zéro risque** pour votre code C++ actuel. On ajoute juste une petite fonction "Envoyer".
*   **Flexibilité** : Le robot peut être un G1, un bras robotique, ou une simulation Isaac Sim, le code C++ ne change pas.
*   **Performance** : UDP est ultra-rapide, parfait pour du temps réel (latence < 1ms).

---

## 3. Détails d'Implémentation

### Étape 1 : Côté C++ (L'Émetteur)
Dans `Visualizer.cpp` ou une nouvelle classe `NetworkBridge`, nous allons créer une socket UDP simple.
Au lieu d'afficher juste le texte, nous envoyons la structure de données des "poignets" (Wrists).

**Format des données suggéré (JSON pour débuter, Struct Binaire pour la perf plus tard)** :
```json
{
  "timestamp": 123456789,
  "joints": {
    "left_wrist":  { "x": -0.2, "y": 0.5, "z": 1.2, "confidence": 0.9 },
    "right_wrist": { "x": 0.3, "y": 0.4, "z": 1.1, "confidence": 0.8 }
  }
}
```

### Étape 2 : Le Pont ROS2 (Le Traducteur)
Dans votre projet `mobile-robotics-ws`, nous créerons un script Python simple (`human_bridge_node.py`).
*   Il écoute le port `8888` (par exemple).
*   Il parse le JSON.
*   Il publie des messages ROS2 standards.

**Type de Message ROS2** : 
Utiliser `geometry_msgs/msg/PoseStamped` ou `visualization_msgs/msg/Marker` pour commencer à visualiser dans Rviz.

### Étape 3 : Le Cerveau du Robot (Le Mimétisme)
C'est le défi mathématique.
1.  **Changement de Repère (Coordinate Transform)** :
    *   La caméra voit : Z = Profondeur (devant), Y = Bas (souvent), X = Droite.
    *   Le Robot (ROS standard) : X = Devant, Z = Haut, Y = Gauche.
    *   => Il faudra appliquer une matrice de rotation.
2.  **Mise à l'échelle (Retargeting)** :
    *   Votre bras fait 70cm, celui du G1 fait ~40cm.
    *   *Solution* : Ne pas copier la position absolue ("aller au point 3 mètres"). Copier les **vecteurs relatifs** ou les **angles**.
    *   *Approche simple* : Si votre main monte de 10cm, la main du robot monte de 10cm (commande en vitesse ou delta-position).

---

## 4. Plan de Bataille (Roadmap)

### Phase A : Communication (La Voix)
1.  [C++] Ajouter `SimpleUDP` (header only) au projet.
2.  [C++] Envoyer les données "Wrist" à chaque frame (60Hz).
3.  [WSL/ROS2] Script python qui reçoit et fait `print()`.

### Phase B : Visualisation dans l'univers Robot (Les Yeux)
1.  [ROS2] Publier un `Marker` (sphère rouge) dans Rviz qui bouge comme votre main.
2.  [ROS2] Calibrer les axes (X,Y,Z) pour que "Haut" soit "Haut" des deux côtés.

### Phase C : Contrôle du G1 (Le Corps)
1.  Utiliser l'API Unitree (via ROS2 wrapper).
2.  Envoyer les commandes `InverseKinematics` (IK) : "Place ta main ici".
3.  Ajouter des limites de sécurité (Safety Limits) pour ne pas casser le robot si la caméra détecte un point aberrant.

---

## 5. Résumé pour le `mobile-robotics-ws`

Dans votre autre projet, préparez-vous à créer un package `human_control` contenant :
*   `udp_receiver.py` (Noeud d'entrée)
*   `motion_retargeter.py` (Noeud de calcul mathématique)

C'est une architecture propre, modulaire et robuste.
