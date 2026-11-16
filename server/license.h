#pragma once

#include <QString>
#include <QDateTime>
#include <QJsonObject>

// одна видана ліцензія
class License {
private:
    QString key;
    QString owner;
    QString note;
    bool blocked = false;
    QString hwid; // порожній - ще не активована
    QDateTime createdAt;
    QDateTime activatedAt;
    QDateTime lastSeen;
public:
    License() = default;
    License(QString key, QString owner, QString note);

    QString getKey() const { return key; }
    QString getOwner() const { return owner; }
    QString getNote() const { return note; }
    QString getHwid() const { return hwid; }
    QDateTime getCreatedAt() const { return createdAt; }
    QDateTime getActivatedAt() const { return activatedAt; }
    QDateTime getLastSeen() const { return lastSeen; }
    bool isBlocked() const { return blocked; }
    bool isActivated() const { return !hwid.isEmpty(); }
    QString status() const;

    void setBlocked(bool value) { blocked = value; }
    void activate(QString newHwid);
    void resetHwid();
    void touch() { lastSeen = QDateTime::currentDateTime(); }

    QJsonObject toJson() const;
    static License fromJson(QString key, QJsonObject obj);
};
