#pragma once
#include <QGraphicsView>
#include <QList>
#include <QSet>
#include <QStringList>
#include "equipment.h"

class QGraphicsScene;
class QGraphicsRectItem;
class QTimer;
class QWheelEvent;

// ============================================================
//  StoragePlanWidget : plan du dépôt (vue "jumeau numérique").
//
//  - Chaque ÉTAGÈRE = une valeur distincte de EQUIPEMENT.LOCALISATION
//    (aucune table ni colonne en plus dans la base).
//  - Chaque équipement = une tuile (icône de sa catégorie) dont la
//    bordure a la couleur de son statut.
//  - On glisse une tuile sur une autre étagère : le widget émet
//    equipmentMoved(id, nouvelleLocalisation) ; la page met à jour
//    Oracle via le repository (Equipment::modifier).
//  - Aucune requête SQL ici : c'est une vue pure.
// ============================================================
class StoragePlanWidget : public QGraphicsView
{
    Q_OBJECT
public:
    explicit StoragePlanWidget(QWidget *parent = nullptr);

    void setEquipments(const QList<Equipment> &list);
    bool addRack(const QString &nom);              // étagère vide (jusqu'au 1er dépôt)
    bool rackExists(const QString &nom) const;
    void setSelected(int idEquipment);

    // Appelés par les tuiles (usage interne)
    void tileDragging(const QPointF &scenePos);
    void tileDropped(int idEquipment, const QPointF &scenePos);
    void tileClicked(int idEquipment);

signals:
    void equipmentMoved(int idEquipment, const QString &nouvelleLocalisation);
    void equipmentSelected(int idEquipment);

protected:
    void resizeEvent(QResizeEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;

private:
    struct Rack { QString nom; QRectF rect; QGraphicsRectItem *item = nullptr; };

    void rebuild();
    int  colonnes() const;
    int  rackIndexAt(const QPointF &scenePos) const;
    void highlightRack(int index);

    QGraphicsScene *m_scene;
    QList<Equipment> m_equipments;
    QStringList      m_extraRacks;     // étagères créées mais encore vides
    QList<Rack>      m_racks;
    QSet<int>        m_enRetard;       // ids dont la maintenance dépasse 180 jours
    QList<QGraphicsItem *> m_alertBadges;
    QTimer          *m_pulse;
    int              m_selected = -1;
    int              m_colonnes = 0;
    bool             m_pulseOn = false;
};
