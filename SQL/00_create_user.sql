-- ============================================================
--  Smart Weather - Création de l'utilisateur Oracle METEO (facultatif)
--
--  À exécuter UNE SEULE FOIS avec un compte administrateur (SYSTEM).
--  Les identifiants correspondent à Sources/connection.cpp :
--      source ODBC : smartweatherDSN   utilisateur : meteo   mot de passe : meteo5
--  Puis se reconnecter en METEO et lancer install_all.sql.
-- ============================================================
SET DEFINE OFF

DECLARE
    v_existe NUMBER;
BEGIN
    SELECT COUNT(*) INTO v_existe FROM DBA_USERS WHERE USERNAME = 'METEO';
    IF v_existe = 0 THEN
        EXECUTE IMMEDIATE 'CREATE USER meteo IDENTIFIED BY meteo5 '
                       || 'DEFAULT TABLESPACE users QUOTA UNLIMITED ON users';
    END IF;
END;
/

-- Droits strictement nécessaires aux scripts et à l'application.
GRANT CREATE SESSION, CREATE TABLE, CREATE SEQUENCE, CREATE TRIGGER TO meteo;
