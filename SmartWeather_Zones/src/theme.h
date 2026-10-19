#pragma once
#include <QColor>
#include <QString>

// ============================================================
//  THEME : tous les styles de l'application sont ici.
//  Les figures du cahier utilisent un theme sombre (bleu nuit + turquoise).
//  Pour passer a la charte du chapitre 7.2 (#162B3E, #347EA7 ...),
//  il suffit de modifier les couleurs dans la feuille de style ci-dessous.
// ============================================================
namespace Theme {

// Couleurs par niveau de vulnerabilite / statut
inline QColor levelColor(const QString &niveau)
{
    if (niveau == QStringLiteral("Élevé") || niveau == QStringLiteral("Insuffisante"))
        return QColor(0xE0, 0x5A, 0x5B);              // rouge
    if (niveau == QStringLiteral("Moyen"))
        return QColor(0xE4, 0x97, 0x40);              // orange
    return QColor(0x3F, 0xC9, 0x8F);                  // vert (Faible / OK)
}

inline QColor textColor()  { return QColor(0xE6, 0xF1, 0xF5); }
inline QColor mutedColor() { return QColor(0x8F, 0xB3, 0xC2); }
inline QColor accentColor(){ return QColor(0x19, 0xC6, 0xB7); }

inline QString styleSheet()
{
    return QStringLiteral(R"(
QWidget { color: #e6f1f5; font-family: 'Segoe UI'; font-size: 13px; }
QMainWindow, QWidget#root { background: #07151f; }

/* ---------- Barre laterale ---------- */
QFrame#sidebar { background: #081b28; border-right: 1px solid #123244; }
QPushButton#nav {
    text-align: left; padding: 11px 16px; border: none; border-radius: 10px;
    color: #b7d0da; background: transparent; font-size: 14px;
}
QPushButton#nav:hover   { background: #0f2f42; }
QPushButton#nav:checked { background: #0f5563; color: white; font-weight: 600; }

/* ---------- Cartes ---------- */
QFrame#header, QFrame#card, QFrame#kpi {
    background: #0c2333; border: 1px solid #17475a; border-radius: 12px;
}
QLabel#kpiIcon {
    background: #0f3a4a; border: 1px solid #1c6a7e; border-radius: 22px; font-size: 20px;
}
QLabel#kpiValue { font-size: 26px; font-weight: 700; color: #ffffff; }
QLabel#title    { font-size: 20px; font-weight: 700; color: #ffffff; }
QLabel#cardTitle{ font-size: 15px; font-weight: 600; color: #ffffff; }
QLabel#muted    { color: #8fb3c2; }

/* ---------- Boutons ---------- */
QPushButton {
    background: #0f2f42; border: 1px solid #1c5a6e; border-radius: 8px; padding: 8px 14px;
}
QPushButton:hover    { background: #133b52; }
QPushButton:disabled { color: #56727e; border-color: #15384a; }
QPushButton#primary  { background: #19c6b7; color: #04202a; border: none; font-weight: 700; padding: 9px 18px; }
QPushButton#primary:hover { background: #3ddccd; }
QPushButton#danger   { border: 1px solid #cc4e4f; color: #ff8a8b; }
QPushButton#danger:hover { background: #3a1d26; }
QPushButton#tab { border: none; background: transparent; color: #b7d0da; padding: 8px 16px; border-radius: 8px; }
QPushButton#tab:checked { background: #0f5563; color: white; font-weight: 600; }

/* ---------- Champs ---------- */
QLineEdit, QComboBox, QDoubleSpinBox, QSpinBox {
    background: #0a1c2a; border: 1px solid #1c5a6e; border-radius: 8px; padding: 7px 10px;
    selection-background-color: #19c6b7;
}
QLineEdit:focus, QComboBox:focus, QDoubleSpinBox:focus, QSpinBox:focus { border: 1px solid #19c6b7; }
QComboBox::drop-down { border: none; width: 22px; }
QComboBox QAbstractItemView {
    background: #0a1c2a; border: 1px solid #1c5a6e; selection-background-color: #12586a; outline: 0;
}
QGroupBox { border: 1px solid #17475a; border-radius: 10px; margin-top: 14px; padding: 12px; }
QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; color: #19c6b7; }

/* ---------- Tableaux ---------- */
QTableView, QTableWidget {
    background: transparent; alternate-background-color: #0e2839; border: none;
    gridline-color: transparent; selection-background-color: #12586a; selection-color: white;
}
QTableView::item { padding: 6px; border: none; }
QHeaderView::section {
    background: transparent; color: #19c6b7; border: none;
    border-bottom: 1px solid #17475a; padding: 8px; font-weight: 600;
}
QTableCornerButton::section { background: transparent; border: none; }

/* ---------- Dialogues ---------- */
QDialog, QMessageBox { background: #0a1c2a; }
QScrollBar:vertical { background: transparent; width: 10px; }
QScrollBar::handle:vertical { background: #1c5a6e; border-radius: 5px; min-height: 30px; }
QScrollBar::add-line, QScrollBar::sub-line { height: 0; width: 0; }
QToolTip { background: #0c2333; color: white; border: 1px solid #19c6b7; }
)");
}

} // namespace Theme
