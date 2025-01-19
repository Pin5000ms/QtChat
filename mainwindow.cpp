#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "MessageDelegate.h"





MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
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


    // 發送訊息到伺服器
    connect(ui->sendButton, &QPushButton::clicked, this, [=]() {

        QString tmpdst = getSelectedRowId();
        if(tmpdst != "")//如果有選擇傳送對象，更新dst
            dst = tmpdst;

        QJsonObject json;
        json["type"] = "msg";  // 類型為msg
        json["from"] = myid;
        json["content"] = ui->msgEdit->toPlainText();//訊息內容
        json["to"] = dst; //發送到目的client


        if (socket->state() == QAbstractSocket::ConnectedState && !myid.isEmpty()) {
            // 將 JSON 對象轉為byte array
            QJsonDocument doc(json);
            QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
            socket->write(jsonData);

            sendMessage(ui->msgEdit->toPlainText(), ":/icon/avatar.png", "sent", dst);
        } else {
            qDebug() << "Not connected to server!";
        }

    });

    // 在主窗口中創建並設置自定義的委託
    MessageDelegate *delegate = new MessageDelegate(this);
    ui->chatroom->setItemDelegate(delegate);

    // 設置 QListView 的模型
    //ui->chatroom->setModel(chat_model);
    ui->chatroom->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    connect(socket, &QTcpSocket::readyRead, this, &MainWindow::on_received);



    ui->contacts->setModel(contact_model);

    // 連接按鈕的點擊事件
    connect(ui->sendFileButton, &QPushButton::clicked, this, &MainWindow::onSendFileButtonClicked);

}


void MainWindow::on_received()
{
    // 從 socket 讀取所有可用的數據
    QByteArray message = socket->readAll();
    qDebug() << "Received message:" << message;

    // 解析 JSON 數據
    QJsonDocument jsonDoc = QJsonDocument::fromJson(message);
    if (jsonDoc.isNull()) {
        qDebug() << "Failed to create JSON doc.";
        return;
    }

    // 檢查 JSON 是否為對象
    if (!jsonDoc.isObject()) {
        qDebug() << "JSON is not an object.";
        return;
    }

    // 取得 JSON 對象
    QJsonObject jsonObj = jsonDoc.object();

    // 存取 JSON 數據
    QString type = jsonObj["type"].toString();
    qDebug() << "Parsed type:" << type;

    if (type == "contacts") {
        QJsonArray contactsArray = jsonObj["content"].toArray();
        for (const QJsonValue &value : contactsArray)
        {
            QJsonObject contactObj = value.toObject();

            int id = contactObj["id"].toInt();
            QString qid = QString::number(id);
            QString name = contactObj["name"].toString();

            if(current_contacts.find(qid) == current_contacts.end())
            {
                current_contacts[qid] = name;
                QString combined = "#" + qid + ":" + name;
                QStandardItem *newItem = new QStandardItem(combined);
                contact_model->appendRow(newItem);

                if(!chat_models.contains(qid))
                {
                    chat_models.insert(qid, new QStandardItemModel(this));
                }
            }
        }

        myid = jsonObj["getid"].toString();//更新自己的ID
        if(!chat_models.contains(myid)){
            chat_models.insert(myid, new QStandardItemModel(this));
            ui->chatroom->setModel(chat_models[myid]);
        }
    }
    else if(type == "msg"){
        QString from = jsonObj["from"].toString();
        QString message = jsonObj["content"].toString();
        dst = from;//收到來自from的訊息後，自動把下次傳送對象設定為from
        recvMessage(message, ":/icon/avatar.png", "receive", from);
    }
    else if(type == "file_chunk"){
        QString file_name = jsonObj["file_name"].toString();
        QFile file(file_name);

        // 嘗試以附加模式打開文件，不存在時自動創建
        if (!file.open(QIODevice::Append | QIODevice::WriteOnly)) {
            qWarning() << "Failed to open file for writing/appending:" << file_name;
            return;
        }

        // 解碼從 server 接收到的 base64 塊
        QByteArray decodedChunk = QByteArray::fromBase64(jsonObj["content"].toString().toUtf8());

        // 寫入塊數據
        qint64 bytesWritten = file.write(decodedChunk);
        if (bytesWritten == -1) {
            qWarning() << "Failed to write data to file:" << file_name;
        }

        file.close();
    }
    else if(type == "file_chunk_ack"){
        ack = true;
    }
}



