#include "mainwindow.h"
#include "licensedialog.h"
#include <QClipboard>
#include <QGridLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QVBoxLayout>

// статус у License::status() і назва кнопки фільтра
static const QStringList FILTERS = {"", "Активна", "Не активована", "Заблокована"};
static const QStringList FILTER_NAMES = {"Усі", "Активні", "Не активовані", "Заблоковані"};

// напис зі стилем із style.qss
static QLabel* text(QString s, QString style){
    QLabel* l = new QLabel(s);
    l->setObjectName(style);
    return l;
}

static QString pillStyle(QString status){
    QString base = "padding: 2px 8px; border-radius: 2px; font-size: 13px; font-weight: 600;";
    if (status == "Активна") return base + "color: #1ad1d1; background: rgba(26, 209, 209, 0.14);";
    if (status == "Заблокована") return base + "color: #fb5b40; background: rgba(251, 91, 64, 0.14);";
    return base + "color: #aba9b7; background: #100d1b;";
}

// плашка статусу для клітинки таблиці
static QWidget* pill(QString status){
    QWidget* w = new QWidget;
    QHBoxLayout* l = new QHBoxLayout(w);
    l->setContentsMargins(8, 0, 8, 0);
    QLabel* p = new QLabel(status);
    p->setStyleSheet(pillStyle(status));
    l->addWidget(p, 0, Qt::AlignVCenter);
    l->addStretch();
    return w;
}

static QString date(QDateTime d){
    return d.isValid() ? d.toString("dd.MM.yyyy HH:mm") : "—";
}

static QFont mono(bool bold){
    QFont f("JetBrains Mono");
    f.setPixelSize(13);
    if (bold) f.setWeight(QFont::DemiBold);
    return f;
}

