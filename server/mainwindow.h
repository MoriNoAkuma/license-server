#pragma once

#include "licensestore.h"
#include <QMainWindow>
#include <QButtonGroup>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QTableWidget>

// панель адміна: статистика, таблиця ліцензій, деталі вибраної
class MainWindow : public QMainWindow {
    Q_OBJECT
private:
    LicenseStore& store;
    QString selected; // ключ вибраної ліцензії

    QLabel *totalLabel, *activeLabel, *inactiveLabel, *blockedLabel;
    QLineEdit* search;
    QButtonGroup* filters;
    QTableWidget* table;
    QLabel* shownLabel;

    QStackedWidget* details;
    QLabel *selKey, *selStatus, *selOwner, *selNote, *selHwid, *selCreated, *selActivated, *selLastSeen;
    QPushButton *resetBtn, *blockBtn;

    QWidget* buildDetails();
    void showDetails();
public:
    MainWindow(LicenseStore& store, QString address, QWidget* parent = nullptr);
public slots:
    void refresh();
private slots:
    void newLicense();
    void resetHwid();
    void toggleBlock();
    void removeLicense();
};
