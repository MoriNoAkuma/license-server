#pragma once

#include "license.h"
#include <QMap>

// всі ліцензії, зберігаються в licenses.json
class LicenseStore {
private:
    QMap<QString, License> licenses; // ключ -> ліцензія
    QString path;
public:
    explicit LicenseStore(QString path);

    QList<License> all() const { return licenses.values(); }
    bool contains(QString key) const { return licenses.contains(key); }
    License get(QString key) const { return licenses.value(key); }

    QString create(QString owner, QString note); // повертає новий ключ
    bool setBlocked(QString key, bool blocked);
    bool resetHwid(QString key);
    bool remove(QString key);

    // відповідь на запит клієнта (без підпису)
    QJsonObject verify(QString key, QString hwid);

    bool load();
    bool save() const;
    static QString generateKey();
};
