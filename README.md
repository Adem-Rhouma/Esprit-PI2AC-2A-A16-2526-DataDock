# Smart Fishing Port Management (DataDock)

## Description

DataDock is a comprehensive desktop application developed in C++ using the Qt framework for managing a smart fishing port. The system integrates various hardware components (IoT via Arduino), such as automated barriers, as well as Artificial Intelligence (Machine Learning) for face recognition (`faceidd`) and predictive analytics, providing an all-in-one management platform.

## Technologies Used

* **Frontend / Backend:** C++ (Qt 6)
* **Embedded / IoT:** Arduino (C++)
* **Artificial Intelligence:** Python (Machine Learning, Face Recognition)
* **Database:** Oracle

## Prerequisites

* OS: Windows 10+ or Linux (Ubuntu 20.04+)
* Compiler: g++ 11+ or MSVC 2022
* Framework: Qt 5.15+ or Qt 6+ with Qt Creator
* Python 3.8+ with pip (for the FaceID module)
* Arduino IDE (for flashing code to Arduino/ESP32 boards)

## Installation & Running

### 1. Main Application (C++ Qt)

Open the `DataDock.pro` file in Qt Creator.

Configure the project with the appropriate kit (Desktop Qt 5 or 6), then build and run it (`Ctrl + R`).

To compile from the command line:

```bash
mkdir build && cd build
qmake ../DataDock.pro
make
./DataDock
```

### 2. Artificial Intelligence Modules

Model download link: [Download Model](https://huggingface.co/AdamRh/faceid/tree/main)

Once downloaded, place the model inside the `faceidd` directory.

To install the ML module dependencies:
```bash
cd faceidd

python -m venv venv

source venv/bin/activate

venv\Scripts\activate

pip install -r requirements.txt

```

### 3. Embedded & IoT Module (Arduino)

The board source code files are located in the project root:

* `arduinoprojetqt.ino`
* `barrier_auto.ino`

Use the Arduino IDE to upload the code:

1. Open the `.ino` file in Arduino IDE.
2. Verify/compile the code.
3. Upload it to the connected board.

## Environment Variables

See the `.env` file for the required environment variables.

## Hardware & Wiring (IoT)

* **[Bill of Materials (BOM)](docs/liste-materiel.md)**
* **[Wiring Diagram](docs/schema-cablage.md)**

## Authors

[Vortex Team]
