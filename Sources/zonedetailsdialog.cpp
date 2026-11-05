#include "zonedetailsdialog.h"
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include "theme.h"
#include "uihelpers.h"
#include "zoneservice.h"

ZoneDetailsDialog::ZoneDetailsDialog(const Zone &z,
                                     int rang, int nbZones, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Détails de la zone — %1").arg(z.nom));
    resize(560, 480);

    const QString niv = ZoneService::niveau(z.vulnerabilite);
    const QString nivHtml = QStringLiteral("<b style='color:%1'>%2</b>").arg(Theme::levelColor(niv).name(), niv);
    const bool sous = ZoneService::sousCouverture(z);
    const QString couvHtml = sous
        ? QStringLiteral("<b style='color:%1'>Insuffisante</b>").arg(Theme::levelColor("Insuffisante").name())
        : QStringLiteral("<b style='color:%1'>Suffisante</b>").arg(Theme::levelColor("OK").name());

    auto *title = new QLabel(z.nom);
    title->setObjectName("title");
    auto *sub = new QLabel(QStringLiteral("Code %1  •  %2").arg(z.code, z.statut));
    sub->setObjectName("muted");

    auto row = [](const QString &k, const QString &v) {
        return QStringLiteral("<tr><td style='color:#8fb3c2; padding-right:18px; padding-bottom:6px'>%1</td>"
                              "<td style='padding-bottom:6px'>%2</td></tr>").arg(k, v);
    };
    QString html = QStringLiteral("<table>");
    html += row("Vulnérabilité", QStringLiteral("%1 / 100  (%2)").arg(z.vulnerabilite).arg(nivHtml));
    html += row("Coordonnées", QStringLiteral("%1 ; %2").arg(z.latitude, 0, 'f', 5).arg(z.longitude, 0, 'f', 5));
    html += row("Stations", QStringLiteral("%1 présente(s) / %2 attendue(s) — %3")
                                .arg(z.nbStations).arg(ZoneService::stationsAttendues(z)).arg(couvHtml));
    html += row("Interventions actives", QString::number(z.interventionsActives));
    html += row("Score de priorité", QStringLiteral("%1  (rang %2 sur %3)")
                                         .arg(ZoneService::scorePriorite(z)).arg(rang).arg(nbZones));
    const QStringList anomalies = ZoneService::controleGeographique(z);
    html += row("Cohérence géographique", anomalies.isEmpty()
                    ? QStringLiteral("<span style='color:#3fc98f'>Aucune anomalie</span>")
                    : QStringLiteral("<span style='color:#e49740'>%1</span>").arg(anomalies.join("<br>")));
    html += QStringLiteral("</table>");

    auto *info = new QLabel(html);
    info->setTextFormat(Qt::RichText);
    info->setWordWrap(true);

    auto *close = new QPushButton(QStringLiteral("Fermer"));
    connect(close, &QPushButton::clicked, this, &QDialog::accept);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(22, 20, 22, 20);
    root->setSpacing(12);
    root->addWidget(title);
    root->addWidget(sub);
    root->addWidget(info);
    root->addStretch(1);
    root->addWidget(close, 0, Qt::AlignRight);
}
