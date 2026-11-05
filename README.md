# Smart Weather — Application intégrée (Qt 6 / C++17 / Oracle)

Projet Qt 6 (Widgets + Charts + SQL), compilé avec qmake : `SmartWeather.pro`.

## Lancer
1. Base de données : voir la section suivante (une seule fois).
2. Qt Creator -> File -> Open File or Project -> choisir `SmartWeather.pro`
3. Kit : Qt 6.7.3 MinGW 64-bit -> Configure Project
4. Ctrl+R

(Il faut avoir installé les modules **Qt Charts** et **Qt SQL** (pilote ODBC) avec Qt.)

## Base de données (Oracle 11g XE)
1. Créer une source ODBC **64 bits** nommée `smartweatherDSN` (identifiants dans `Sources/connection.cpp` : `meteo` / `meteo5`).
2. *(Seulement si l'utilisateur n'existe pas)* ouvrir `SQL/00_create_user.sql` avec **SYSTEM** et l'exécuter (F5).
3. Se connecter avec **METEO**, ouvrir `SQL/install_all.sql` et l'exécuter (F5) :
   il crée toutes les tables dans le bon ordre, avec un jeu de données de démonstration.
4. Lancer l'application : `main.cpp` ouvre la connexion (Singleton `Connection`) avant la fenêtre principale.

| Script | Tables | Dépend de |
|---|---|---|
| `zones_oracle.sql` | ZONE, ZONE_HISTORIQUE | — |
| `equipement_oracle.sql` | EQUIPEMENT, EQUIPEMENT_HISTORIQUE | — |
| `personnel_oracle.sql` | AGENT, AGENT_HISTORIQUE | ZONE |
| `intervention_oracle.sql` | INTERVENTION, INTERVENTION_HISTORIQUE, INTERVENTION_EQUIPEMENT | ZONE, EQUIPEMENT, AGENT |
| `fournisseur_oracle.sql` | FOURNISSEUR, FOURNISSEUR_HISTORIQUE | — |

- Chaque script est **re-exécutable** (il supprime puis recrée ses objets) : ne pas le relancer sur une base dont les
  données comptent. Pour tout réinitialiser de façon cohérente, utiliser `install_all.sql`.
- Ouvrir les scripts en **UTF-8** (SQL Developer > Outils > Préférences > Environnement > Encodage), sinon les valeurs
  accentuées (`Planifiée`, `Archivé`...) ne correspondront plus aux contraintes `CHECK`.
- Modèles (MCD / MLD / MPD) : `docs/MODELE_DONNEES.md`.

### État de la persistance
| Module | Page | Données |
|---|---|---|
| Zones | Stations & Zones | **Oracle** (`zone.cpp`) |
| Équipements | Équipements | **Oracle** (`equipment.cpp`) |
| Fournisseurs | Fournisseurs | **Oracle** (`fournisseur.cpp`) |
| Interventions | Interventions | en mémoire (`interventionservice.cpp`) — schéma SQL prêt |
| Personnel | Gestion d'emploi | en mémoire (`emploipage.cpp`) — schéma SQL prêt |

La couverture réseau (nombre de stations par zone) vaudra 0 % tant que la table STATION n'existe pas.

## Structure
| Dossier | Contenu |
|---|---|
| `Sources/`, `Headers/` | code C++ (un `.cpp` par classe ; `theme.h`, `uihelpers.h`, `dbhelpers.h` sont des en-têtes seuls) |
| `Forms/` | fichiers Qt Designer (`mainwindow.ui`, `interventiondialog.ui`) |
| `resources/` | logo (`resources.qrc`) |
| `SQL/` | scripts Oracle |
| `docs/` | modèles de données |

### Architecture modèle-vue
Les requêtes SQL (toutes **préparées** : `prepare` + `bindValue`) sont uniquement dans les entités :
`Sources/zone.cpp`, `equipment.cpp`, `fournisseur.cpp` (`ajouter`, `modifier`, `supprimer`, `afficher`, `historique`).
Chaque `*Repository` n'a aucune requête : il appelle l'entité, garde la liste en mémoire et émet `changed()` pour
rafraîchir les pages. Les services (`zoneservice`, `equipmentservice`, `interventionservice`) contiennent les règles
métier sans accès à la base ni à l'interface. `Headers/dbhelpers.h` fournit la transaction RAII partagée.

| Fichier | Rôle |
|---|---|
| main.cpp | connexion Oracle + thème + fenêtre principale |
| mainwindow.* | barre latérale, bandeau météo, navigation entre les pages |
| theme.h / uihelpers.h | couleurs + feuille de style QSS, petits widgets réutilisables |
| accueilpage.* | page d'accueil |
| zonesmodule.*, zonespage.*, zonedialog.*, zonedetailsdialog.*, coveragepage.*, mapwidget.* | module Stations & Zones |
| equipmentpage.*, equipmentdialog.* | module Équipements |
| smartsupply.*, fournisseurrepository.* | module Fournisseurs (page `SmartSupply`) |
| interventionwidget.*, interventiondialog.*, barchartwidget.* | module Interventions |
| emploipage.* | module Personnel / planning |
