# Smart Weather — Gestion des Zones (Adem Nouioui)

Projet Qt 6 (Widgets + Charts) — C++17.

## Lancer
1. Qt Creator -> File -> Open File or Project -> choisir `SmartWeather.pro`
2. Kit : Qt 6.7.3 MinGW 64-bit -> Configure Project
3. Ctrl+R

(Il faut avoir installé le module **Qt Charts** avec Qt.)

## Structure (src/)
| Fichier | Role |
|---|---|
| main.cpp | demarre l'application + applique le theme |
| theme.h | couleurs + feuille de style QSS (tout le look est ici) |
| zone.h | structure `Zone` (= table ZONE du MLD) + `ZoneHistory` |
| zoneservice.* | les 5 metiers avances (sans interface => testables) |
| zonerepository.* | acces aux donnees (en memoire pour l'instant -> MySQL plus tard) |
| mainwindow.* | fenetre principale : barre laterale, bandeau meteo, pages |
| zonesmodule.* | module "Stations & Zones" avec onglets |
| zonespage.* | interface Zones : KPI, recherche, filtres, tableau, carte, detail, export |
| zonedialog.* | formulaire Ajouter / Modifier (validations + calculateur de score) |
| zonedetailsdialog.* | interface "Details zone" + historique |
| coveragepage.* | interface "Analyse couverture" + graphiques |
| mapwidget.* | carte dessinee a la main |

## Base de donnees (Oracle)
1. Source ODBC 64 bits `smartweatherDSN` (utilisateur `meteo`) -- voir l'atelier Connexion Oracle/Qt.
2. Executer `SQL/zones_oracle.sql` dans SQL Developer (tables ZONE et ZONE_HISTORIQUE + donnees de demo).
3. Lancer l'application : `main.cpp` ouvre la connexion (Singleton `Connection`) avant la fenetre principale.

Architecture modele-vue : toutes les requetes (preparees) de l'entite Zone sont dans `Sources/zone.cpp`
(`ajouter`, `modifier`, `supprimer`, `afficher`). `ZoneRepository` n'a aucune requete : il appelle `Zone`,
garde la liste en memoire et emet `changed()` pour rafraichir les pages.
Modeles (MCD / MLD / MPD) : `docs/MODELE_DONNEES.md`.
