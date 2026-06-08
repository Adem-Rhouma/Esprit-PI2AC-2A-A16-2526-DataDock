# DataDock - Smart Fishing Port Management Platform

## Application Overview

DataDock is an integrated desktop management solution designed for modern smart fishing ports. The application unifies all operational aspects of port management into a single platform, leveraging Qt 6 for the user interface, Oracle for data persistence, Arduino for IoT connectivity, and Python-based AI services for intelligent decision-making.

## Core Functional Modules

### 1. Employee Management
- **Employee Registry**: Complete CRUD operations for staff records
- **Role-Based Access**: Admin, Agent, HR, Driver, and Fisher roles with distinct permissions
- **Biometric Integration**: Face recognition enrollment using OpenCV YuNet/SFace models
- **Document Generation**: Automated work certificate and regulation PDF exports
- **RFID Access Control**: Hardware-integrated entry permissions with real-time validation

### 2. Fishing Operations
- **Catch Lot Management**: Registration and tracking of fishing lots by species and quantity
- **Dynamic Pricing**: AI-powered price recommendations via Gemini API based on market conditions
- **Market Intelligence**: Sales recommendations, freshness monitoring, and anomaly detection
- **Traceability**: QR-code enabled fish batch tracking from catch to sale
- **Historical Analytics**: Statistics dashboard with trend visualization

### 3. Vessel Management
- **Vessel Registry**: Ship registration with operational status monitoring
- **Weather Integration**: Real-time meteorological data for departure/arrival planning
- **Predictive Maintenance**: Schedule-based maintenance with Gantt chart visualization
- **Maintenance Alerts**: Automated notifications based on operational hours and conditions

### 4. Cold Storage (Chambres Froides)
- **Inventory Tracking**: Temperature and humidity monitoring for each cold room
- **Smart Allocation**: AI-optimized fish batch distribution using OR-Tools constraint solver
- **RFID/NFC Tagging**: Automated inventory identification and traceability
- **Alert Center**: Centralized view of storage anomalies and quality issues

### 5. Logistics & Transport
- **Fleet Management**: Refrigerated truck registration with capacity and load tracking
- **RFID Gate Control**: Automated truck entry/exit via Arduino-connected barriers
- **Activity Logging**: Complete history of truck movements with timestamps
- **Predictive Analytics**: 90-day operation forecasting using CatBoost ML model
- **Geographic View**: Interactive map showing truck positioning and routes

### 6. Equipment Management
- **Asset Inventory**: Equipment catalog with maintenance scheduling
- **IoT Monitoring**: Real-time sensor data integration for critical assets

## Technology Stack

| Layer | Technology |
|-------|------------|
| Frontend/UI | Qt 6 (C++17) |
| Backend | Qt/C++ with Oracle QSqlDatabase |
| Database | Oracle 11g XE |
| AI Services | Python 3.10+ (Flask, OpenCV, CatBoost, OR-Tools) |
| IoT Integration | Arduino (C++), Qt SerialPort |

## Installation Prerequisites

- Qt 6+ development environment
- C++17 compatible compiler
- Python 3.10+ with pip
- Oracle 11g XE database
- Arduino IDE 1.8+

## Quick Start

1. **Database Setup**: Execute `standardize_db.sql` on Oracle
2. **Build Application**: Open `DataDock.pro` in Qt Creator and build
3. **AI Services**: Install Python dependencies and download ML models
4. **IoT Setup**: Flash `arduinoprojetqt.ino` and `barrier_auto.ino` to Arduino boards
5. **Launch**: Run from Qt Creator (`Ctrl+R`)

## Documentation

- System regulation PDF: Available via Employee module
- Hardware wiring guide: `docs/liste-materiel.md`
- Face recognition API: `faceidd/README.md`

## Development Team

| Developer | Class | Year |
|-----------|-------|------|
| Adam Rhouma | 2A16 | 2526 |
| Emna Ben Othman | 2A16 | 2526 |
| Youssef Ben Hariz | 2A16 | 2526 |
| Yassine Benzid | 2A16 | 2526 |
| Farah Mejeldi | 2A16 | 2526 |
| Ribel Ben Abdallah | 2A16 | 2526 |

*Supervised by: Mme Houde Jouini*