MainWindow::MainWindow(LicenseStore& store, QString address, QWidget* parent) : QMainWindow(parent), store(store) {
    setWindowTitle("Сервер ліцензій");
    resize(1280, 860);
    setMinimumSize(1040, 500);
    QWidget* central = new QWidget;
    setCentralWidget(central);
    QVBoxLayout* root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // шапка: назва і стан сервера
    QFrame* header = new QFrame;
    header->setObjectName("header");
    header->setFixedHeight(64);
    QHBoxLayout* h = new QHBoxLayout(header);
    h->setContentsMargins(32, 0, 32, 0);
    h->setSpacing(12);
    QLabel* logo = new QLabel;
    logo->setObjectName("logo");
    logo->setFixedSize(36, 36);
    logo->setAlignment(Qt::AlignCenter);
    logo->setPixmap(QIcon(":/icons/key.svg").pixmap(20, 20));
    h->addWidget(logo);
    h->addWidget(text("СЕРВЕР ЛІЦЕНЗІЙ", "brand"));
    h->addStretch();
    QString chip = address.isEmpty()
        ? "<span style='color:#fb5b40'>●</span> Офлайн"
        : "<span style='color:#cbec41'>●</span> Онлайн <span style='font-family:\"JetBrains Mono\"; font-weight:400; color:#aba9b7'>" + address + "</span>";
    h->addWidget(text(chip, "chip"), 0, Qt::AlignVCenter);
    h->addSpacing(4);
    h->addWidget(text("licenses.json", "mono"));
    root->addWidget(header);

    // вміст прокручується, якщо вікно низьке (як сторінка в браузері)
    QWidget* content = new QWidget;
    content->setObjectName("page");
    QVBoxLayout* page = new QVBoxLayout(content);
    page->setContentsMargins(32, 32, 32, 32);
    page->setSpacing(24);
    QScrollArea* scroll = new QScrollArea;
    scroll->setObjectName("pageScroll");
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(content);
    root->addWidget(scroll);

    // заголовок і кнопка видачі ключа
    QHBoxLayout* top = new QHBoxLayout;
    QVBoxLayout* titles = new QVBoxLayout;
    titles->setSpacing(4);
    titles->addWidget(text("ПАНЕЛЬ АДМІНІСТРАТОРА", "kickerUv"));
    titles->addWidget(text("ЛІЦЕНЗІЇ", "h1"));
    top->addLayout(titles);
    top->addStretch();
    QPushButton* newBtn = new QPushButton(QIcon(":/icons/plus.svg"), "НОВА ЛІЦЕНЗІЯ");
    newBtn->setObjectName("primary");
    top->addWidget(newBtn, 0, Qt::AlignBottom);
    page->addLayout(top);

    // картки з кількістю
    QHBoxLayout* cards = new QHBoxLayout;
    cards->setSpacing(16);
    auto card = [&](QString title, QString color){
        QFrame* c = new QFrame;
        c->setObjectName("card");
        QVBoxLayout* l = new QVBoxLayout(c);
        l->setContentsMargins(20, 16, 20, 16);
        l->setSpacing(8);
        QLabel* number = text("0", "stat");
        number->setStyleSheet("color: " + color);
        l->addWidget(text(title, "kicker"));
        l->addWidget(number);
        cards->addWidget(c);
        return number;
    };
    totalLabel = card("УСЬОГО", "#f2f1f6");
    activeLabel = card("АКТИВНІ", "#1ad1d1");
    inactiveLabel = card("НЕ АКТИВОВАНІ", "#aba9b7");
    blockedLabel = card("ЗАБЛОКОВАНІ", "#fb5b40");
    page->addLayout(cards);

    // таблиця з пошуком і фільтром
    QHBoxLayout* body = new QHBoxLayout;
    body->setSpacing(24);
    QFrame* panel = new QFrame;
    panel->setObjectName("panel");
    QVBoxLayout* pl = new QVBoxLayout(panel);
    pl->setContentsMargins(0, 0, 0, 0);
    pl->setSpacing(0);

    QHBoxLayout* tools = new QHBoxLayout;
    tools->setContentsMargins(16, 16, 16, 16);
    tools->setSpacing(12);
    search = new QLineEdit;
    search->setPlaceholderText("Ключ, власник або HWID");
    search->addAction(QIcon(":/icons/search.svg"), QLineEdit::LeadingPosition);
    search->setClearButtonEnabled(true);
    tools->addWidget(search, 1);
    QFrame* tabs = new QFrame;
    tabs->setObjectName("tabs");
    QHBoxLayout* tl = new QHBoxLayout(tabs);
    tl->setContentsMargins(4, 4, 4, 4);
    tl->setSpacing(4);
    filters = new QButtonGroup(this);
    for (int i = 0; i < FILTERS.size(); i++) {
        QPushButton* b = new QPushButton(FILTER_NAMES[i]);
        b->setCheckable(true);
        b->setChecked(i == 0);
        filters->addButton(b, i);
        tl->addWidget(b);
    }
    tools->addWidget(tabs);
    pl->addLayout(tools);

    table = new QTableWidget(0, 5);
    table->setHorizontalHeaderLabels({"КЛЮЧ", "ВЛАСНИК", "СТАТУС", "ПРИВ’ЯЗКА (HWID)", "ОСТАННЯ ПЕРЕВІРКА"});
    table->verticalHeader()->hide();
    table->verticalHeader()->setDefaultSectionSize(60);
    table->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    table->horizontalHeader()->setHighlightSections(false);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table->setColumnWidth(0, 190);
    table->setColumnWidth(2, 155);
    table->setColumnWidth(3, 140);
    table->setColumnWidth(4, 160);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setFocusPolicy(Qt::NoFocus);
    table->setShowGrid(false);
    table->setWordWrap(false);
    pl->addWidget(table, 1);

    QHBoxLayout* footer = new QHBoxLayout;
    footer->setContentsMargins(16, 12, 16, 12);
    shownLabel = text("", "mono");
    footer->addWidget(shownLabel);
    footer->addStretch();
    footer->addWidget(text("POST /api/verify", "mono"));
    pl->addLayout(footer);

    body->addWidget(panel, 1);
    body->addWidget(buildDetails(), 0, Qt::AlignTop);
    page->addLayout(body, 1);

    connect(newBtn, &QPushButton::clicked, this, &MainWindow::newLicense);
    connect(search, &QLineEdit::textChanged, this, &MainWindow::refresh);
    connect(filters, &QButtonGroup::idClicked, this, &MainWindow::refresh);
    connect(table, &QTableWidget::itemSelectionChanged, this, [this]{
        QList<QTableWidgetItem*> items = table->selectedItems();
        if (items.isEmpty()) return;
        selected = table->item(items[0]->row(), 0)->text();
        showDetails();
    });
    refresh();
}

