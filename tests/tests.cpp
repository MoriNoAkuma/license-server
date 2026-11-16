#include "../server/licenseserver.h"
#include "../client/licenseclient.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <cassert>
#include <iostream>

// тести без вікна: сховище, підпис, сервер і клієнт по справжньому TCP
int main(int argc, char* argv[]){
    QApplication app(argc, argv);

    QString path = QDir::temp().filePath("licenses_test.json");
    QFile::remove(path);

    // ключ і операції адміна
    assert(QRegularExpression("^LIS(-[0-9A-F]{4}){3}$").match(LicenseStore::generateKey()).hasMatch());
    LicenseStore store(path);
    QString key = store.create("Іван", "тест");
    QString key2 = store.create("Петро", "");
    assert(store.all().size() == 2);
    assert(store.get(key).status() == "Не активована");
    assert(!store.setBlocked("LIS-0000-0000-0000", true));
    assert(store.remove(key2) && !store.contains(key2));

    // перевірка ключа і прив'язка до ПК
    assert(store.verify("LIS-0000-0000-0000", "pc1")["status"] == "invalid");
    assert(store.verify(key, "")["status"] == "error");
    assert(store.verify(key, "pc1")["status"] == "valid");
    assert(store.get(key).getHwid() == "pc1");
    assert(store.verify(key, "pc1")["status"] == "valid");
    assert(store.verify(key, "pc2")["status"] == "hwid_mismatch");
    store.setBlocked(key, true);
    assert(store.verify(key, "pc1")["status"] == "blocked");
    store.setBlocked(key, false);
    store.resetHwid(key);
    assert(store.verify(key, "pc2")["status"] == "valid");

    // файл зберігає все
    LicenseStore loaded(path);
    assert(loaded.load());
    assert(loaded.all().size() == 1);
    assert(loaded.get(key).getOwner() == "Іван" && loaded.get(key).getHwid() == "pc2");
    assert(!LicenseStore(QDir::temp().filePath("no_such_licenses.json")).load());

    // пошкоджений файл не завантажується
    QString broken = QDir::temp().filePath("licenses_broken.json");
    QFile bf(broken);
    assert(bf.open(QIODevice::WriteOnly));
    bf.write("{\"LIS-1111");
    bf.close();
    assert(!LicenseStore(broken).load());

    // підпис однаковий у сервера і клієнта, зміна даних його ламає
    QJsonObject obj{{"status", "valid"}, {"hwid", "pc1"}};
    assert(LicenseServer::sign(obj) == LicenseClient::sign(obj));
    QJsonObject fake = obj;
    fake["hwid"] = "pc2";
    assert(LicenseServer::sign(fake) != LicenseServer::sign(obj));

    // справжній сервер на вільному порту
    LicenseServer server(loaded);
    assert(server.listen(QHostAddress::LocalHost, 0));
    QString url = "http://127.0.0.1:" + QString::number(server.serverPort());

    LicenseClient ok(key, url, "pc2");
    assert(ok.check());
    LicenseClient otherPc(key, url, "pc3");
    assert(!otherPc.check() && otherPc.getMessage().contains("іншому"));
    LicenseClient wrongKey("LIS-0000-0000-0000", url, "pc2");
    assert(!wrongKey.check() && wrongKey.getMessage().contains("не знайдено"));
    loaded.setBlocked(key, true);
    assert(!ok.check() && ok.getMessage().contains("заблокована"));
    loaded.setBlocked(key, false);

    // підроблений сервер відповідає "valid" без підпису
    QTcpServer evil;
    assert(evil.listen(QHostAddress::LocalHost, 0));
    QObject::connect(&evil, &QTcpServer::newConnection, [&]{
        QTcpSocket* s = evil.nextPendingConnection();
        QObject::connect(s, &QTcpSocket::readyRead, [s]{
            QByteArray body = R"({"status":"valid","message":"OK","hwid":"pc2"})";
            s->write("HTTP/1.1 200 OK\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body);
            s->disconnectFromHost();
        });
    });
    LicenseClient tricked(key, "http://127.0.0.1:" + QString::number(evil.serverPort()), "pc2");
    assert(!tricked.check() && tricked.getMessage().contains("підпису"));

    // сервер вимкнено
    quint16 port = server.serverPort();
    server.close();
    LicenseClient offline(key, "http://127.0.0.1:" + QString::number(port), "pc2");
    assert(!offline.check() && offline.getMessage().contains("недоступний"));

    std::cout << "All tests passed" << std::endl;
    return 0;
}
