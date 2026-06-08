-- Standardizing telemetry and inventory metadata for CHAMBRESFROIDES
-- Run this script in your Oracle SQL Command Line or SQL Developer

-- 1. Remove the temporary TYPE_POISSON column
ALTER TABLE CHAMBRESFROIDES DROP COLUMN TYPE_POISSON;

-- 2. Add Humidity and Tag number columns if they don't already exist
-- Note: If they already exist, these commands will fail safely, you can ignore the error "column name already used".
ALTER TABLE CHAMBRESFROIDES ADD HUMIDITY NUMBER DEFAULT 85;
ALTER TABLE CHAMBRESFROIDES ADD TAG_NUMBER VARCHAR2(50);

-- 3. Auto-populating industrial tags for existing records
UPDATE CHAMBRESFROIDES 
SET TAG_NUMBER = 'CHF-' || CF_ID 
WHERE TAG_NUMBER IS NULL OR TAG_NUMBER NOT LIKE 'CHF-%';

-- 4. Creating the Alert Command Center table
-- We check for existence manually since IF NOT EXISTS is not standard in Oracle < 21c
BEGIN
   EXECUTE IMMEDIATE 'CREATE TABLE CHAMBRES_ALERTS (
       ALERT_ID VARCHAR2(100) PRIMARY KEY,
       STATUS VARCHAR2(20) DEFAULT ''New'',
       DATE_CREATED DATE,
       DATE_WORKING DATE,
       DATE_RESOLVED DATE,
       LAST_UPDATE DATE,
       HANDLED_BY VARCHAR2(100) DEFAULT ''System''
   )';
EXCEPTION
   WHEN OTHERS THEN
      IF SQLCODE = -955 THEN
         NULL; -- name is already used by an existing object
      ELSE
         RAISE;
      END IF;
END;
/

-- Commit changes
COMMIT;