// права панель: деталі вибраної ліцензії і дії
QWidget* MainWindow::buildDetails(){
    QFrame* side = new QFrame;
    side->setObjectName("panel");
    side->setFixedWidth(340);
    QVBoxLayout* sl = new QVBoxLayout(side);
    sl->setContentsMargins(24, 24, 24, 24);
    details = new QStackedWidget;
    sl->addWidget(details);

    QLabel* empty = text("Вибери ліцензію в таблиці, щоб побачити деталі та дії.", "muted");
    empty->setWordWrap(true);
    empty->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    details->addWidget(empty);

    QWidget* info = new QWidget;
    QVBoxLayout* l = new QVBoxLayout(info);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(6);
    l->addWidget(text("ВИБРАНА ЛІЦЕНЗІЯ", "kickerUv"));
    QHBoxLayout* keyRow = new QHBoxLayout;
    selKey = text("", "selKey");
    QPushButton* copy = new QPushButton(QIcon(":/icons/copy.svg"), "");
    copy->setObjectName("iconBtn");
    copy->setToolTip("Скопіювати ключ");
    copy->setAccessibleName("Скопіювати ключ");
    keyRow->addWidget(selKey);
    keyRow->addStretch();
    keyRow->addWidget(copy);
    l->addLayout(keyRow);
    selStatus = new QLabel;
    l->addWidget(selStatus, 0, Qt::AlignLeft);
    l->addSpacing(12);

    // назва поля + значення
    auto field = [](QString title, QString style, QLabel*& value){
        QWidget* w = new QWidget;
        QVBoxLayout* f = new QVBoxLayout(w);
        f->setContentsMargins(0, 0, 0, 0);
        f->setSpacing(2);
        value = text("", style);
        value->setTextFormat(Qt::PlainText); // власник і примітка - не html
        value->setWordWrap(style == "muted"); // переносимо тільки примітку, інакше layout стискає поля
        f->addWidget(text(title, "kicker"));
        f->addWidget(value);
        return w;
    };
    l->addWidget(field("ВЛАСНИК", "strong", selOwner));
    l->addWidget(field("ПРИМІТКА", "muted", selNote));
    l->addWidget(field("ПРИВ’ЯЗКА ДО ПК (HWID)", "valueMuted", selHwid));
    QGridLayout* dates = new QGridLayout;
    dates->setSpacing(10);
    dates->addWidget(field("СТВОРЕНО", "value", selCreated), 0, 0);
    dates->addWidget(field("АКТИВОВАНО", "value", selActivated), 0, 1);
    dates->addWidget(field("ОСТАННЯ ПЕРЕВІРКА", "value", selLastSeen), 1, 0, 1, 2);
    l->addLayout(dates);

    l->addSpacing(8);
    QFrame* line = new QFrame;
    line->setObjectName("line");
    l->addWidget(line);
    l->addSpacing(8);
    resetBtn = new QPushButton(QIcon(":/icons/reset.svg"), "СКИНУТИ HWID");
    blockBtn = new QPushButton(QIcon(":/icons/lock.svg"), "ЗАБЛОКУВАТИ");
    QPushButton* del = new QPushButton(QIcon(":/icons/trash.svg"), "ВИДАЛИТИ");
    del->setObjectName("danger");
    l->addWidget(resetBtn);
    l->addWidget(blockBtn);
    l->addWidget(del);
    QLabel* tip = text("Скинь HWID, коли покупець легально змінив ПК: ключ прив’яжеться до нового при першій перевірці.", "hint");
    tip->setWordWrap(true);
    l->addWidget(tip);
    details->addWidget(info);

    connect(copy, &QPushButton::clicked, this, [this]{ QGuiApplication::clipboard()->setText(selected); });
    connect(resetBtn, &QPushButton::clicked, this, &MainWindow::resetHwid);
    connect(blockBtn, &QPushButton::clicked, this, &MainWindow::toggleBlock);
    connect(del, &QPushButton::clicked, this, &MainWindow::removeLicense);
    return side;
}

