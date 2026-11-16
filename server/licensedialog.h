#pragma once

#include "licensestore.h"
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStackedWidget>

// видача нового ключа: форма -> готовий ключ
class LicenseDialog : public QDialog {
    Q_OBJECT
private:
    LicenseStore& store;
    QStackedWidget* pages;
    QLineEdit* ownerEdit;
    QPlainTextEdit* noteEdit;
    QPushButton* createBtn;
    QLabel* keyLabel;
    QLabel* ownerLabel;
    QString createdKey;
public:
    LicenseDialog(LicenseStore& store, QWidget* parent = nullptr);
    QString getCreatedKey() const { return createdKey; }
private slots:
    void create();
    void again();
};
