#include "database.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QVariantList>

namespace {

bool run(const QString &sql, QString *err)
{
    return true;
}

void ins(const QString &sql, const QVariantList &values)
{
}

QString iso(const QDateTime &d) { return d.toString(Qt::ISODate); }

void seed()
{
}

} // namespace

bool Database::open(const QString &path, QString *error)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    return true;
}

void Database::close()
{
}
