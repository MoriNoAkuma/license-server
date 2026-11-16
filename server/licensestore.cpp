#include "licensestore.h"
#include <QFile>
#include <QJsonDocument>
#include <QRandomGenerator>

LicenseStore::LicenseStore(QString path) : path(path) {}

// формат LIS-XXXX-XXXX-XXXX
QString LicenseStore::generateKey(){
    QString key = "LIS";
    for (int i = 0; i < 3; i++)
        key += "-" + QString::number(QRandomGenerator::system()->bounded(0x10000), 16).rightJustified(4, '0').toUpper();
    return key;
}

QString LicenseStore::create(QString owner, QString note){
    QString key;
    do key = generateKey(); while (licenses.contains(key));
    licenses[key] = License(key, owner, note);
    save();
    return key;
}

bool LicenseStore::setBlocked(QString key, bool blocked){
    if (!licenses.contains(key)) return false;
    licenses[key].setBlocked(blocked);
    return save();
}

bool LicenseStore::resetHwid(QString key){
    if (!licenses.contains(key)) return false;
    licenses[key].resetHwid();
    return save();
}

bool LicenseStore::remove(QString key){
    if (!licenses.remove(key)) return false;
    return save();
}

QJsonObject LicenseStore::verify(QString key, QString hwid){
    if (key.isEmpty() || hwid.isEmpty())
        return {{"status", "error"}, {"message", "Некоректний запит."}};
    if (!licenses.contains(key))
        return {{"status", "invalid"}, {"message", "Ліцензійний ключ не знайдено."}};

    License& lic = licenses[key];
    if (lic.isBlocked())
        return {{"status", "blocked"}, {"message", "Ліцензія заблокована. Зверніться до продавця."}};

    QString message = "OK";
    if (!lic.isActivated()) { // перший запуск - прив'язуємо до ПК
        lic.activate(hwid);
        message = "Ліцензію активовано.";
    }
    else if (lic.getHwid() != hwid)
        return {{"status", "hwid_mismatch"}, {"message", "Ключ уже активовано на іншому комп'ютері."}};

    lic.touch();
    save();
    return {{"status", "valid"}, {"message", message}, {"hwid", hwid}};
}

bool LicenseStore::save() const {
    QJsonObject root;
    for (const License& lic : licenses)
        root[lic.getKey()] = lic.toJson();
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return false;
    file.write(QJsonDocument(root).toJson());
    return true;
}

bool LicenseStore::load(){
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return false;
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError) return false; // пошкоджений файл не чіпаємо
    QJsonObject root = doc.object();
    licenses.clear();
    for (QString key : root.keys())
        licenses[key] = License::fromJson(key, root[key].toObject());
    return true;
}
