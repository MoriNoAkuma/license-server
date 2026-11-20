# Етап 4. Архітектура та патерни

## Шари

```mermaid
flowchart TB
    subgraph GUI["LicenseServer.exe — інтерфейс (Qt Widgets)"]
        MW[MainWindow]
        LD[LicenseDialog]
    end
    subgraph CORE["licensecore — ядро (Qt Core + Network, без вікон)"]
        LS[LicenseServer]
        ST[LicenseStore]
        L[License]
    end
    subgraph DATA["Дані"]
        J[(licenses.json)]
    end
    subgraph CLIENT["licenseclient — у захищеній програмі"]
        LC[LicenseClient]
    end
    MW --> ST
    LD --> ST
    LS --> ST
    ST --> L
    ST --> J
    LC -- "HTTP POST /api/verify" --> LS
    LS -. "сигнал changed()" .-> MW
```

## UML-діаграма класів

```mermaid
classDiagram
    direction LR

    class License {
        -QString key
        -QString owner
        -QString note
        -bool blocked
        -QString hwid
        -QDateTime createdAt
        -QDateTime activatedAt
        -QDateTime lastSeen
        +License(key, owner, note)
        +getKey() QString
        +getOwner() QString
        +getHwid() QString
        +isBlocked() bool
        +isActivated() bool
        +status() QString
        +setBlocked(bool)
        +activate(hwid)
        +resetHwid()
        +touch()
        +toJson() QJsonObject
        +fromJson(key, obj)$ License
    }

    class LicenseStore {
        -QMap~QString, License~ licenses
        -QString path
        +LicenseStore(path)
        +all() QList~License~
        +contains(key) bool
        +get(key) License
        +create(owner, note) QString
        +setBlocked(key, blocked) bool
        +resetHwid(key) bool
        +remove(key) bool
        +verify(key, hwid) QJsonObject
        +load() bool
        +save() bool
        +generateKey()$ QString
    }

    class QTcpServer
    <<Qt>> QTcpServer

    class LicenseServer {
        -LicenseStore& store
        -handle(QTcpSocket*)
        +LicenseServer(store, parent)
        +sign(obj)$ QByteArray
        +changed() signal
    }

    class QMainWindow
    <<Qt>> QMainWindow
    class QDialog
    <<Qt>> QDialog

    class MainWindow {
        -LicenseStore& store
        -QString selected
        -QTableWidget* table
        -QLineEdit* search
        -QButtonGroup* filters
        -QStackedWidget* details
        +MainWindow(store, address, parent)
        +refresh() slot
        -newLicense() slot
        -resetHwid() slot
        -toggleBlock() slot
        -removeLicense() slot
        -showDetails()
    }

    class LicenseDialog {
        -LicenseStore& store
        -QStackedWidget* pages
        -QLineEdit* ownerEdit
        -QString createdKey
        +LicenseDialog(store, parent)
        +getCreatedKey() QString
        -create() slot
        -again() slot
    }

    class LicenseClient {
        -QString key
        -QString server
        -QString hwid
        -QString message
        -send(body) QJsonObject
        +LicenseClient(key, server, hwid)
        +check() bool
        +getMessage() QString
        +hardwareId()$ QString
        +sign(obj)$ QByteArray
    }

    LicenseStore "1" *-- "0..*" License : зберігає
    QTcpServer <|-- LicenseServer
    QMainWindow <|-- MainWindow
    QDialog <|-- LicenseDialog
    LicenseServer --> LicenseStore : використовує
    MainWindow --> LicenseStore : використовує
    LicenseDialog --> LicenseStore : використовує
    MainWindow ..> LicenseDialog : створює
    LicenseServer ..> MainWindow : changed() → refresh()
    LicenseClient ..> LicenseServer : HTTP /api/verify
```

## Використані патерни проєктування

| Патерн | Де | Навіщо |
|---|---|---|
| **Repository** | `LicenseStore` | Вікно і сервер працюють з колекцією ліцензій через методи `create / get / remove / verify`, не знаючи, що всередині JSON-файл. Заміна на SQLite не зачепить GUI. |
| **Observer** (сигнали і слоти Qt) | `LicenseServer::changed` → `MainWindow::refresh`; `textChanged`, `idClicked`, `itemSelectionChanged` | Сервер не знає про вікно, але таблиця оновлюється, щойно клієнт активувався. |
| **Facade** | `requireLicense()` | Ховає HWID, HTTP, HMAC, nonce і діалоги помилок за одним викликом. Розробнику захищеної програми треба один рядок. |
| **Dependency Injection** (через конструктор) | `LicenseStore&` передається в `LicenseServer`, `MainWindow`, `LicenseDialog` | Один екземпляр сховища на всю програму; у тестах підставляється сховище з тимчасовим файлом. |
| **Static Factory Method** | `License::fromJson`, `LicenseStore::generateKey` | Створення ліцензії з JSON і генерація ключа в одному місці, з правилами формату. |
| **Composite** (дерево об'єктів Qt) | віджети вікна | Вікно — дерево віджетів; батько малює і видаляє всіх дітей. |

## Управління пам'яттю

- **Дерево об'єктів Qt.** Віджети створюються через `new` і одразу кладуться в layout, який робить їх дітьми вікна. Коли вікно знищується, Qt видаляє всіх нащадків — ручних `delete` у коді немає.
- **Тимчасові об'єкти мережі.** Кожен `QTcpSocket` видаляється через `deleteLater` по сигналу `disconnected`; `QNetworkReply` — через `deleteLater` після прочитання.
- **Стек для довгоживучих об'єктів.** `LicenseStore`, `LicenseServer`, `MainWindow` живуть на стеку `main()` у правильному порядку: сховище створюється першим і знищується останнім, тому посилання `LicenseStore&` в інших класах ніколи не «висить».
- **Модальний діалог** `LicenseDialog` створюється на стеку `MainWindow::newLicense()` і зникає після `exec()`.
- **Значення замість вказівників.** `License` — звичайний копійований клас у `QMap`, тому витоків тут бути не може.
