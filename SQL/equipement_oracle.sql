-- ============================================================
--  Smart Weather - Module "Gestion des Équipements" (Adem Nouioui)
--  Oracle 11g XE - à exécuter avec l'utilisateur METEO
--  (SQL Developer : ouvrir ce fichier, puis "Exécuter le script" = F5)
--
--  Tables      : EQUIPEMENT, EQUIPEMENT_HISTORIQUE
--  Prérequis   : FOURNISSEUR (fournisseur_oracle.sql) - un équipement a
--                exactement un fournisseur (modèle physique : Fournit 1..1 / 1..N)
--  Re-exécutable : OUI - supprime puis recrée ses objets (supprimer
--                EQUIPEMENT seule retire aussi la clé étrangère de
--                INTERVENTION_EQUIPEMENT : préférer install_all.sql).
--  Encodage    : fichier en UTF-8.
--
--  Valeurs autorisées = celles du code C++ (Headers/equipment.h) :
--  si vous ajoutez une catégorie ou un statut là-bas, ajoutez-la aussi
--  dans les CHECK ci-dessous.
-- ============================================================
SET DEFINE OFF

-- ---------- Nettoyage (aucune erreur si les objets n'existent pas) ----------
BEGIN
    FOR t IN (SELECT table_name FROM user_tables
              WHERE table_name IN ('EQUIPEMENT_HISTORIQUE', 'EQUIPEMENT')) LOOP
        EXECUTE IMMEDIATE 'DROP TABLE ' || t.table_name || ' CASCADE CONSTRAINTS PURGE';
    END LOOP;
    FOR s IN (SELECT sequence_name FROM user_sequences
              WHERE sequence_name IN ('SEQ_EQUIPEMENT', 'SEQ_EQUIPEMENT_HISTORIQUE')) LOOP
        EXECUTE IMMEDIATE 'DROP SEQUENCE ' || s.sequence_name;
    END LOOP;
END;
/

-- ---------- Séquences (auto-incrément sous Oracle 11g) ----------
CREATE SEQUENCE SEQ_EQUIPEMENT            START WITH 1 INCREMENT BY 1 NOCACHE;
CREATE SEQUENCE SEQ_EQUIPEMENT_HISTORIQUE START WITH 1 INCREMENT BY 1 NOCACHE;

-- ---------- Table EQUIPEMENT ----------
CREATE TABLE EQUIPEMENT (
    ID_EQUIPEMENT  NUMBER(10)    NOT NULL,
    NOM            VARCHAR2(100) NOT NULL,
    CODE           VARCHAR2(20),
    CATEGORIE      VARCHAR2(50)  NOT NULL,
    STATUT         VARCHAR2(30)  DEFAULT 'Disponible' NOT NULL,
    LOCALISATION   VARCHAR2(100) NOT NULL,
    DERNIERE_MAINT DATE,
    DESCRIPTION    VARCHAR2(500),
    ID_FOURNISSEUR NUMBER(10)    NOT NULL,
    CONSTRAINT PK_EQUIPEMENT        PRIMARY KEY (ID_EQUIPEMENT),
    -- Pas de cascade : un fournisseur qui fournit encore des équipements ne peut pas être supprimé (ORA-02292).
    CONSTRAINT FK_EQUIPEMENT_FOURN  FOREIGN KEY (ID_FOURNISSEUR) REFERENCES FOURNISSEUR (ID_FOURNISSEUR),
    CONSTRAINT CK_EQUIPEMENT_CAT    CHECK (CATEGORIE IN ('Capteurs', 'Véhicules', 'Matériel de terrain', 'Équipements de sécurité')),
    CONSTRAINT CK_EQUIPEMENT_STATUT CHECK (STATUT IN ('Disponible', 'En maintenance', 'Hors service'))
);

-- Code facultatif mais unique (sans tenir compte de la casse, comme
-- EquipmentService::valider). Les codes NULL ne sont pas indexés : plusieurs
-- équipements sans code sont donc permis.
CREATE UNIQUE INDEX UQ_EQUIPEMENT_CODE ON EQUIPEMENT (UPPER(CODE));

-- Index de la clé étrangère (Oracle ne le crée pas automatiquement).
CREATE INDEX IDX_EQUIP_FOURN ON EQUIPEMENT (ID_FOURNISSEUR);

-- ---------- Table EQUIPEMENT_HISTORIQUE (métier 5 : traçabilité) ----------
CREATE TABLE EQUIPEMENT_HISTORIQUE (
    ID_HIST          NUMBER(10)   NOT NULL,
    ID_EQUIPEMENT    NUMBER(10)   NOT NULL,
    DATE_CHANGEMENT  DATE         DEFAULT SYSDATE NOT NULL,
    CHAMP            VARCHAR2(50) NOT NULL,
    ANCIENNE_VALEUR  VARCHAR2(200),
    NOUVELLE_VALEUR  VARCHAR2(200),
    CONSTRAINT PK_EQUIPEMENT_HISTORIQUE PRIMARY KEY (ID_HIST),
    CONSTRAINT FK_HIST_EQUIPEMENT FOREIGN KEY (ID_EQUIPEMENT)
        REFERENCES EQUIPEMENT (ID_EQUIPEMENT) ON DELETE CASCADE
);

-- Sert aussi d'index à la clé étrangère (ID_EQUIPEMENT en tête).
CREATE INDEX IDX_HIST_EQUIP ON EQUIPEMENT_HISTORIQUE (ID_EQUIPEMENT, DATE_CHANGEMENT);

-- ---------- Triggers : clé primaire automatique si non fournie ----------
CREATE OR REPLACE TRIGGER TRG_EQUIPEMENT_ID
BEFORE INSERT ON EQUIPEMENT
FOR EACH ROW
WHEN (NEW.ID_EQUIPEMENT IS NULL)
BEGIN
    :NEW.ID_EQUIPEMENT := SEQ_EQUIPEMENT.NEXTVAL;
END;
/

CREATE OR REPLACE TRIGGER TRG_EQUIPEMENT_HIST_ID
BEFORE INSERT ON EQUIPEMENT_HISTORIQUE
FOR EACH ROW
WHEN (NEW.ID_HIST IS NULL)
BEGIN
    :NEW.ID_HIST := SEQ_EQUIPEMENT_HISTORIQUE.NEXTVAL;
END;
/

-- ---------- Données de démonstration ----------
-- Dates de maintenance RELATIVES à aujourd'hui (l'application signale une
-- alerte au-delà de 180 jours : EQ-013 et EQ-021 sont volontairement en retard).
-- Colonnes : NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, ID_FOURNISSEUR
-- (le fournisseur est retrouvé par son nom : les identifiants viennent de la séquence)
INSERT INTO EQUIPEMENT (NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, ID_FOURNISSEUR) VALUES ('Thermomètre connecté',     'EQ-001', 'Capteurs',                'Disponible',     'ST-04',   TRUNC(SYSDATE) - 2, (SELECT ID_FOURNISSEUR FROM FOURNISSEUR WHERE NOM = 'Meteo Sensors TN'));
INSERT INTO EQUIPEMENT (NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, ID_FOURNISSEUR) VALUES ('Station météo',            'EQ-002', 'Capteurs',                'Disponible',     'ST-07',   TRUNC(SYSDATE) - 4, (SELECT ID_FOURNISSEUR FROM FOURNISSEUR WHERE NOM = 'Meteo Sensors TN'));
INSERT INTO EQUIPEMENT (NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, ID_FOURNISSEUR) VALUES ('Capteur UV',               'EQ-003', 'Capteurs',                'Disponible',     'ST-01',   TRUNC(SYSDATE) - 20, (SELECT ID_FOURNISSEUR FROM FOURNISSEUR WHERE NOM = 'Meteo Sensors TN'));
INSERT INTO EQUIPEMENT (NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, ID_FOURNISSEUR) VALUES ('Baromètre digital',        'EQ-004', 'Capteurs',                'Hors service',   'ST-03',   TRUNC(SYSDATE) - 58, (SELECT ID_FOURNISSEUR FROM FOURNISSEUR WHERE NOM = 'Meteo Sensors TN'));
INSERT INTO EQUIPEMENT (NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, ID_FOURNISSEUR) VALUES ('Anémomètre',               'EQ-005', 'Capteurs',                'Disponible',     'ST-09',   TRUNC(SYSDATE) - 1, (SELECT ID_FOURNISSEUR FROM FOURNISSEUR WHERE NOM = 'Meteo Sensors TN'));
INSERT INTO EQUIPEMENT (NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, ID_FOURNISSEUR) VALUES ('Pluviomètre',              'EQ-006', 'Capteurs',                'Disponible',     'ST-05',   TRUNC(SYSDATE) - 9, (SELECT ID_FOURNISSEUR FROM FOURNISSEUR WHERE NOM = 'Meteo Sensors TN'));
INSERT INTO EQUIPEMENT (NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, ID_FOURNISSEUR) VALUES ('Véhicule Alpha'   ,            'EQ-007', 'Véhicules',               'Disponible',     'Tunis',   TRUNC(SYSDATE) - 12, (SELECT ID_FOURNISSEUR FROM FOURNISSEUR WHERE NOM = 'AutoPro Tunisie'));
INSERT INTO EQUIPEMENT (NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, ID_FOURNISSEUR) VALUES ('Véhicule Bravo'   ,            'EQ-008', 'Véhicules',               'Disponible',     'Sfax',    TRUNC(SYSDATE) - 7, (SELECT ID_FOURNISSEUR FROM FOURNISSEUR WHERE NOM = 'AutoPro Tunisie'));
INSERT INTO EQUIPEMENT (NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, ID_FOURNISSEUR) VALUES ('Véhicule Charlie' ,            'EQ-009', 'Véhicules',               'En maintenance', 'Sousse',  TRUNC(SYSDATE) - 11, (SELECT ID_FOURNISSEUR FROM FOURNISSEUR WHERE NOM = 'AutoPro Tunisie'));
INSERT INTO EQUIPEMENT (NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, ID_FOURNISSEUR) VALUES ('Moto tout-terrain',        'EQ-010', 'Véhicules',               'Disponible',     'Nabeul',  TRUNC(SYSDATE) - 24, (SELECT ID_FOURNISSEUR FROM FOURNISSEUR WHERE NOM = 'AutoPro Tunisie'));
INSERT INTO EQUIPEMENT (NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, ID_FOURNISSEUR) VALUES ('Camionnette terrain',      'EQ-011', 'Véhicules',               'Disponible',     'Bizerte', TRUNC(SYSDATE) - 5, (SELECT ID_FOURNISSEUR FROM FOURNISSEUR WHERE NOM = 'AutoPro Tunisie'));
INSERT INTO EQUIPEMENT (NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, ID_FOURNISSEUR) VALUES ('Drone de surveillance',    'EQ-012', 'Véhicules',               'Disponible',     'Ariana',  TRUNC(SYSDATE) - 3, (SELECT ID_FOURNISSEUR FROM FOURNISSEUR WHERE NOM = 'AutoPro Tunisie'));
INSERT INTO EQUIPEMENT (NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, ID_FOURNISSEUR) VALUES ('GPS portable',            'EQ-013', 'Matériel de terrain',     'Disponible',     'ST-12',   TRUNC(SYSDATE) - 190, (SELECT ID_FOURNISSEUR FROM FOURNISSEUR WHERE NOM = 'Réseau Plus'));
INSERT INTO EQUIPEMENT (NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, ID_FOURNISSEUR) VALUES ('Tente de terrain',        'EQ-014', 'Matériel de terrain',     'En maintenance', 'Manouba', TRUNC(SYSDATE) - 7, (SELECT ID_FOURNISSEUR FROM FOURNISSEUR WHERE NOM = 'Réseau Plus'));
INSERT INTO EQUIPEMENT (NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, ID_FOURNISSEUR) VALUES ('Lampe UV terrain',        'EQ-015', 'Matériel de terrain',     'Disponible',     'ST-02',   TRUNC(SYSDATE) - 14, (SELECT ID_FOURNISSEUR FROM FOURNISSEUR WHERE NOM = 'Réseau Plus'));
INSERT INTO EQUIPEMENT (NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, ID_FOURNISSEUR) VALUES ('Trousse premiers secours', 'EQ-016', 'Matériel de terrain',     'Disponible',     'ST-06',   TRUNC(SYSDATE) - 1, (SELECT ID_FOURNISSEUR FROM FOURNISSEUR WHERE NOM = 'Réseau Plus'));
INSERT INTO EQUIPEMENT (NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, ID_FOURNISSEUR) VALUES ('Radio longue portée',      'EQ-017', 'Matériel de terrain',     'Disponible',     'ST-08',   TRUNC(SYSDATE) - 6, (SELECT ID_FOURNISSEUR FROM FOURNISSEUR WHERE NOM = 'Réseau Plus'));
INSERT INTO EQUIPEMENT (NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, ID_FOURNISSEUR) VALUES ('Gilet de sécurité',        'EQ-018', 'Équipements de sécurité', 'Disponible',     'Dépôt-1', TRUNC(SYSDATE) - 10, (SELECT ID_FOURNISSEUR FROM FOURNISSEUR WHERE NOM = 'Sécurité Pro'));
INSERT INTO EQUIPEMENT (NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, ID_FOURNISSEUR) VALUES ('Casque de protection',     'EQ-019', 'Équipements de sécurité', 'Disponible',     'Dépôt-2', TRUNC(SYSDATE) - 8, (SELECT ID_FOURNISSEUR FROM FOURNISSEUR WHERE NOM = 'Sécurité Pro'));
INSERT INTO EQUIPEMENT (NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, ID_FOURNISSEUR) VALUES ('Harnais de sécurité',      'EQ-020', 'Équipements de sécurité', 'Disponible',     'Dépôt-1', TRUNC(SYSDATE) - 11, (SELECT ID_FOURNISSEUR FROM FOURNISSEUR WHERE NOM = 'Sécurité Pro'));
INSERT INTO EQUIPEMENT (NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, ID_FOURNISSEUR) VALUES ('Extincteur portable',      'EQ-021', 'Équipements de sécurité', 'Disponible',     'Dépôt-3', TRUNC(SYSDATE) - 210, (SELECT ID_FOURNISSEUR FROM FOURNISSEUR WHERE NOM = 'Sécurité Pro'));

