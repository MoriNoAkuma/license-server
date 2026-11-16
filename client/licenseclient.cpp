#include "licenseclient.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QJsonDocument>
#include <QMessageAuthenticationCode>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSysInfo>
#include <QUuid>

// має збігатися з секретом сервера
static const QByteArray SECRET = "lab28-license-secret";

LicenseClient::LicenseClient(QString key, QString server, QString hwid)
    : key(key), server(server), hwid(hwid) {}

// відбиток ПК: id системи, якщо Windows його не дав - ім'я ПК
QString LicenseClient::hardwareId(){
    QByteArray id = QSysInfo::machineUniqueId();
    if (id.isEmpty()) id = QSysInfo::machineHostName().toUtf8();
    return QCryptographicHash::hash(id, QCryptographicHash::Sha256).toHex().left(32);
}

QByteArray LicenseClient::sign(const QJsonObject& obj){
    QByteArray data = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    return QMessageAuthenticationCode::hash(data, SECRET, QCryptographicHash::Sha256).toHex();
}

QJsonObject LicenseClient::send(QJsonObject body){
    QNetworkAccessManager net;
    QNetworkRequest req(QUrl(server + "/api/verify"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setTransferTimeout(5000);
    QNetworkReply* reply = net.post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));

    // чекаємо відповідь, не блокуючи вікна
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    QByteArray data = reply->readAll();
    bool failed = reply->error() != QNetworkReply::NoError && data.isEmpty();
    if (failed) message = "Сервер ліцензій недоступний (" + reply->errorString() + ").";
    reply->deleteLater();
    return failed ? QJsonObject() : QJsonDocument::fromJson(data).object();
}

bool LicenseClient::check(){
    message = "Сервер ліцензій повернув незрозумілу відповідь.";
    QString nonce = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QJsonObject reply = send({{"license_key", key}, {"hardware_id", hwid}, {"nonce", nonce}});
    if (reply.isEmpty()) return false;

    // відповідь "valid" може підробити будь-хто, тому віримо тільки підпису
    QString sig = reply.take("sig").toString();
    if (sig.toLatin1() != sign(reply) || reply["nonce"].toString() != nonce) {
        message = "Відповідь сервера не пройшла перевірку підпису.";
        return false;
    }
    message = reply["message"].toString();
    return reply["status"].toString() == "valid" && reply["hwid"].toString() == hwid;
}

bool requireLicense(){
    QString path = QCoreApplication::applicationDirPath() + "/license.json";
    QFile file(path);
    if (!file.exists() && file.open(QIODevice::WriteOnly)) { // шаблон, куди покупець впише ключ
        file.write(QJsonDocument(QJsonObject{{"key", ""}, {"server", "http://127.0.0.1:8099"}}).toJson());
        file.close();
    }
    QJsonObject cfg;
    if (file.open(QIODevice::ReadOnly)) cfg = QJsonDocument::fromJson(file.readAll()).object();

    QString key = cfg["key"].toString().trimmed();
    if (key.isEmpty()) {
        QMessageBox::critical(nullptr, "Ліцензія", "Впишіть ліцензійний ключ у файл\n" + QDir::toNativeSeparators(path));
        return false;
    }

    LicenseClient client(key, cfg["server"].toString("http://127.0.0.1:8099"));
    if (!client.check()) {
        QMessageBox::critical(nullptr, "Ліцензія", client.getMessage());
        return false;
    }
    return true;
}
