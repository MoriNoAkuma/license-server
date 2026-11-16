#include "licensedialog.h"
#include <QClipboard>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>

// напис зі стилем із style.qss
static QLabel* text(QString s, QString style){
    QLabel* l = new QLabel(s);
    l->setObjectName(style);
    return l;
}

LicenseDialog::LicenseDialog(LicenseStore& store, QWidget* parent) : QDialog(parent), store(store) {
    setWindowTitle("Нова ліцензія");
    setFixedSize(520, 520);
    pages = new QStackedWidget;
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(32, 32, 32, 32);
    root->addWidget(pages);

    // сторінка 1 - форма
    QWidget* form = new QWidget;
    QVBoxLayout* f = new QVBoxLayout(form);
    f->setContentsMargins(0, 0, 0, 0);
    f->setSpacing(8);
    f->addWidget(text("ВИДАЧА КЛЮЧА", "kickerUv"));
    f->addWidget(text("НОВА ЛІЦЕНЗІЯ", "h1"));
    f->addWidget(text("Ключ згенерується сам у форматі <span style='font-family:\"JetBrains Mono\"'>LIS-XXXX-XXXX-XXXX</span>.", "hint"));
    f->addSpacing(16);
    f->addWidget(text("Власник", "label"));
    ownerEdit = new QLineEdit;
    ownerEdit->setPlaceholderText("Ім’я покупця або назва компанії");
    f->addWidget(ownerEdit);
    f->addSpacing(16);
    f->addWidget(text("Примітка <span style='font-weight:400; color:#868492'>— необов’язково</span>", "label"));
    noteEdit = new QPlainTextEdit;
    noteEdit->setPlaceholderText("Оплата, термін, контакт");
    noteEdit->setFixedHeight(96);
    f->addWidget(noteEdit);
    f->addStretch();

    QHBoxLayout* fb = new QHBoxLayout;
    fb->addStretch();
    QPushButton* cancel = new QPushButton("СКАСУВАТИ");
    createBtn = new QPushButton(QIcon(":/icons/key-dark.svg"), "СТВОРИТИ КЛЮЧ");
    createBtn->setObjectName("primary");
    createBtn->setEnabled(false);
    fb->addWidget(cancel);
    fb->addWidget(createBtn);
    f->addLayout(fb);
    pages->addWidget(form);

    // сторінка 2 - готовий ключ
    QWidget* done = new QWidget;
    QVBoxLayout* d = new QVBoxLayout(done);
    d->setContentsMargins(0, 0, 0, 0);
    d->setSpacing(8);
    QLabel* ok = text("<img src=':/icons/check.svg'> ЛІЦЕНЗІЮ СТВОРЕНО", "kickerOk");
    d->addWidget(ok);
    d->addWidget(text("КЛЮЧ <span style='color:#d4a3ff'>ГОТОВИЙ</span>", "h1"));
    d->addSpacing(16);

    QFrame* panel = new QFrame;
    panel->setObjectName("keyPanel");
    QVBoxLayout* p = new QVBoxLayout(panel);
    p->setContentsMargins(16, 28, 16, 28);
    p->setSpacing(16);
    keyLabel = text("", "bigKey");
    keyLabel->setAlignment(Qt::AlignCenter);
    keyLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    QPushButton* copy = new QPushButton(QIcon(":/icons/copy.svg"), "СКОПІЮВАТИ");
    p->addWidget(keyLabel);
    p->addWidget(copy, 0, Qt::AlignCenter);
    d->addWidget(panel);
    d->addSpacing(16);

    ownerLabel = text("", "label");
    d->addWidget(ownerLabel);
    QLabel* info = text("Передай ключ покупцю. Перша перевірка прив’яже його до ПК покупця.", "hint");
    info->setWordWrap(true);
    d->addWidget(info);
    d->addStretch();

    QHBoxLayout* db = new QHBoxLayout;
    db->addStretch();
    QPushButton* more = new QPushButton("ЩЕ ОДНУ");
    QPushButton* finish = new QPushButton("ГОТОВО");
    finish->setObjectName("primary");
    db->addWidget(more);
    db->addWidget(finish);
    d->addLayout(db);
    pages->addWidget(done);

    // без власника ключ не створюємо
    connect(ownerEdit, &QLineEdit::textChanged, this, [this](QString s){ createBtn->setEnabled(!s.trimmed().isEmpty()); });
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(createBtn, &QPushButton::clicked, this, &LicenseDialog::create);
    connect(copy, &QPushButton::clicked, this, [this]{ QGuiApplication::clipboard()->setText(createdKey); });
    connect(more, &QPushButton::clicked, this, &LicenseDialog::again);
    connect(finish, &QPushButton::clicked, this, &QDialog::accept);
}

void LicenseDialog::create(){
    QString owner = ownerEdit->text().trimmed();
    QString note = noteEdit->toPlainText().trimmed();
    createdKey = store.create(owner, note);
    keyLabel->setText(createdKey);
    ownerLabel->setText(note.isEmpty() ? owner : owner + " · " + note);
    pages->setCurrentIndex(1);
}

void LicenseDialog::again(){
    ownerEdit->clear();
    noteEdit->clear();
    pages->setCurrentIndex(0);
    ownerEdit->setFocus();
}
