-- ============================================================
--  Smart Weather - Module "Gestion des Zones" (Adem Nouioui)
--  Oracle 11g XE - à exécuter avec l'utilisateur METEO
--  (SQL Developer : ouvrir ce fichier, puis "Exécuter le script" = F5)
--
--  Tables      : ZONE, ZONE_HISTORIQUE
--  Prérequis   : aucun (ZONE est la table racine : les autres modules
--                INTERVENTION et AGENT la référencent)
--  Re-exécutable : OUI - supprime puis recrée ses objets. Pour repartir
--                de zéro sur toute la base, lancer plutôt install_all.sql
--                (supprimer ZONE seule retire aussi les clés étrangères
--                des tables qui la référencent).
--  Encodage    : fichier en UTF-8 (SQL Developer > Outils > Préférences >
--                Environnement > Encodage = UTF-8).
-- ============================================================
SET DEFINE OFF

-- ---------- Nettoyage (aucune erreur si les objets n'existent pas) ----------
BEGIN
    FOR t IN (SELECT table_name FROM user_tables
              WHERE table_name IN ('ZONE_HISTORIQUE', 'ZONE')) LOOP
        EXECUTE IMMEDIATE 'DROP TABLE ' || t.table_name || ' CASCADE CONSTRAINTS PURGE';
    END LOOP;
    FOR s IN (SELECT sequence_name FROM user_sequences
              WHERE sequence_name IN ('SEQ_ZONE', 'SEQ_ZONE_HISTORIQUE')) LOOP
        EXECUTE IMMEDIATE 'DROP SEQUENCE ' || s.sequence_name;
    END LOOP;
END;
/

-- ---------- Séquences (auto-incrément sous Oracle 11g) ----------
CREATE SEQUENCE SEQ_ZONE            START WITH 1 INCREMENT BY 1 NOCACHE;
CREATE SEQUENCE SEQ_ZONE_HISTORIQUE START WITH 1 INCREMENT BY 1 NOCACHE;

-- ---------- Table ZONE ----------
CREATE TABLE ZONE (
    ID_ZONE        NUMBER(10)    NOT NULL,
    NOM            VARCHAR2(100) NOT NULL,
    CODE           VARCHAR2(20)  NOT NULL,
    VULNERABILITE  NUMBER(4,1)   DEFAULT 0 NOT NULL,
    LATITUDE       NUMBER(9,6)   DEFAULT 36.8065 NOT NULL,
    LONGITUDE      NUMBER(9,6)   DEFAULT 10.1815 NOT NULL,
    STATUT         VARCHAR2(10)  DEFAULT 'Active' NOT NULL,
    CONSTRAINT PK_ZONE        PRIMARY KEY (ID_ZONE),
    CONSTRAINT CK_ZONE_VULN   CHECK (VULNERABILITE BETWEEN 0 AND 100),
    CONSTRAINT CK_ZONE_LAT    CHECK (LATITUDE  BETWEEN -90  AND 90),
    CONSTRAINT CK_ZONE_LON    CHECK (LONGITUDE BETWEEN -180 AND 180),
    CONSTRAINT CK_ZONE_STATUT CHECK (STATUT IN ('Active', 'Inactive'))
);

-- Code unique SANS tenir compte de la casse : l'application (ZoneService::valider)
-- compare déjà les codes en insensible à la casse, la base fait donc pareil.
CREATE UNIQUE INDEX UQ_ZONE_CODE ON ZONE (UPPER(CODE));

-- ---------- Table ZONE_HISTORIQUE (métier 5 : historique d'évolution) ----------
CREATE TABLE ZONE_HISTORIQUE (
    ID_HIST         NUMBER(10)     NOT NULL,
    ID_ZONE         NUMBER(10)     NOT NULL,
    DATE_CHANGEMENT DATE           DEFAULT SYSDATE NOT NULL,
    CHAMP           VARCHAR2(50)   NOT NULL,
    ANCIENNE_VALEUR VARCHAR2(200),
    NOUVELLE_VALEUR VARCHAR2(200),
    CONSTRAINT PK_ZONE_HISTORIQUE PRIMARY KEY (ID_HIST),
    CONSTRAINT FK_HIST_ZONE FOREIGN KEY (ID_ZONE)
        REFERENCES ZONE (ID_ZONE) ON DELETE CASCADE
);

-- Sert aussi d'index à la clé étrangère (ID_ZONE en tête).
CREATE INDEX IDX_HIST_ZONE ON ZONE_HISTORIQUE (ID_ZONE, DATE_CHANGEMENT);

-- ---------- Triggers : clé primaire automatique si non fournie ----------
CREATE OR REPLACE TRIGGER TRG_ZONE_ID
BEFORE INSERT ON ZONE
FOR EACH ROW
WHEN (NEW.ID_ZONE IS NULL)
BEGIN
    :NEW.ID_ZONE := SEQ_ZONE.NEXTVAL;
END;
/

CREATE OR REPLACE TRIGGER TRG_ZONE_HISTORIQUE_ID
BEFORE INSERT ON ZONE_HISTORIQUE
FOR EACH ROW
WHEN (NEW.ID_HIST IS NULL)
BEGIN
    :NEW.ID_HIST := SEQ_ZONE_HISTORIQUE.NEXTVAL;
END;
/

-- ---------- Jeu de données de démonstration ----------
-- Les identifiants viennent des triggers (pas besoin de recaler les séquences).
INSERT INTO ZONE (NOM, CODE, VULNERABILITE, LATITUDE, LONGITUDE, STATUT) VALUES ('Tunis Centre', 'ZN-01', 62, 36.8065, 10.1815, 'Active');
INSERT INTO ZONE (NOM, CODE, VULNERABILITE, LATITUDE, LONGITUDE, STATUT) VALUES ('Ariana',       'ZN-02', 48, 36.8665, 10.1647, 'Active');
INSERT INTO ZONE (NOM, CODE, VULNERABILITE, LATITUDE, LONGITUDE, STATUT) VALUES ('Ben Arous',    'ZN-03', 71, 36.7531, 10.2189, 'Active');
INSERT INTO ZONE (NOM, CODE, VULNERABILITE, LATITUDE, LONGITUDE, STATUT) VALUES ('Manouba',      'ZN-04', 35, 36.8101, 10.0863, 'Active');
INSERT INTO ZONE (NOM, CODE, VULNERABILITE, LATITUDE, LONGITUDE, STATUT) VALUES ('Bizerte',      'ZN-05', 55, 37.2744,  9.8739, 'Active');
INSERT INTO ZONE (NOM, CODE, VULNERABILITE, LATITUDE, LONGITUDE, STATUT) VALUES ('Nabeul',       'ZN-06', 40, 36.4513, 10.7357, 'Active');

-- Historique : une ligne "Création" par zone (comme le fait l'application)...
INSERT INTO ZONE_HISTORIQUE (ID_ZONE, DATE_CHANGEMENT, CHAMP, NOUVELLE_VALEUR)
SELECT ID_ZONE, TRUNC(SYSDATE) - 60 + 9/24, 'Création', NOM FROM ZONE;

-- ... puis quelques modifications (dates relatives : la démo reste cohérente).
INSERT INTO ZONE_HISTORIQUE (ID_ZONE, DATE_CHANGEMENT, CHAMP, ANCIENNE_VALEUR, NOUVELLE_VALEUR)
SELECT ID_ZONE, TRUNC(SYSDATE) - 45 + 9/24, 'Vulnérabilité', '65', '71' FROM ZONE WHERE CODE = 'ZN-03';
INSERT INTO ZONE_HISTORIQUE (ID_ZONE, DATE_CHANGEMENT, CHAMP, ANCIENNE_VALEUR, NOUVELLE_VALEUR)
SELECT ID_ZONE, TRUNC(SYSDATE) - 42 + 9/24, 'Statut', 'Inactive', 'Active' FROM ZONE WHERE CODE = 'ZN-03';
INSERT INTO ZONE_HISTORIQUE (ID_ZONE, DATE_CHANGEMENT, CHAMP, ANCIENNE_VALEUR, NOUVELLE_VALEUR)
SELECT ID_ZONE, TRUNC(SYSDATE) - 39 + 9/24, 'Vulnérabilité', '50', '55' FROM ZONE WHERE CODE = 'ZN-05';

COMMIT;
