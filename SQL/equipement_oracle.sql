-- ============================================================
--  Smart Weather - Module "Gestion des Équipements" (Adem Nouioui)
--  Oracle 11g XE - à exécuter avec l'utilisateur METEO
--  (SQL Developer : ouvrir ce fichier, puis "Exécuter le script" = F5)
--
--  Tables      : EQUIPEMENT
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
              WHERE table_name IN ('EQUIPEMENT')) LOOP
        EXECUTE IMMEDIATE 'DROP TABLE ' || t.table_name || ' CASCADE CONSTRAINTS PURGE';
    END LOOP;
    FOR s IN (SELECT sequence_name FROM user_sequences
              WHERE sequence_name IN ('SEQ_EQUIPEMENT')) LOOP
        EXECUTE IMMEDIATE 'DROP SEQUENCE ' || s.sequence_name;
    END LOOP;
END;
/

-- ---------- Séquences (auto-incrément sous Oracle 11g) ----------
CREATE SEQUENCE SEQ_EQUIPEMENT            START WITH 1 INCREMENT BY 1 NOCACHE;

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
    CONSTRAINT CK_EQUIPEMENT_CAT    CHECK (CATEGORIE IN ('Capteurs', UNISTR('V\00e9hicules'), UNISTR('Mat\00e9riel de terrain'), UNISTR('\00c9quipements de s\00e9curit\00e9'))),
    CONSTRAINT CK_EQUIPEMENT_STATUT CHECK (STATUT IN ('Disponible', 'En maintenance', 'Hors service'))
);

-- Code facultatif mais unique (sans tenir compte de la casse, comme
-- EquipmentService::valider). Les codes NULL ne sont pas indexés : plusieurs
-- équipements sans code sont donc permis.
CREATE UNIQUE INDEX UQ_EQUIPEMENT_CODE ON EQUIPEMENT (UPPER(CODE));

-- Index de la clé étrangère (Oracle ne le crée pas automatiquement).
CREATE INDEX IDX_EQUIP_FOURN ON EQUIPEMENT (ID_FOURNISSEUR);

-- ---------- Triggers : clé primaire automatique si non fournie ----------
CREATE OR REPLACE TRIGGER TRG_EQUIPEMENT_ID
BEFORE INSERT ON EQUIPEMENT
FOR EACH ROW
WHEN (NEW.ID_EQUIPEMENT IS NULL)
BEGIN
    :NEW.ID_EQUIPEMENT := SEQ_EQUIPEMENT.NEXTVAL;
END;
/

