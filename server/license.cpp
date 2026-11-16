#include "license.h"

License::License(QString key, QString owner, QString note)
    : key(key), owner(owner), note(note), createdAt(QDateTime::currentDateTime()) {}

QString License::status() const {
    if (blocked) return "Заблокована";
    return isActivated() ? "Активна" : "Не активована";
}

void License::activate(QString newHwid){
    hwid = newHwid;
    activatedAt = QDateTime::currentDateTime();
}

// покупець легально змінив ПК
void License::resetHwid(){
    hwid.clear();
    activatedAt = QDateTime();
}

QJsonObject License::toJson() const {
    return {
        {"owner", owner},
        {"note", note},
        {"blocked", blocked},
        {"hardware_id", hwid},
        {"created_at", createdAt.toString(Qt::ISODate)},
        {"activated_at", activatedAt.toString(Qt::ISODate)},
        {"last_seen", lastSeen.toString(Qt::ISODate)},
    };
}

License License::fromJson(QString key, QJsonObject obj){
    License lic(key, obj["owner"].toString(), obj["note"].toString());
    lic.blocked = obj["blocked"].toBool();
    lic.hwid = obj["hardware_id"].toString();
    lic.createdAt = QDateTime::fromString(obj["created_at"].toString(), Qt::ISODate);
    lic.activatedAt = QDateTime::fromString(obj["activated_at"].toString(), Qt::ISODate);
    lic.lastSeen = QDateTime::fromString(obj["last_seen"].toString(), Qt::ISODate);
    return lic;
}
