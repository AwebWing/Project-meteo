-- ============================================================
--  Smart Weather - Module "Gestion des Interventions"
--  Oracle 11g XE - à exécuter avec l'utilisateur METEO
--  (SQL Developer : ouvrir ce fichier, puis "Exécuter le script" = F5)
--
--  Tables      : INTERVENTION, INTERVENTION_EQUIPEMENT
--  Prérequis   : zones_oracle.sql, equipement_oracle.sql, personnel_oracle.sql
--                (clés étrangères vers ZONE, EQUIPEMENT et AGENT)
--  Re-exécutable : OUI - supprime puis recrée ses objets.
--  Encodage    : fichier en UTF-8.
--
--  Types, priorités et statuts = ceux de InterventionService / intervention.h.
--  NB : le module C++ travaille pour l'instant en mémoire (données de
--  démonstration) ; ce schéma est prêt pour le brancher sur Oracle.
-- ============================================================
SET DEFINE OFF

-- ---------- Nettoyage (aucune erreur si les objets n'existent pas) ----------
BEGIN
    FOR t IN (SELECT table_name FROM user_tables
              WHERE table_name IN ('INTERVENTION_EQUIPEMENT', 'INTERVENTION')) LOOP
        EXECUTE IMMEDIATE 'DROP TABLE ' || t.table_name || ' CASCADE CONSTRAINTS PURGE';
    END LOOP;
    FOR s IN (SELECT sequence_name FROM user_sequences
              WHERE sequence_name IN ('SEQ_INTERVENTION')) LOOP
        EXECUTE IMMEDIATE 'DROP SEQUENCE ' || s.sequence_name;
    END LOOP;
END;
/

-- ---------- Séquences (auto-incrément sous Oracle 11g) ----------
CREATE SEQUENCE SEQ_INTERVENTION            START WITH 1 INCREMENT BY 1 NOCACHE;

-- ---------- Table INTERVENTION ----------
CREATE TABLE INTERVENTION (
    ID_INTERVENTION NUMBER(10)     NOT NULL,
    TYPE            VARCHAR2(50)   NOT NULL,
    PRIORITE        VARCHAR2(10)   DEFAULT 'Moyenne' NOT NULL,
    STATUT          VARCHAR2(20)   DEFAULT UNISTR('Planifi\00e9e') NOT NULL,
    DATE_DEBUT      DATE           NOT NULL,
    DATE_FIN        DATE,
    DESCRIPTION     VARCHAR2(500),
    RESPONSABLE     VARCHAR2(100),
    COMPTE_RENDU    VARCHAR2(2000),
    ID_ZONE         NUMBER(10)     NOT NULL,
    ID_AGENT        NUMBER(10),
    CONSTRAINT PK_INTERVENTION  PRIMARY KEY (ID_INTERVENTION),
    CONSTRAINT CK_INTERV_TYPE   CHECK (TYPE IN ('Inondation', 'Chaleur', 'Vent fort', 'Maintenance station', 'Inspection', 'Autre')),
    CONSTRAINT CK_INTERV_PRIO   CHECK (PRIORITE IN ('Faible', 'Moyenne', 'Haute', 'Critique')),
    CONSTRAINT CK_INTERV_STATUT CHECK (STATUT IN (UNISTR('Planifi\00e9e'), 'En cours', UNISTR('Termin\00e9e'), UNISTR('Annul\00e9e'))),
    CONSTRAINT CK_INTERV_DATES  CHECK (DATE_FIN IS NULL OR DATE_FIN >= DATE_DEBUT),
    -- Règle métier (InterventionService::controleCloture) : pas de clôture sans compte-rendu d'au moins 20 caractères.
    CONSTRAINT CK_INTERV_CLOT   CHECK (STATUT <> UNISTR('Termin\00e9e') OR NVL(LENGTH(TRIM(COMPTE_RENDU)), 0) >= 20),
    -- Une zone référencée ne peut pas être supprimée (ORA-02292, géré par l'application).
    CONSTRAINT FK_INTERV_ZONE   FOREIGN KEY (ID_ZONE)  REFERENCES ZONE (ID_ZONE),
    CONSTRAINT FK_INTERV_AGENT  FOREIGN KEY (ID_AGENT) REFERENCES AGENT (ID_AGENT) ON DELETE SET NULL
);

-- Oracle n'indexe pas les clés étrangères : ces index accélèrent les jointures
-- et évitent de parcourir toute la table lors de la suppression d'une zone / d'un agent.
CREATE INDEX IDX_INTERV_ZONE  ON INTERVENTION (ID_ZONE);
CREATE INDEX IDX_INTERV_AGENT ON INTERVENTION (ID_AGENT);

-- ---------- Table INTERVENTION_EQUIPEMENT (équipements mobilisés) ----------
CREATE TABLE INTERVENTION_EQUIPEMENT (
    ID_INTERVENTION NUMBER(10) NOT NULL,
    ID_EQUIPEMENT   NUMBER(10) NOT NULL,
    CONSTRAINT PK_INTERVENTION_EQUIPEMENT PRIMARY KEY (ID_INTERVENTION, ID_EQUIPEMENT),
    -- Supprimer une intervention libère ses équipements...
    CONSTRAINT FK_IE_INTERVENTION FOREIGN KEY (ID_INTERVENTION)
        REFERENCES INTERVENTION (ID_INTERVENTION) ON DELETE CASCADE,
    -- ... mais un équipement déjà engagé dans une intervention ne peut pas être
    -- supprimé (ORA-02292, géré par equipment.cpp) : on garde la traçabilité.
    CONSTRAINT FK_IE_EQUIPEMENT FOREIGN KEY (ID_EQUIPEMENT)
        REFERENCES EQUIPEMENT (ID_EQUIPEMENT)
);

-- La clé primaire couvre déjà ID_INTERVENTION ; il manque l'autre sens.
CREATE INDEX IDX_IE_EQUIPEMENT ON INTERVENTION_EQUIPEMENT (ID_EQUIPEMENT);

-- ---------- Triggers : clé primaire automatique si non fournie ----------
CREATE OR REPLACE TRIGGER TRG_INTERVENTION_ID
BEFORE INSERT ON INTERVENTION
FOR EACH ROW
WHEN (NEW.ID_INTERVENTION IS NULL)
BEGIN
    :NEW.ID_INTERVENTION := SEQ_INTERVENTION.NEXTVAL;
END;
/

-- ---------- Données de démonstration (dates relatives à aujourd'hui) ----------
-- Zones, agents et équipements sont retrouvés par leur CODE / NOM (jamais par un id codé en dur).
INSERT INTO INTERVENTION (TYPE, PRIORITE, STATUT, DATE_DEBUT, DATE_FIN, DESCRIPTION, RESPONSABLE, ID_ZONE, ID_AGENT)
VALUES ('Inondation', 'Haute', 'En cours', TRUNC(SYSDATE) - 2 + 8/24, TRUNC(SYSDATE) + 1 + 18/24,
        UNISTR('Pompage et contr\00f4le des capteurs apr\00e8s les fortes pluies'), 'Ahmed Ben Salah',
        (SELECT ID_ZONE FROM ZONE WHERE CODE = 'ZN-01'),
        (SELECT ID_AGENT FROM AGENT WHERE NOM = 'Ben Salah' AND PRENOM = 'Ahmed'));

INSERT INTO INTERVENTION (TYPE, PRIORITE, STATUT, DATE_DEBUT, DATE_FIN, DESCRIPTION, RESPONSABLE, ID_ZONE, ID_AGENT)
VALUES ('Vent fort', 'Critique', UNISTR('Planifi\00e9e'), TRUNC(SYSDATE) + 1 + 8/24, TRUNC(SYSDATE) + 2 + 18/24,
        UNISTR('S\00e9curisation de la station avant l''alerte vent fort'), 'Meriam Gharbi',
        (SELECT ID_ZONE FROM ZONE WHERE CODE = 'ZN-02'),
        (SELECT ID_AGENT FROM AGENT WHERE NOM = 'Gharbi' AND PRENOM = 'Meriam'));

INSERT INTO INTERVENTION (TYPE, PRIORITE, STATUT, DATE_DEBUT, DATE_FIN, DESCRIPTION, RESPONSABLE, COMPTE_RENDU, ID_ZONE, ID_AGENT)
VALUES ('Maintenance station', 'Faible', UNISTR('Termin\00e9e'), TRUNC(SYSDATE) - 10 + 8/24, TRUNC(SYSDATE) - 10 + 16/24,
        UNISTR('V\00e9rification annuelle des capteurs'), 'Karim Jlassi',
        UNISTR('Capteurs v\00e9rifi\00e9s et recalibr\00e9s, batterie de la station remplac\00e9e, aucune anomalie restante.'),
        (SELECT ID_ZONE FROM ZONE WHERE CODE = 'ZN-03'),
        (SELECT ID_AGENT FROM AGENT WHERE NOM = 'Jlassi' AND PRENOM = 'Karim'));

INSERT INTO INTERVENTION (TYPE, PRIORITE, STATUT, DATE_DEBUT, DATE_FIN, DESCRIPTION, RESPONSABLE, ID_ZONE, ID_AGENT)
VALUES ('Chaleur', 'Haute', 'En cours', TRUNC(SYSDATE) - 4 + 8/24, TRUNC(SYSDATE) - 1 + 18/24,
        UNISTR('Suivi des capteurs de temp\00e9rature pendant la vague de chaleur (en retard)'), 'Yasmine Kouki',
        (SELECT ID_ZONE FROM ZONE WHERE CODE = 'ZN-03'),
        (SELECT ID_AGENT FROM AGENT WHERE NOM = 'Kouki' AND PRENOM = 'Yasmine'));

INSERT INTO INTERVENTION (TYPE, PRIORITE, STATUT, DATE_DEBUT, DESCRIPTION, RESPONSABLE, ID_ZONE, ID_AGENT)
VALUES ('Inspection', 'Moyenne', UNISTR('Planifi\00e9e'), TRUNC(SYSDATE) + 5 + 9/24,
        UNISTR('Inspection du nouveau pluviom\00e8tre'), 'Aweb Ghandour',
        (SELECT ID_ZONE FROM ZONE WHERE CODE = 'ZN-06'),
        (SELECT ID_AGENT FROM AGENT WHERE NOM = 'Ghandour' AND PRENOM = 'Aweb'));

-- Équipements mobilisés
INSERT INTO INTERVENTION_EQUIPEMENT (ID_INTERVENTION, ID_EQUIPEMENT)
SELECT i.ID_INTERVENTION, e.ID_EQUIPEMENT FROM INTERVENTION i, EQUIPEMENT e
WHERE i.DESCRIPTION LIKE 'Pompage%' AND e.CODE IN ('EQ-007', 'EQ-012');
INSERT INTO INTERVENTION_EQUIPEMENT (ID_INTERVENTION, ID_EQUIPEMENT)
SELECT i.ID_INTERVENTION, e.ID_EQUIPEMENT FROM INTERVENTION i, EQUIPEMENT e
WHERE i.DESCRIPTION LIKE UNISTR('V\00e9rification annuelle%') AND e.CODE = 'EQ-001';
INSERT INTO INTERVENTION_EQUIPEMENT (ID_INTERVENTION, ID_EQUIPEMENT)
SELECT i.ID_INTERVENTION, e.ID_EQUIPEMENT FROM INTERVENTION i, EQUIPEMENT e
WHERE i.DESCRIPTION LIKE 'Inspection du nouveau%' AND e.CODE = 'EQ-006';

COMMIT;
