-- ============================================================
-- Oracle Bootstrap Script for NAVIRES Module
-- Run this ONCE in SQL*Plus or Oracle SQL Developer
-- before first use of the Navires CRUD.
-- ============================================================

-- 1. Create the auto-increment sequence for NAVIRES (if not exists)
DECLARE
    v_count NUMBER;
BEGIN
    SELECT COUNT(*) INTO v_count
    FROM USER_SEQUENCES WHERE SEQUENCE_NAME = 'NAVIRE_SEQ';

    IF v_count = 0 THEN
        EXECUTE IMMEDIATE
            'CREATE SEQUENCE NAVIRE_SEQ
             START WITH 1
             INCREMENT BY 1
             NOCACHE
             NOCYCLE';
        DBMS_OUTPUT.PUT_LINE('NAVIRE_SEQ created successfully.');
    ELSE
        DBMS_OUTPUT.PUT_LINE('NAVIRE_SEQ already exists, skipping.');
    END IF;
END;
/

-- 2. Align the sequence to MAX(IDNAVIRE) so no conflict with existing data
DECLARE
    v_max  NUMBER := 0;
    v_curr NUMBER := 0;
    v_diff NUMBER := 0;
    v_dummy NUMBER;
BEGIN
    SELECT NVL(MAX(IDNAVIRE), 0) INTO v_max FROM NAVIRES;
    SELECT NAVIRE_SEQ.NEXTVAL INTO v_curr FROM DUAL;

    -- If current sequence value is behind the max existing ID, fast-forward it
    v_diff := v_max - v_curr;
    IF v_diff > 0 THEN
        EXECUTE IMMEDIATE
            'ALTER SEQUENCE NAVIRE_SEQ INCREMENT BY ' || v_diff;
        SELECT NAVIRE_SEQ.NEXTVAL INTO v_dummy FROM DUAL;
        EXECUTE IMMEDIATE
            'ALTER SEQUENCE NAVIRE_SEQ INCREMENT BY 1';
        DBMS_OUTPUT.PUT_LINE('NAVIRE_SEQ fast-forwarded to ' || (v_curr + v_diff));
    ELSE
        DBMS_OUTPUT.PUT_LINE('NAVIRE_SEQ is already ahead of MAX(IDNAVIRE), no adjustment needed.');
    END IF;
END;
/

COMMIT;
