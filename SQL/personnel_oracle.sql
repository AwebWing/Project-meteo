-- ============================================================
--  Smart Weather - Module "Gestion du Personnel" (page Emploi)
--  Oracle 11g XE - à exécuter avec l'utilisateur METEO
--  (SQL Developer : ouvrir ce fichier, puis "Exécuter le script" = F5)
--
--  Tables      : AGENT
--  Prérequis   : zones_oracle.sql (clé étrangère AGENT.ID_ZONE -> ZONE)
--  Re-exécutable : OUI - supprime puis recrée ses objets (supprimer AGENT
--                seule retire aussi la clé étrangère de INTERVENTION.ID_AGENT :
--                préférer install_all.sql).
--  Encodage    : fichier en UTF-8.
--
--  Les rôles, statuts et quarts de travail sont ceux de la page Emploi
--  (Headers/emploipage.h, Sources/emploipage.cpp).
-- ============================================================
SET DEFINE OFF

-- ---------- Nettoyage (aucune erreur si les objets n'existent pas) ----------
BEGIN
    FOR t IN (SELECT table_name FROM user_tables
              WHERE table_name IN ('AGENT')) LOOP
        EXECUTE IMMEDIATE 'DROP TABLE ' || t.table_name || ' CASCADE CONSTRAINTS PURGE';
    END LOOP;
    FOR s IN (SELECT sequence_name FROM user_sequences
              WHERE sequence_name IN ('SEQ_AGENT')) LOOP
        EXECUTE IMMEDIATE 'DROP SEQUENCE ' || s.sequence_name;
    END LOOP;
END;
/

-- ---------- Séquences (auto-incrément sous Oracle 11g) ----------
CREATE SEQUENCE SEQ_AGENT            START WITH 1 INCREMENT BY 1 NOCACHE;

-- ---------- Table AGENT ----------
CREATE TABLE AGENT (
    ID_AGENT        NUMBER(10)    NOT NULL,
    NOM             VARCHAR2(100) NOT NULL,
    PRENOM          VARCHAR2(100) NOT NULL,
    ROLE            VARCHAR2(50)  NOT NULL,
    STATUT          VARCHAR2(20)  DEFAULT 'Disponible' NOT NULL,
    ID_ZONE         NUMBER(10),
    QUART_TRAVAIL   VARCHAR2(30)  DEFAULT 'Matin (06h-14h)' NOT NULL,
    EMAIL           VARCHAR2(100),
    TELEPHONE       VARCHAR2(30),
    CONSTRAINT PK_AGENT        PRIMARY KEY (ID_AGENT),
    CONSTRAINT CK_AGENT_ROLE   CHECK (ROLE IN ('Technicien Météo', 'Météorologue', 'Superviseur', 'Agent de terrain')),
    CONSTRAINT CK_AGENT_STATUT CHECK (STATUT IN ('En service', 'Disponible', 'En congé', 'Formation')),
    CONSTRAINT CK_AGENT_QUART  CHECK (QUART_TRAVAIL IN ('Matin (06h-14h)', 'Après-midi (14h-22h)', 'Nuit (22h-06h)')),
    -- Un agent n'est pas supprimé avec sa zone : il redevient simplement "sans zone".
    CONSTRAINT FK_AGENT_ZONE   FOREIGN KEY (ID_ZONE) REFERENCES ZONE (ID_ZONE) ON DELETE SET NULL
);

-- Oracle n'indexe pas les clés étrangères : sans cet index, chaque suppression
-- de zone parcourt toute la table AGENT.
CREATE INDEX IDX_AGENT_ZONE ON AGENT (ID_ZONE);

-- ---------- Triggers : clé primaire automatique si non fournie ----------
CREATE OR REPLACE TRIGGER TRG_AGENT_ID
BEFORE INSERT ON AGENT
FOR EACH ROW
WHEN (NEW.ID_AGENT IS NULL)
BEGIN
    :NEW.ID_AGENT := SEQ_AGENT.NEXTVAL;
END;
/

-- ---------- Données de démonstration (mêmes agents que la page Emploi) ----------
-- La zone est retrouvée par son CODE (ZN-xx, voir zones_oracle.sql).
INSERT INTO AGENT (NOM, PRENOM, ROLE, STATUT, ID_ZONE, QUART_TRAVAIL)
VALUES ('Ben Salah', 'Ahmed',   'Technicien Météo', 'En service', (SELECT ID_ZONE FROM ZONE WHERE CODE = 'ZN-01'), 'Matin (06h-14h)');
INSERT INTO AGENT (NOM, PRENOM, ROLE, STATUT, ID_ZONE, QUART_TRAVAIL)
VALUES ('Trabelsi',  'Sana',    'Météorologue',     'Disponible', (SELECT ID_ZONE FROM ZONE WHERE CODE = 'ZN-05'), 'Après-midi (14h-22h)');
INSERT INTO AGENT (NOM, PRENOM, ROLE, STATUT, ID_ZONE, QUART_TRAVAIL)
VALUES ('Jlassi',    'Karim',   'Agent de terrain', 'En service', (SELECT ID_ZONE FROM ZONE WHERE CODE = 'ZN-01'), 'Matin (06h-14h)');
INSERT INTO AGENT (NOM, PRENOM, ROLE, STATUT, ID_ZONE, QUART_TRAVAIL)
VALUES ('Gharbi',    'Meriam',  'Superviseur',      'Disponible', (SELECT ID_ZONE FROM ZONE WHERE CODE = 'ZN-04'), 'Matin (06h-14h)');
INSERT INTO AGENT (NOM, PRENOM, ROLE, STATUT, ID_ZONE, QUART_TRAVAIL)
VALUES ('Kouki',     'Yasmine', 'Météorologue',     'En service', (SELECT ID_ZONE FROM ZONE WHERE CODE = 'ZN-03'), 'Après-midi (14h-22h)');
INSERT INTO AGENT (NOM, PRENOM, ROLE, STATUT, ID_ZONE, QUART_TRAVAIL)
VALUES ('Nouioui',   'Adem',    'Technicien Météo', 'En congé',   (SELECT ID_ZONE FROM ZONE WHERE CODE = 'ZN-01'), 'Nuit (22h-06h)');
INSERT INTO AGENT (NOM, PRENOM, ROLE, STATUT, ID_ZONE, QUART_TRAVAIL)
VALUES ('Ghandour',  'Aweb',    'Agent de terrain', 'En service', (SELECT ID_ZONE FROM ZONE WHERE CODE = 'ZN-06'), 'Matin (06h-14h)');
INSERT INTO AGENT (NOM, PRENOM, ROLE, STATUT, ID_ZONE, QUART_TRAVAIL)
VALUES ('Eddine',    'Bader',   'Superviseur',      'Formation',  (SELECT ID_ZONE FROM ZONE WHERE CODE = 'ZN-02'), 'Après-midi (14h-22h)');

COMMIT;