-- Historique : une ligne "Création" par équipement (comme le fait l'application)...
INSERT INTO EQUIPEMENT_HISTORIQUE (ID_EQUIPEMENT, DATE_CHANGEMENT, CHAMP, NOUVELLE_VALEUR)
SELECT ID_EQUIPEMENT, TRUNC(SYSDATE) - 90 + 9/24, 'Création', NOM FROM EQUIPEMENT;

-- ... puis quelques changements de statut.
INSERT INTO EQUIPEMENT_HISTORIQUE (ID_EQUIPEMENT, DATE_CHANGEMENT, CHAMP, ANCIENNE_VALEUR, NOUVELLE_VALEUR)
SELECT ID_EQUIPEMENT, TRUNC(SYSDATE) - 40 + 9/24, 'Statut', 'En maintenance', 'Disponible' FROM EQUIPEMENT WHERE CODE = 'EQ-003';
INSERT INTO EQUIPEMENT_HISTORIQUE (ID_EQUIPEMENT, DATE_CHANGEMENT, CHAMP, ANCIENNE_VALEUR, NOUVELLE_VALEUR)
SELECT ID_EQUIPEMENT, TRUNC(SYSDATE) - 30 + 9/24, 'Statut', 'Disponible', 'En maintenance' FROM EQUIPEMENT WHERE CODE = 'EQ-009';
INSERT INTO EQUIPEMENT_HISTORIQUE (ID_EQUIPEMENT, DATE_CHANGEMENT, CHAMP, ANCIENNE_VALEUR, NOUVELLE_VALEUR)
SELECT ID_EQUIPEMENT, TRUNC(SYSDATE) - 25 + 9/24, 'Statut', 'Disponible', 'Hors service' FROM EQUIPEMENT WHERE CODE = 'EQ-004';
INSERT INTO EQUIPEMENT_HISTORIQUE (ID_EQUIPEMENT, DATE_CHANGEMENT, CHAMP, ANCIENNE_VALEUR, NOUVELLE_VALEUR)
SELECT ID_EQUIPEMENT, TRUNC(SYSDATE) - 20 + 9/24, 'Statut', 'Disponible', 'En maintenance' FROM EQUIPEMENT WHERE CODE = 'EQ-014';

COMMIT;