QString MainWindow::getSelectedRowId()
{
     // 取得 QListView 的選擇模型
     QItemSelectionModel *selectionModel = ui->contacts->selectionModel();

     // 取得目前選取的索引
     QModelIndexList selectedIndexes = selectionModel->selectedIndexes();

     // 檢查是否有選取的項
     if (!selectedIndexes.isEmpty()) {
         // 取得第一個選取的索引
         QModelIndex index = selectedIndexes.first();

         // 取得選取行的內容
         QString selectedText = index.data(Qt::DisplayRole).toString();

         // 輸出選取的內容
         qDebug() << "Selected Row Content:" << selectedText;

         // 提取 id
         if (selectedText.startsWith("#")) {
             // 去掉開頭的 '#'
             QStringList parts = selectedText.mid(1).split(':'); // 分割字串
             QString idString = parts[0]; // 擷取 id
             return idString;
         }
         else {
             qDebug() << "String does not start with '#'.";
             return ""; // 傳回錯誤值
         }
     }
     else {
        qDebug() << "No row selected.";
        return ""; // 傳回錯誤值
     }
}

void MainWindow::sendMessage(const QString &message, const QString &avatarPath, const QString type, QString to)
{
    ui->current_contact->setText( "#" + to + current_contacts[to] ); //標籤更新成聊天對象名稱

    // 設置自定義角色值
    QStandardItem *item = new QStandardItem();

    item->setData(message, Qt::DisplayRole);    // 設置消息內容
    item->setData(QPixmap(avatarPath), Qt::DecorationRole);  // 設置頭像
    item->setData(type, MessageDelegate::MessageTypeRole);  // 設置消息類型：sent 或 received

    ui->chatroom->setModel(chat_models[to]);
    // 添加到模型中
    chat_models[to]->appendRow(item);

    // 自動滾動到底部
    ui->chatroom->scrollTo(chat_models[to]->index(chat_models[to]->rowCount() - 1, 0), QAbstractItemView::PositionAtBottom);
}

void MainWindow::recvMessage(const QString &message, const QString &avatarPath, const QString type, QString from)
{
    ui->current_contact->setText( "#" + from + current_contacts[from] );//標籤更新成聊天對象名稱


    // 設置自定義角色值
    QStandardItem *item = new QStandardItem();

    item->setData(message, Qt::DisplayRole);    // 設置消息內容
    item->setData(QPixmap(avatarPath), Qt::DecorationRole);  // 設置頭像
    item->setData(type, MessageDelegate::MessageTypeRole);  // 設置消息類型：sent 或 received


    ui->chatroom->setModel(chat_models[from]);

    chat_models[from]->appendRow(item);

    // 自動滾動到底部
    ui->chatroom->scrollTo(chat_models[from]->index(chat_models[from]->rowCount() - 1, 0), QAbstractItemView::PositionAtBottom);
}


void MainWindow::setupDragAndDrop() {
    ui->msgEdit->setAcceptDrops(true);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event) {
    const QMimeData *mimeData = event->mimeData();
    if (mimeData->hasUrls()) {
        QList<QUrl> urlList = mimeData->urls();
        if (!urlList.isEmpty()) {
            QString filePath = urlList.first().toLocalFile();
            sendFileToServer(filePath);
        }
    }
}


// 按鈕點擊時呼叫此槽函數
void MainWindow::onSendFileButtonClicked() {
    // 使用 QFileDialog 來選擇檔案
    QString filePath = QFileDialog::getOpenFileName(this, "Choose File", "", "All Files (*.*)");

    if (!filePath.isEmpty()) {
        sendFileToServer(filePath);  // 呼叫檔案傳送函數
    }
}

void MainWindow::sendFileToServer(const QString &filePath) {
    QtConcurrent::run([=]() {
        QFile file(filePath);
        QFileInfo fileInfo(file);
        qint64 fileSize = fileInfo.size();  // 取得文件大小
        QString fileName = fileInfo.fileName();  // 取得檔案名稱

        if (file.open(QIODevice::ReadOnly)) {
            qint64 offset = 0;
            const int chunkSize = 1024; // 每個區塊1KB
            while (offset < fileSize) {
                QByteArray chunk = file.read(chunkSize); // 讀取檔案塊
                QByteArray encodedChunk = chunk.toBase64(); // 編碼區塊為Base64

                QJsonObject json;
                json["type"] = "file_chunk";
                json["file_name"] = fileName;
                json["file_size"] = fileSize;
                dst = getSelectedRowId();
                json["from"] = myid;
                json["to"] = dst;
                json["content"] = QString::fromUtf8(encodedChunk);
                json["offset"] = offset;

                QJsonDocument doc(json);
                socket->write(doc.toJson(QJsonDocument::Compact));
                ack = false;
                socket->waitForBytesWritten();  // 等待檔案區塊發送完畢

                offset += chunkSize;
                while(!ack){
                    QThread::msleep(10);
                }
            }
            file.close();
        }
    });
}


MainWindow::~MainWindow()
{
    delete ui;
}

