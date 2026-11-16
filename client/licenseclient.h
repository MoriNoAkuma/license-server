#pragma once

#include <QString>
#include <QJsonObject>

// перевірка ліцензії; щоб захистити програму - додати файли в .pro і викликати requireLicense() в main
class LicenseClient {
private:
    QString key;
    QString server;
    QString hwid;
    QString message;
    QJsonObject send(QJsonObject body);
public:
    LicenseClient(QString key, QString server, QString hwid = hardwareId());
    bool check(); // true - сервер підтвердив ліцензію
    QString getMessage() const { return message; }
    static QString hardwareId();
    static QByteArray sign(const QJsonObject& obj);
};

// читає ключ з license.json біля exe, при помилці показує причину; false - програму треба закрити
bool requireLicense();
