#ifndef NAVIREDATABASEHELPER_H
#define NAVIREDATABASEHELPER_H

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QVariant>

#include <QDate>
#include <QDateTime>
#include "connection.h"

class NavireDatabaseHelper {
public:
    static bool initializeSchema() {
        Connection &c = Connection::createInstance();
        if (!c.db.isOpen()) {
            if (!c.createConnection()) {
                qDebug() << "Failed to connect to DB for migrations.";
                return false;
            }
        }

        QSqlQuery query(c.db);

        // 1. ALTER NAVIRES (Add MAX_CARGO_CAPACITY if missing)
        query.exec("ALTER TABLE NAVIRES ADD MAX_CARGO_CAPACITY NUMBER(10,2) DEFAULT 0 NOT NULL");
        
        // Trigger for NAVIRES if not already present
        query.exec(R"(
            CREATE OR REPLACE TRIGGER TRG_NAVIRES_ID
            BEFORE INSERT ON NAVIRES
            FOR EACH ROW
            BEGIN
                IF :NEW.IDNAVIRE IS NULL OR :NEW.IDNAVIRE = 0 THEN
                    SELECT NAVIRE_SEQ.NEXTVAL INTO :NEW.IDNAVIRE FROM DUAL;
                END IF;
            END;
        )");

        // 2. VESSEL_LOGS table (No IDENTITY)
        QString createLogsTable = R"(
            CREATE TABLE VESSEL_LOGS (
                IDLOG NUMBER PRIMARY KEY,
                IDNAVIRE NUMBER NOT NULL,
                DEPARTURE_TIME DATE NOT NULL,
                RETURN_TIME DATE NOT NULL,
                DISTANCE_NM NUMBER(10,2) NOT NULL,
                FUEL_CONSUMED NUMBER(10,2) NOT NULL,
                CARGO_LOAD_TONS NUMBER(10,2) NOT NULL,
                AVG_STRAIN_INDEX NUMBER(5,2) DEFAULT 0 NOT NULL,
                WAVE_EXPOSURE_HOURS NUMBER(5,2) DEFAULT 0 NOT NULL,
                DESCRIPTION VARCHAR2(1000),
                CONSTRAINT FK_VLOGS_NAVIRE FOREIGN KEY (IDNAVIRE) REFERENCES NAVIRES(IDNAVIRE) ON DELETE CASCADE
            )
        )";
        query.exec(createLogsTable);
        query.exec("ALTER TABLE VESSEL_LOGS ADD DESCRIPTION VARCHAR2(1000)");

        // Sequence and Trigger for Logs
        query.exec("CREATE SEQUENCE VESSEL_LOGS_SEQ START WITH 1");
        query.exec(R"(
            CREATE OR REPLACE TRIGGER TRG_VESSEL_LOGS_ID
            BEFORE INSERT ON VESSEL_LOGS
            FOR EACH ROW
            BEGIN
                IF :NEW.IDLOG IS NULL THEN
                    SELECT VESSEL_LOGS_SEQ.NEXTVAL INTO :NEW.IDLOG FROM DUAL;
                END IF;
            END;
        )");

        // 3. MAINTENANCE_TASKS table (No IDENTITY)
        QString createMaintTable = R"(
            CREATE TABLE MAINTENANCE_TASKS (
                IDTASK NUMBER PRIMARY KEY,
                IDNAVIRE NUMBER NOT NULL,
                SCHEDULED_DATE DATE NOT NULL,
                TASK_TYPE VARCHAR2(50) NOT NULL,
                TASK_STATUS VARCHAR2(20) DEFAULT 'PENDING' NOT NULL,
                PRIORITY NUMBER(1) DEFAULT 3,
                CONSTRAINT FK_MAINT_NAVIRE FOREIGN KEY (IDNAVIRE) REFERENCES NAVIRES(IDNAVIRE) ON DELETE CASCADE,
                CONSTRAINT CHK_MAINT_STATUS CHECK (TASK_STATUS IN ('PENDING', 'COMPLETED', 'OVERDUE'))
            )
        )";
        query.exec(createMaintTable);

        // Sequence and Trigger for Maintenance
        query.exec("CREATE SEQUENCE MAINTENANCE_TASKS_SEQ START WITH 1");
        query.exec(R"(
            CREATE OR REPLACE TRIGGER TRG_MAINT_TASKS_ID
            BEFORE INSERT ON MAINTENANCE_TASKS
            FOR EACH ROW
            BEGIN
                IF :NEW.IDTASK IS NULL THEN
                    SELECT MAINTENANCE_TASKS_SEQ.NEXTVAL INTO :NEW.IDTASK FROM DUAL;
                END IF;
            END;
        )");

        // 4. Indexes
        query.exec("CREATE INDEX IDX_VLOGS_NAVIRE ON VESSEL_LOGS(IDNAVIRE)");
        query.exec("CREATE INDEX IDX_VLOGS_RETURN ON VESSEL_LOGS(RETURN_TIME)");
        query.exec("CREATE INDEX IDX_MAINT_SCHEDULE ON MAINTENANCE_TASKS(SCHEDULED_DATE)");
        
        // 5. WEATHER_HISTORY table (Unified Caching + History)
        QString createWeatherTable = R"(
            CREATE TABLE WEATHER_HISTORY (
                RECORD_DATE DATE PRIMARY KEY,
                JSON_DATA CLOB NOT NULL,
                WIND_SPEED NUMBER(5,2),
                WAVE_HEIGHT NUMBER(5,2),
                VISIBILITY NUMBER(5,2),
                PORT_OPERATIONAL_RISK_SCORE NUMBER(3)
            )
        )";
        query.exec(createWeatherTable);
        query.exec("CREATE INDEX IDX_WEATHER_DATE ON WEATHER_HISTORY(RECORD_DATE)");

        return true;
    }
};

#endif // NAVIREDATABASEHELPER_H
