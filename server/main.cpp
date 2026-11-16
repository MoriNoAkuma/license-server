#include "mainwindow.h"
#include "licenseserver.h"
#include <QApplication>
#include <QFile>
#include <QFontDatabase>
#include <QLibraryInfo>
#include <QMessageBox>
#include <QStyleHints>
#include <QTranslator>

int main(int argc, char* argv[]){
    QApplication app(argc, argv);
    app.styleHints()->setColorScheme(Qt::ColorScheme::Dark);

    // переклад стандартних кнопок Qt
    QTranslator translator;
    if (translator.load(QLocale(QLocale::Ukrainian), "qtbase", "_", QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
        app.installTranslator(&translator);

    // шрифти і стиль з ресурсів, щоб однаково виглядало на будь-якому ПК
    for (QString font : {"Oswald", "Onest", "JetBrainsMono"})
        QFontDatabase::addApplicationFont(":/fonts/" + font + ".ttf");
    QFile qss(":/style.qss");
    if (qss.open(QIODevice::ReadOnly)) app.setStyleSheet(qss.readAll());

    QString path = QCoreApplication::applicationDirPath() + "/licenses.json";
    LicenseStore store(path);
    if (!store.load() && QFile::exists(path)) {
        QMessageBox::critical(nullptr, "Сервер ліцензій", "Файл licenses.json пошкоджений. Виправ або видали його.");
        return 1;
    }

    LicenseServer server(store);
    bool online = server.listen(QHostAddress::Any, 8099);
    MainWindow w(store, online ? "0.0.0.0:8099" : "");
    QObject::connect(&server, &LicenseServer::changed, &w, &MainWindow::refresh);
    w.show();
    if (!online) QMessageBox::warning(&w, "Сервер ліцензій", "Не вдалося відкрити порт 8099: " + server.errorString());
    return app.exec();
}
