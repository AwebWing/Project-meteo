-- ============================================================
--  Smart Weather - Installation complète de la base (Oracle 11g XE)
--
--  À exécuter avec l'utilisateur METEO (SQL Developer : ouvrir ce fichier,
--  puis "Exécuter le script" = F5). Si l'utilisateur n'existe pas encore,
--  lancer d'abord 00_create_user.sql avec SYSTEM.
--
--  Les modules sont créés dans l'ordre des dépendances (clés étrangères) :
--
--      FOURNISSEUR ── EQUIPEMENT ──┐
--      ZONE ── AGENT ──────────────┴── INTERVENTION ── INTERVENTION_EQUIPEMENT
--
--  ATTENTION : chaque script SUPPRIME puis recrée ses tables ; toutes les
--  données existantes de METEO sont remplacées par le jeu de démonstration.
--  (@@ = chemin relatif à ce fichier : garder les scripts dans le même dossier.)
-- ============================================================
SET DEFINE OFF
SET ECHO OFF

PROMPT >>> 1/5  Zones
@@zones_oracle.sql

PROMPT >>> 2/5  Fournisseurs (avant les équipements : EQUIPEMENT référence FOURNISSEUR)
@@fournisseur_oracle.sql

PROMPT >>> 3/5  Équipements
@@equipement_oracle.sql

PROMPT >>> 4/5  Personnel
@@personnel_oracle.sql

PROMPT >>> 5/5  Interventions
@@intervention_oracle.sql

PROMPT >>> Terminé. Vérification :
SELECT 'ZONE' AS TABLE_NAME, COUNT(*) AS NB_LIGNES FROM ZONE
UNION ALL SELECT 'EQUIPEMENT',             COUNT(*) FROM EQUIPEMENT
UNION ALL SELECT 'AGENT',                  COUNT(*) FROM AGENT
UNION ALL SELECT 'INTERVENTION',           COUNT(*) FROM INTERVENTION
UNION ALL SELECT 'INTERVENTION_EQUIPEMENT',COUNT(*) FROM INTERVENTION_EQUIPEMENT
UNION ALL SELECT 'FOURNISSEUR',            COUNT(*) FROM FOURNISSEUR;

-- Objets invalides éventuels (doit renvoyer 0 ligne) :
SELECT OBJECT_NAME, OBJECT_TYPE FROM USER_OBJECTS WHERE STATUS <> 'VALID';
