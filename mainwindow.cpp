#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "MessageDelegate.h"




MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , chat_model(new QStandardItemModel(this))
    , contact_model(new QStandardItemModel(this))
    , socket(new QTcpSocket())
{
    ui->setupUi(this);


    connect(socket, &QTcpSocket::connected, []() {
        qDebug() << "Connected to server!";
    });

    connect(socket, &QTcpSocket::disconnected, []() {
        qDebug() << "Disconnected from server!";
    });

    // 連接到Server
    connect(socket, &QTcpSocket::connected, [=]() {
        // 創建json對象
        QJsonObject json;
        json["type"] = "name";
        json["content"] = "Tom";

        // 將 JSON 對象轉為byte array
        QJsonDocument doc(json);
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

        // 發送 JSON 數據
        socket->write(jsonData);
    });


    socket->connectToHost(QHostAddress("127.0.0.1"), 9527);

    qDebug()<<socket;


    // 發送數據到伺服器
    connect(ui->sendButton, &QPushButton::clicked, this, [=]() {

        QString id = getSelectedRowId();

        QJsonObject json;
        json["type"] = "msg";  // 類型為msg
        json["content"] = ui->msgEdit->toPlainText();//訊息內容
        json["dst"] = id; //發送到目的client


        if (socket->state() == QAbstractSocket::ConnectedState && !id.isEmpty()) {
            // 將 JSON 對象轉為byte array
            QJsonDocument doc(json);
            QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
            socket->write(jsonData);

            addMessage(ui->msgEdit->toPlainText(), ":/icon/avatar.png", "sent");
        } else {
            qDebug() << "Not connected to server!";
        }

    });

    // 在主窗口中創建並設置自定義的委託
    MessageDelegate *delegate = new MessageDelegate(this);
    ui->chatroom->setItemDelegate(delegate);

    // 設置 QListView 的模型
    ui->chatroom->setModel(chat_model);
    ui->chatroom->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->chatroom->setStyleSheet("QListView::item { height: 50px; }");

    connect(socket, &QTcpSocket::readyRead, this, &MainWindow::on_msg_received);


    ui->contacts->setModel(contact_model);

}


void MainWindow::on_msg_received()
{
    // 从 socket 中读取所有可用的数据
    QByteArray message = socket->readAll();
    qDebug() << "Received message:" << message;

    // 解析 JSON 数据
    QJsonDocument jsonDoc = QJsonDocument::fromJson(message);
    if (jsonDoc.isNull()) {
        qDebug() << "Failed to create JSON doc.";
        return;
    }

    // 检查 JSON 是否是对象
    if (!jsonDoc.isObject()) {
        qDebug() << "JSON is not an object.";
        return;
    }

    // 获取 JSON 对象
    QJsonObject jsonObj = jsonDoc.object();

    // 访问 JSON 数据
    QString type = jsonObj["type"].toString();
    qDebug() << "Parsed type:" << type;

    if (type == "contacts") {
        QJsonArray contactsArray = jsonObj["content"].toArray();
        for (const QJsonValue &value : contactsArray) {
            QJsonObject contactObj = value.toObject();
            int id = contactObj["id"].toInt();
            std::string name = contactObj["name"].toString().toStdString();

            if(current_contacts.find(id) == current_contacts.end()){
                current_contacts[id] = name;
                std::string combined = "#" + std::to_string(id) + ":" + name;
                QStandardItem *newItem = new QStandardItem(QString::fromStdString(combined));
                contact_model->appendRow(newItem);
            }
        }
    }
    else if(type == "msg"){
        QString message = jsonObj["content"].toString();
        addMessage(message, ":/icon/avatar.png", "receive");
    }
}

QString MainWindow::getSelectedRowId()
{
    // 获取 QListView 的选择模型
    QItemSelectionModel *selectionModel = ui->contacts->selectionModel();

    // 获取当前选中的索引
    QModelIndexList selectedIndexes = selectionModel->selectedIndexes();

    // 检查是否有选中的项
    if (!selectedIndexes.isEmpty()) {
        // 获取第一个选中的索引
        QModelIndex index = selectedIndexes.first();

        // 获取选中行的内容
        QString selectedText = index.data(Qt::DisplayRole).toString();

        // 输出选中的内容
        qDebug() << "Selected Row Content:" << selectedText;

        // 提取 id
        if (selectedText.startsWith("#")) {
            // 去掉开头的 '#'
            QStringList parts = selectedText.mid(1).split(':'); // 分割字符串
            if (parts.size() == 2) {
                QString idString = parts[0]; // 提取 id
                QString name = parts[1]; // 提取 name
                std::string id = idString.toStdString(); // 转换为 int，ok 用于检查转换是否成功
                return idString; // 返回 id

            } else {
                qDebug() << "Invalid format. Expected #id:name.";
                return ""; // 返回错误值
            }
        } else {
            qDebug() << "String does not start with '#'.";
            return ""; // 返回错误值
        }
    } else {
        qDebug() << "No row selected.";
        return ""; // 返回错误值
    }
}

void MainWindow::addMessage(const QString &message, const QString &avatarPath, const QString type)
{
    // 設置自定義角色值
    QStandardItem *item = new QStandardItem();

    item->setData(message, Qt::DisplayRole);    // 設置消息內容
    item->setData(QPixmap(avatarPath), Qt::DecorationRole);  // 設置頭像
    item->setData(type, MessageDelegate::MessageTypeRole);  // 設置消息類型：sent 或 received

    // 添加到模型中
    chat_model->appendRow(item);

    // 自動滾動到底部
    ui->chatroom->scrollTo(chat_model->index(chat_model->rowCount() - 1, 0), QAbstractItemView::PositionAtBottom);
}


MainWindow::~MainWindow()
{
    delete ui;
}