void MainWindow::refresh(){
    QList<License> all = store.all();
    int counts[4] = {(int)all.size(), 0, 0, 0};
    for (const License& lic : all)
        counts[FILTERS.indexOf(lic.status())]++;
    totalLabel->setText(QString::number(counts[0]));
    activeLabel->setText(QString::number(counts[1]));
    inactiveLabel->setText(QString::number(counts[2]));
    blockedLabel->setText(QString::number(counts[3]));
    for (int i = 0; i < FILTERS.size(); i++)
        filters->button(i)->setText(FILTER_NAMES[i] + "   " + QString::number(counts[i]));

    QString want = FILTERS[filters->checkedId()];
    QString query = search->text().trimmed();
    QSignalBlocker blocker(table); // перебудова таблиці не скидає вибір
    table->setRowCount(0);
    for (const License& lic : all) {
        if (!want.isEmpty() && lic.status() != want) continue;
        if (!(lic.getKey() + " " + lic.getOwner() + " " + lic.getHwid()).contains(query, Qt::CaseInsensitive)) continue;

        int row = table->rowCount();
        table->insertRow(row);
        QTableWidgetItem* key = new QTableWidgetItem(lic.getKey());
        key->setFont(mono(true));
        table->setItem(row, 0, key);

        QString note = lic.getNote().isEmpty() ? "—" : lic.getNote().toHtmlEscaped();
        QLabel* owner = new QLabel("<span style='font-weight:600'>" + lic.getOwner().toHtmlEscaped() + "</span><br>"
                                   "<span style='font-size:13px; color:#868492'>" + note + "</span>");
        owner->setContentsMargins(8, 0, 8, 0);
        table->setCellWidget(row, 1, owner);
        table->setCellWidget(row, 2, pill(lic.status()));

        QString hwid = lic.getHwid();
        QTableWidgetItem* hw = new QTableWidgetItem(hwid.isEmpty() ? "—" : hwid.left(8) + "…" + hwid.right(4));
        QTableWidgetItem* seen = new QTableWidgetItem(date(lic.getLastSeen()));
        for (QTableWidgetItem* item : {hw, seen}) {
            item->setFont(mono(false));
            item->setForeground(QColor("#aba9b7"));
        }
        hw->setToolTip(hwid);
        table->setItem(row, 3, hw);
        table->setItem(row, 4, seen);
        if (lic.getKey() == selected) table->selectRow(row);
    }
    shownLabel->setText(QString("Показано %1 з %2").arg(table->rowCount()).arg(all.size()));
    showDetails();
}

void MainWindow::showDetails(){
    if (!store.contains(selected)) {
        details->setCurrentIndex(0);
        return;
    }
    License lic = store.get(selected);
    details->setCurrentIndex(1);
    selKey->setText(lic.getKey());
    selStatus->setText(lic.status());
    selStatus->setStyleSheet(pillStyle(lic.status()));
    selOwner->setText(lic.getOwner());
    selNote->setText(lic.getNote().isEmpty() ? "—" : lic.getNote());
    selHwid->setText(lic.isActivated() ? lic.getHwid() : "Ще не прив’язана: прив’яжеться при першій перевірці");
    selCreated->setText(date(lic.getCreatedAt()));
    selActivated->setText(date(lic.getActivatedAt()));
    selLastSeen->setText(date(lic.getLastSeen()));
    resetBtn->setEnabled(lic.isActivated());
    blockBtn->setText(lic.isBlocked() ? "РОЗБЛОКУВАТИ" : "ЗАБЛОКУВАТИ");
}

void MainWindow::newLicense(){
    LicenseDialog dialog(store, this);
    dialog.exec();
    if (!dialog.getCreatedKey().isEmpty()) selected = dialog.getCreatedKey();
    refresh();
}

void MainWindow::resetHwid(){
    if (!store.resetHwid(selected)) QMessageBox::warning(this, "Помилка", "Не вдалося зберегти licenses.json");
    refresh();
}

void MainWindow::toggleBlock(){
    if (!store.setBlocked(selected, !store.get(selected).isBlocked()))
        QMessageBox::warning(this, "Помилка", "Не вдалося зберегти licenses.json");
    refresh();
}

void MainWindow::removeLicense(){
    if (QMessageBox::question(this, "Видалення", "Видалити ліцензію " + selected + "?\nПрограма покупця перестане запускатися.")
        != QMessageBox::Yes) return;
    if (!store.remove(selected)) QMessageBox::warning(this, "Помилка", "Не вдалося зберегти licenses.json");
    selected.clear();
    refresh();
}