-- ---------- Données de démonstration ----------
-- Dates de maintenance RELATIVES à aujourd'hui (l'application signale une
-- alerte au-delà de 180 jours : EQ-013 et EQ-021 sont volontairement en retard).
-- Colonnes : NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, ID_FOURNISSEUR
-- (le fournisseur est retrouvé par son nom : les identifiants viennent de la séquence)
INSERT INTO EQUIPEMENT (NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, ID_FOURNISSEUR) VALUES (UNISTR('Thermom\00e8tre connect\00e9'),     'EQ-001', 'Capteurs',                'Disponible',     UNISTR('Entrep\00f4t Tunis'),   TRUNC(SYSDATE) - 2, (SELECT ID_FOURNISSEUR FROM FOURNISSEUR WHERE NOM = 'Meteo Sensors TN'));
INSERT INTO EQUIPEMENT (NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, ID_FOURNISSEUR) VALUES (UNISTR('Station m\00e9t\00e9o'),            'EQ-002', 'Capteurs',                'Disponible',     UNISTR('Entrep\00f4t Tunis'),   TRUNC(SYSDATE) - 4, (SELECT ID_FOURNISSEUR FROM FOURNISSEUR WHERE NOM = 'Meteo Sensors TN'));
INSERT INTO EQUIPEMENT (NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, ID_FOURNISSEUR) VALUES ('Capteur UV',               'EQ-003', 'Capteurs',                'Disponible',     UNISTR('Entrep\00f4t Tunis'),   TRUNC(SYSDATE) - 20, (SELECT ID_FOURNISSEUR FROM FOURNISSEUR WHERE NOM = 'Meteo Sensors TN'));
INSERT INTO EQUIPEMENT (NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, ID_FOURNISSEUR) VALUES (UNISTR('Barom\00e8tre digital'),        'EQ-004', 'Capteurs',                'Hors service',   'Atelier',   TRUNC(SYSDATE) - 58, (SELECT ID_FOURNISSEUR FROM FOURNISSEUR WHERE NOM = 'Meteo Sensors TN'));
INSERT INTO EQUIPEMENT (NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, ID_FOURNISSEUR) VALUES (UNISTR('An\00e9mom\00e8tre'),               'EQ-005', 'Capteurs',                'Disponible',     'Atelier',   TRUNC(SYSDATE) - 1, (SELECT ID_FOURNISSEUR FROM FOURNISSEUR WHERE NOM = 'Meteo Sensors TN'));
INSERT INTO EQUIPEMENT (NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, ID_FOURNISSEUR) VALUES (UNISTR('Pluviom\00e8tre'),              'EQ-006', 'Capteurs',                'Disponible',     'Atelier',   TRUNC(SYSDATE) - 9, (SELECT ID_FOURNISSEUR FROM FOURNISSEUR WHERE NOM = 'Meteo Sensors TN'));
INSERT INTO EQUIPEMENT (NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, ID_FOURNISSEUR) VALUES (UNISTR('V\00e9hicule Alpha')   ,            'EQ-007', UNISTR('V\00e9hicules'),               'Disponible',     'Garage',   TRUNC(SYSDATE) - 12, (SELECT ID_FOURNISSEUR FROM FOURNISSEUR WHERE NOM = 'AutoPro Tunisie'));
INSERT INTO EQUIPEMENT (NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, ID_FOURNISSEUR) VALUES (UNISTR('V\00e9hicule Bravo')   ,            'EQ-008', UNISTR('V\00e9hicules'),               'Disponible',     'Garage',    TRUNC(SYSDATE) - 7, (SELECT ID_FOURNISSEUR FROM FOURNISSEUR WHERE NOM = 'AutoPro Tunisie'));
INSERT INTO EQUIPEMENT (NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, ID_FOURNISSEUR) VALUES (UNISTR('V\00e9hicule Charlie') ,            'EQ-009', UNISTR('V\00e9hicules'),               'En maintenance', 'Garage',  TRUNC(SYSDATE) - 11, (SELECT ID_FOURNISSEUR FROM FOURNISSEUR WHERE NOM = 'AutoPro Tunisie'));
INSERT INTO EQUIPEMENT (NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, ID_FOURNISSEUR) VALUES ('Moto tout-terrain',        'EQ-010', UNISTR('V\00e9hicules'),               'Disponible',     UNISTR('Entrep\00f4t Sfax'),  TRUNC(SYSDATE) - 24, (SELECT ID_FOURNISSEUR FROM FOURNISSEUR WHERE NOM = 'AutoPro Tunisie'));
INSERT INTO EQUIPEMENT (NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, ID_FOURNISSEUR) VALUES ('Camionnette terrain',      'EQ-011', UNISTR('V\00e9hicules'),               'Disponible',     UNISTR('Entrep\00f4t Sfax'), TRUNC(SYSDATE) - 5, (SELECT ID_FOURNISSEUR FROM FOURNISSEUR WHERE NOM = 'AutoPro Tunisie'));
INSERT INTO EQUIPEMENT (NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, ID_FOURNISSEUR) VALUES ('Drone de surveillance',    'EQ-012', UNISTR('V\00e9hicules'),               'Disponible',     UNISTR('Entrep\00f4t Sfax'),  TRUNC(SYSDATE) - 3, (SELECT ID_FOURNISSEUR FROM FOURNISSEUR WHERE NOM = 'AutoPro Tunisie'));
INSERT INTO EQUIPEMENT (NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, ID_FOURNISSEUR) VALUES ('GPS portable',            'EQ-013', UNISTR('Mat\00e9riel de terrain'),     'Disponible',     UNISTR('Entrep\00f4t Tunis'),   TRUNC(SYSDATE) - 190, (SELECT ID_FOURNISSEUR FROM FOURNISSEUR WHERE NOM = UNISTR('R\00e9seau Plus')));
INSERT INTO EQUIPEMENT (NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, ID_FOURNISSEUR) VALUES ('Tente de terrain',        'EQ-014', UNISTR('Mat\00e9riel de terrain'),     'En maintenance', UNISTR('Entrep\00f4t Tunis'), TRUNC(SYSDATE) - 7, (SELECT ID_FOURNISSEUR FROM FOURNISSEUR WHERE NOM = UNISTR('R\00e9seau Plus')));
INSERT INTO EQUIPEMENT (NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, ID_FOURNISSEUR) VALUES ('Lampe UV terrain',        'EQ-015', UNISTR('Mat\00e9riel de terrain'),     'Disponible',     UNISTR('Entrep\00f4t Tunis'),   TRUNC(SYSDATE) - 14, (SELECT ID_FOURNISSEUR FROM FOURNISSEUR WHERE NOM = UNISTR('R\00e9seau Plus')));
INSERT INTO EQUIPEMENT (NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, ID_FOURNISSEUR) VALUES ('Trousse premiers secours', 'EQ-016', UNISTR('Mat\00e9riel de terrain'),     'Disponible',     UNISTR('Entrep\00f4t Sfax'),   TRUNC(SYSDATE) - 1, (SELECT ID_FOURNISSEUR FROM FOURNISSEUR WHERE NOM = UNISTR('R\00e9seau Plus')));
INSERT INTO EQUIPEMENT (NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, ID_FOURNISSEUR) VALUES (UNISTR('Radio longue port\00e9e'),      'EQ-017', UNISTR('Mat\00e9riel de terrain'),     'Disponible',     UNISTR('Entrep\00f4t Sfax'),   TRUNC(SYSDATE) - 6, (SELECT ID_FOURNISSEUR FROM FOURNISSEUR WHERE NOM = UNISTR('R\00e9seau Plus')));
INSERT INTO EQUIPEMENT (NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, ID_FOURNISSEUR) VALUES (UNISTR('Gilet de s\00e9curit\00e9'),        'EQ-018', UNISTR('\00c9quipements de s\00e9curit\00e9'), 'Disponible',     UNISTR('Armoire s\00e9curit\00e9'), TRUNC(SYSDATE) - 10, (SELECT ID_FOURNISSEUR FROM FOURNISSEUR WHERE NOM = UNISTR('S\00e9curit\00e9 Pro')));
INSERT INTO EQUIPEMENT (NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, ID_FOURNISSEUR) VALUES ('Casque de protection',     'EQ-019', UNISTR('\00c9quipements de s\00e9curit\00e9'), 'Disponible',     UNISTR('Armoire s\00e9curit\00e9'), TRUNC(SYSDATE) - 8, (SELECT ID_FOURNISSEUR FROM FOURNISSEUR WHERE NOM = UNISTR('S\00e9curit\00e9 Pro')));
INSERT INTO EQUIPEMENT (NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, ID_FOURNISSEUR) VALUES (UNISTR('Harnais de s\00e9curit\00e9'),      'EQ-020', UNISTR('\00c9quipements de s\00e9curit\00e9'), 'Disponible',     UNISTR('Armoire s\00e9curit\00e9'), TRUNC(SYSDATE) - 11, (SELECT ID_FOURNISSEUR FROM FOURNISSEUR WHERE NOM = UNISTR('S\00e9curit\00e9 Pro')));
INSERT INTO EQUIPEMENT (NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, ID_FOURNISSEUR) VALUES ('Extincteur portable',      'EQ-021', UNISTR('\00c9quipements de s\00e9curit\00e9'), 'Disponible',     UNISTR('Armoire s\00e9curit\00e9'), TRUNC(SYSDATE) - 210, (SELECT ID_FOURNISSEUR FROM FOURNISSEUR WHERE NOM = UNISTR('S\00e9curit\00e9 Pro')));

COMMIT;
