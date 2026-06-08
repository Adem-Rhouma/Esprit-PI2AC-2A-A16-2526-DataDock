# Face API Flask

Mini-projet Python 3 avec Flask pour exposer une API locale de reconnaissance faciale utilisable depuis une application Qt C++ et une base Oracle.

## Arborescence

```text
face_api/
├── app.py
├── requirements.txt
└── README.md
```

## Fonctionnalites

- `POST /face/register`
  - recoit une image en `multipart/form-data`
  - verifie qu'un seul visage est present
  - controle simplement que l'image n'est pas trop floue
  - extrait un embedding facial avec OpenCV SFace

- `POST /face/verify`
  - recoit une image en `multipart/form-data`
  - recoit un embedding de reference via JSON ou champ `form-data`
  - extrait un nouvel embedding
  - calcule une distance euclidienne
  - compare avec un seuil configurable

## Installation

### 1. Creer un environnement virtuel

Sous Windows PowerShell :

```powershell
python -m venv .venv
.venv\Scripts\Activate.ps1
```

Sous Linux/macOS :

```bash
python3 -m venv .venv
source .venv/bin/activate
```

### 2. Installer les dependances

```bash
pip install -r requirements.txt
```

## Modeles OpenCV a ajouter

Ajoute ces 2 fichiers ONNX dans le dossier `models/` :

- `models/face_detection_yunet_2023mar.onnx`
- `models/face_recognition_sface_2021dec.onnx`

Sources officielles OpenCV :

- OpenCV Zoo : https://github.com/opencv/opencv_zoo
- YuNet : https://github.com/opencv/opencv_zoo/tree/master/models/face_detection_yunet
- SFace : https://github.com/opencv/opencv_zoo/tree/master/models/face_recognition_sface

## Lancement

Depuis le dossier `face_api/` :

```bash
python app.py
```

Par defaut, l'API demarre sur :

```text
http://127.0.0.1:5000
```

## Configuration

Les principaux parametres sont directement en haut de `app.py` :

- `HOST`
- `PORT`
- `MATCH_THRESHOLD`
- `BLUR_THRESHOLD`
- `FACE_DETECTOR_MODEL`
- `FACE_RECOGNIZER_MODEL`

## Exemples de requetes HTTP

### Inscription faciale

```bash
curl -X POST http://127.0.0.1:5000/face/register \
  -F "image=@photos/user.jpg"
```

### Verification avec embedding en champ multipart

```bash
curl -X POST http://127.0.0.1:5000/face/verify \
  -F "image=@photos/login.jpg" \
  -F "embedding=[0.12, -0.04, 0.33, ...]"
```

### Verification avec embedding en JSON

Si vous voulez envoyer le JSON dans le body, il faut que Qt ou un client HTTP envoie l'image en multipart et l'embedding dans un champ `embedding`. C'est le mode le plus simple pour ce projet.

## Format JSON renvoye

### `POST /face/register`

Succes :

```json
{
  "success": true,
  "message": "Inscription faciale prete. Embedding extrait avec succes.",
  "face_detected": true,
  "quality_ok": true,
  "embedding": [0.123, -0.456, 0.789]
}
```

Erreur :

```json
{
  "success": false,
  "message": "Aucun visage detecte dans l'image.",
  "face_detected": false,
  "quality_ok": false,
  "embedding": null
}
```

### `POST /face/verify`

Succes :

```json
{
  "success": true,
  "match": true,
  "distance": 0.412315,
  "threshold": 0.48,
  "message": "Verification faciale reussie : correspondance confirmee."
}
```

Erreur :

```json
{
  "success": false,
  "match": false,
  "distance": null,
  "threshold": 0.48,
  "message": "Embedding de reference manquant dans la requete /face/verify."
}
```

## Logique de matching

- l'inscription ne stocke jamais l'image, seulement l'embedding
- le login compare deux embeddings via une distance euclidienne
- il ne faut jamais faire une comparaison exacte comme pour un mot de passe
- plus la distance est petite, plus les visages sont proches

Le seuil par defaut est `0.48`. En pratique, il faut le recalibrer avec vos propres images.

## Integration cote Qt C++

Approche recommandee :

1. Lors de l'inscription :
   - capturer l'image depuis la webcam ou charger un fichier
   - envoyer l'image a `POST /face/register` via `QNetworkAccessManager`
   - recuperer `embedding`
   - serialiser ce tableau JSON
   - stocker cet embedding dans Oracle dans un champ `CLOB` ou `VARCHAR2` assez grand

2. Lors du login :
   - recuperer l'embedding de l'utilisateur depuis Oracle
   - envoyer l'image courante + l'embedding stocke a `POST /face/verify`
   - autoriser la connexion si `match == true`

Conseils pratiques :

- stocker l'embedding sous forme JSON texte dans Oracle pour simplifier l'integration
- ajouter un timeout reseau cote Qt
- afficher au client les messages d'erreur renvoyes par l'API
- journaliser `distance` et `threshold` pour faciliter les tests
- ne pas lancer l'API Flask depuis le thread UI de Qt

## Exemple de payload a stocker dans Oracle

```json
[-0.0912, 0.1844, -0.0321, 0.0765]
```

## Limitations

- solution simple et academique, suffisante pour un PFE ou une maquette robuste
- la reconnaissance depend de 2 modeles ONNX OpenCV a poser localement dans `models/`
- la qualite d'image repose ici sur un controle simple du flou
- pour un projet de production, il faudrait durcir la securite, la journalisation, les tests et l'anti-spoofing
