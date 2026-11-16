#pragma once

#include "licensestore.h"
#include <QTcpServer>
#include <QTcpSocket>

// приймає POST /api/verify від клієнтів і відповідає підписаним JSON
class LicenseServer : public QTcpServer {
    Q_OBJECT
private:
    LicenseStore& store;
    void handle(QTcpSocket* socket);
public:
    LicenseServer(LicenseStore& store, QObject* parent = nullptr);
    static QByteArray sign(const QJsonObject& obj);
signals:
    void changed(); // клієнт активувався або зайшов - оновити таблицю
};
