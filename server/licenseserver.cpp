#include "licenseserver.h"
#include <QJsonDocument>
#include <QMessageAuthenticationCode>

// той самий секрет вшитий у клієнт, тож його можна витягнути з exe
static const QByteArray SECRET = "lab28-license-secret";

QByteArray LicenseServer::sign(const QJsonObject& obj){
    QByteArray data = QJsonDocument(obj).toJson(QJsonDocument::Compact); // ключі в QJsonObject завжди відсортовані
    return QMessageAuthenticationCode::hash(data, SECRET, QCryptographicHash::Sha256).toHex();
}

LicenseServer::LicenseServer(LicenseStore& store, QObject* parent) : QTcpServer(parent), store(store) {
    connect(this, &QTcpServer::newConnection, this, [this]{
        while (QTcpSocket* socket = nextPendingConnection()) {
            connect(socket, &QTcpSocket::readyRead, this, [this, socket]{ handle(socket); });
            connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
        }
    });
}

void LicenseServer::handle(QTcpSocket* socket){
    // запит може прийти кількома шматками - копимо, поки не буде все тіло
    QByteArray buf = socket->property("buf").toByteArray() + socket->readAll();
    if (buf.size() > 64 * 1024) { socket->abort(); return; }
    int headerEnd = buf.indexOf("\r\n\r\n");
    QByteArray head = buf.left(headerEnd).toLower();
    int lenPos = head.indexOf("content-length:");
    int len = lenPos < 0 ? 0 : head.mid(lenPos + 15, head.indexOf("\r\n", lenPos) - lenPos - 15).trimmed().toInt();
    if (headerEnd < 0 || buf.size() < headerEnd + 4 + len) {
        socket->setProperty("buf", buf);
        return;
    }

    QJsonObject reply;
    QByteArray code = "200 OK";
    if (!head.startsWith("post /api/verify ")) {
        reply = {{"status", "error"}, {"message", "Невідомий запит."}};
        code = "404 Not Found";
    } else {
        QJsonObject req = QJsonDocument::fromJson(buf.mid(headerEnd + 4, len)).object();
        reply = store.verify(req["license_key"].toString().trimmed().toUpper(), req["hardware_id"].toString());
        reply["nonce"] = req["nonce"].toString(); // щоб стару відповідь не можна було підсунути вдруге
        reply["sig"] = QString(sign(reply));
        emit changed();
    }

    QByteArray body = QJsonDocument(reply).toJson(QJsonDocument::Compact);
    socket->write("HTTP/1.1 " + code + "\r\nContent-Type: application/json\r\nContent-Length: "
                  + QByteArray::number(body.size()) + "\r\nConnection: close\r\n\r\n" + body);
    socket->disconnectFromHost();
}
