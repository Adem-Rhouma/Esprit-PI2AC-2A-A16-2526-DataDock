# Nom du projet
**DataDock**

## Description
DataDock is a comprehensive desktop application developed in C++ using the Qt framework for managing a smart fishing port. The system integrates various hardware components (IoT via Arduino), such as automated barriers, as well as Artificial Intelligence (Machine Learning) for face recognition (`faceidd`) and predictive analytics, providing an all-in-one management platform.

## Technologies utilisées
**Frontend / Backend:** Qt 6 (C++17)
**Database:** Oracle
**Embedded / IoT:** Arduino (C++)
**Artificial Intelligence:** Python (Machine Learning, Face Recognition)

## Prérequis
- Desktop Qt 6+
- C++17
- python 3.10+
- Oracle 11g XE
- Arduino IDE

## Installation
### 1. Main Application (C++ Qt)

Open the `DataDock.pro` file in Qt Creator.

Configure the project with the appropriate kit (Desktop Qt 5 or 6), then build and run it (`Ctrl + R`).

### 2. DataBase

The SQL script required to create and initialize the database is available in:

`standardize_db.sql`

Run this script on your Oracle database before starting the application.

### 3. Artificial Intelligence Modules

faceid Model download link: [Download Model](https://huggingface.co/AdamRh/faceid/tree/main)

Once downloaded, place the model inside the `faceidd/models` directory.

To install the ML module dependencies:
```bash
cd faceidd

python -m venv venv

source venv/bin/activate

venv\Scripts\activate

pip install -r requirements.txt

cd ../gestion_chambres_froides/ai_engine

pip install -r requirements.txt
```

please read `faceidd/README.md` for more information.

logistique Model download link: [Download Model](https://huggingface.co/AdamRh/ai_operations_predictor/tree/main)

Once downloaded, place the model inside the `gestion_logistique/ai_operations_predictor/model` directory.

To install the logistique module dependencies:
```bash
cd gestion_logistique/ai_operations_predictor

pip install -r requirements.txt

```

### 4. Embedded & IoT Module (Arduino)

The board source code files are located in the project root:

* `arduinoprojetqt.ino`
* `barrier_auto.ino`

Use the Arduino IDE to upload the code:

1. Open the `.ino` file in Arduino IDE.
2. Verify/compile the code.
3. Upload it to the connected board.


### 5. Hardware & Wiring (IoT)

* **[Bill of Materials (BOM)](docs/liste-materiel.md)**


## Lancement
Open the `DataDock.pro` file in Qt Creator.

Desktop Qt 6 Run `Ctrl + R`.

## Variables d'environnement

create `.env` file.
See the `.env.example` file for the required environment variables.

## Démo
Vidéo : https://...
Déploiement : https://...

## Auteurs
Nom — Classe — Année — Tuteur :
Adam Rhouma _ 2A16 _ 2526 _ Mme Houde Jouini
Emna Ben Othman _ 2A16 _ 2526 _ Mme Houde Jouini
Youssef Ben Hariz _ 2A16 _ 2526 _ Mme Houde Jouini
Yassine Benzid _ 2A16 _ 2526 _ Mme Houde Jouini
Farah Mejeldi _ 2A16 _ 2526 _ Mme Houde Jouini
Ribel Ben Abdallah _ 2A16 _ 2526 _ Mme Houde Jouini