-- ============================================================
--  Smart Weather - Module "Gestion des Fournisseurs"
--  Oracle 11g XE - à exécuter avec l'utilisateur METEO
--  (SQL Developer : ouvrir ce fichier, puis "Exécuter le script" = F5)
--
--  Tables      : FOURNISSEUR
--  Prérequis   : aucun
--  Re-exécutable : OUI - supprime puis recrée ses objets.
--  Encodage    : fichier en UTF-8.
--
--  Le code C++ (Sources/fournisseur.cpp) utilise EXACTEMENT ces noms de
--  tables et de colonnes. Types et statuts = ceux de Headers/fournisseur.h :
--  si vous en ajoutez un là-bas, ajoutez-le aussi dans les CHECK ci-dessous.
-- ============================================================
SET DEFINE OFF

-- ---------- Nettoyage (aucune erreur si les objets n'existent pas) ----------
BEGIN
    FOR t IN (SELECT table_name FROM user_tables
              WHERE table_name IN ('FOURNISSEUR')) LOOP
        EXECUTE IMMEDIATE 'DROP TABLE ' || t.table_name || ' CASCADE CONSTRAINTS PURGE';
    END LOOP;
    FOR s IN (SELECT sequence_name FROM user_sequences
              WHERE sequence_name IN ('SEQ_FOURNISSEUR')) LOOP
        EXECUTE IMMEDIATE 'DROP SEQUENCE ' || s.sequence_name;
    END LOOP;
END;
/

-- ---------- Séquences (auto-incrément sous Oracle 11g) ----------
CREATE SEQUENCE SEQ_FOURNISSEUR            START WITH 1 INCREMENT BY 1 NOCACHE;

-- ---------- Table FOURNISSEUR ----------
CREATE TABLE FOURNISSEUR (
    ID_FOURNISSEUR  NUMBER(10)    NOT NULL,
    NOM             VARCHAR2(100) NOT NULL,
    TYPE            VARCHAR2(50)  NOT NULL,
    CONTACT         VARCHAR2(100),
    EMAIL           VARCHAR2(100),
    TELEPHONE       VARCHAR2(30),
    ADRESSE         VARCHAR2(200),
    GOUVERNORAT     VARCHAR2(50),
    MATRICULE_FISCAL VARCHAR2(30),
    VILLE           VARCHAR2(50),
    NOTE            NUMBER(1)     DEFAULT 3 NOT NULL,
    DELAI           NUMBER(3)     DEFAULT 7 NOT NULL,
    DATE_DEBUT      DATE          NOT NULL,
    DATE_FIN        DATE          NOT NULL,
    STATUT          VARCHAR2(20)  DEFAULT 'Actif' NOT NULL,
    CONSTRAINT PK_FOURNISSEUR     PRIMARY KEY (ID_FOURNISSEUR),
    CONSTRAINT CK_FOURN_TYPE      CHECK (TYPE IN ('Capteurs et stations', 'Maintenance', UNISTR('\00c9nergie'), UNISTR('R\00e9seau et t\00e9l\00e9com'), 'Informatique')),
    CONSTRAINT CK_FOURN_NOTE      CHECK (NOTE BETWEEN 1 AND 5),
    CONSTRAINT CK_FOURN_DELAI     CHECK (DELAI BETWEEN 1 AND 365),
    CONSTRAINT CK_FOURN_DATES     CHECK (DATE_FIN >= DATE_DEBUT),
    CONSTRAINT CK_FOURN_STATUT    CHECK (STATUT IN ('Actif', 'Suspendu', UNISTR('Archiv\00e9')))
);

-- ---------- Triggers : clé primaire automatique si non fournie ----------
CREATE OR REPLACE TRIGGER TRG_FOURNISSEUR_ID
BEFORE INSERT ON FOURNISSEUR
FOR EACH ROW
WHEN (NEW.ID_FOURNISSEUR IS NULL)
BEGIN
    :NEW.ID_FOURNISSEUR := SEQ_FOURNISSEUR.NEXTVAL;
END;
/

-- ---------- Données de démonstration ----------
-- Les dates sont relatives à aujourd'hui pour que les alertes
-- "contrat à renouveler" (moins de 30 jours) restent visibles.
INSERT INTO FOURNISSEUR (NOM, TYPE, CONTACT, EMAIL, TELEPHONE, ADRESSE, GOUVERNORAT, MATRICULE_FISCAL, VILLE, NOTE, DELAI, DATE_DEBUT, DATE_FIN, STATUT)
VALUES ('Meteo Sensors TN', 'Capteurs et stations', 'Sami Ben Ali', 'sami@meteosensors.tn', '71000001', UNISTR('Rue de la Libert\00e9'), 'Tunis', '1234567/A/M/000', 'Tunis', 5, 10, TRUNC(SYSDATE) - 365, TRUNC(SYSDATE) + 200, 'Actif');

INSERT INTO FOURNISSEUR (NOM, TYPE, CONTACT, EMAIL, TELEPHONE, ADRESSE, GOUVERNORAT, MATRICULE_FISCAL, VILLE, NOTE, DELAI, DATE_DEBUT, DATE_FIN, STATUT)
VALUES ('SolarTunis', UNISTR('\00c9nergie'), 'Rim Trabelsi', 'rim@solartunis.tn', '71000002', 'Avenue Habib Bourguiba', 'Ariana', '2345678/B/M/000', 'Ariana', 4, 14, TRUNC(SYSDATE) - 365, TRUNC(SYSDATE) + 20, 'Actif');

INSERT INTO FOURNISSEUR (NOM, TYPE, CONTACT, EMAIL, TELEPHONE, ADRESSE, GOUVERNORAT, MATRICULE_FISCAL, VILLE, NOTE, DELAI, DATE_DEBUT, DATE_FIN, STATUT)
VALUES (UNISTR('R\00e9seau Plus'), UNISTR('R\00e9seau et t\00e9l\00e9com'), 'Karim Mansour', 'karim@reseauplus.tn', '71000003', 'Zone industrielle', 'Ben Arous', '3456789/C/M/000', 'Ben Arous', 4, 8, TRUNC(SYSDATE) - 365, TRUNC(SYSDATE) + 15, 'Actif');

INSERT INTO FOURNISSEUR (NOM, TYPE, CONTACT, EMAIL, TELEPHONE, ADRESSE, GOUVERNORAT, MATRICULE_FISCAL, VILLE, NOTE, DELAI, DATE_DEBUT, DATE_FIN, STATUT)
VALUES ('InfoPlus', 'Informatique', 'Nour Jaziri', 'nour@infoplus.tn', '71000004', 'Centre urbain nord', 'Tunis', '4567890/D/M/000', 'Tunis', 3, 16, TRUNC(SYSDATE) - 365, TRUNC(SYSDATE) + 300, 'Suspendu');

INSERT INTO FOURNISSEUR (NOM, TYPE, CONTACT, EMAIL, TELEPHONE, ADRESSE, GOUVERNORAT, MATRICULE_FISCAL, VILLE, NOTE, DELAI, DATE_DEBUT, DATE_FIN, STATUT)
VALUES ('AutoPro Tunisie', 'Maintenance', 'Hatem Gharbi', 'hatem@autopro.tn', '71000005', 'Route de Sousse', 'Sousse', '5678901/E/M/000', 'Sousse', 4, 6, TRUNC(SYSDATE) - 200, TRUNC(SYSDATE) + 250, 'Actif');

INSERT INTO FOURNISSEUR (NOM, TYPE, CONTACT, EMAIL, TELEPHONE, ADRESSE, GOUVERNORAT, MATRICULE_FISCAL, VILLE, NOTE, DELAI, DATE_DEBUT, DATE_FIN, STATUT)
VALUES (UNISTR('S\00e9curit\00e9 Pro'), 'Maintenance', 'Leila Ferchichi', 'leila@securitepro.tn', '71000006', UNISTR('Avenue de la R\00e9publique'), 'Sfax', '6789012/F/M/000', 'Sfax', 5, 5, TRUNC(SYSDATE) - 150, TRUNC(SYSDATE) + 400, 'Actif');

COMMIT;
