#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "MessageDelegate.h"
#include "ContactsDelegate.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , contact_model(new QStandardItemModel(this))
    , socket(new QTcpSocket())
{
    ui->setupUi(this);
    ui->clientName->setText("Client");


    connect(socket, &QTcpSocket::connected, []() {
        qDebug() << "Connected to server!";
    });

    connect(socket, &QTcpSocket::disconnected, []() {
        qDebug() << "Disconnected from server!";
    });

    // 連接到Server
    connect(socket, &QTcpSocket::connected, this, [=]() {
        // 創建json對象
        QJsonObject json;
        json["type"] = "name";
        json["content"] = ui->clientName->toPlainText();;

        // 將 JSON 對象轉為byte array
        QJsonDocument doc(json);
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

        // 發送 JSON 數據
        socket->write(jsonData);
    });


    socket->connectToHost(QHostAddress("127.0.0.1"), 9527);

    qDebug()<<socket;


    // 發送按鈕點擊事件
    connect(ui->sendButton, &QPushButton::clicked, this, &MainWindow::on_sended);

    // TCP接收事件
    connect(socket, &QTcpSocket::readyRead, this, &MainWindow::on_received);

    //選擇聯絡人點擊事件
    connect(ui->contacts, &QListView::clicked, this, &MainWindow::onContactsClicked);

    //傳送檔案按鈕的點擊事件
    connect(ui->sendFileButton, &QPushButton::clicked, this, &MainWindow::onSendFileButtonClicked);



    // 在聊天室視窗中創建並設置自定義的委託
    MessageDelegate *delegate = new MessageDelegate(this);
    ui->chatroom->setItemDelegate(delegate);
    ui->chatroom->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    //ui->chatroom->setModel(chat_model);// 設置 QListView 的模型


    // 在聯絡人清單中創建並設置自定義的委託
    ContactsDelegate *delegate2 = new ContactsDelegate(this);
    ui->contacts->setItemDelegate(delegate2);
    ui->contacts->setModel(contact_model);


}


void MainWindow::on_received()
{
    // 從 socket 讀取所有可用的數據
    QByteArray data = socket->readAll();




    if(filereceivemode){
        recvFileFromServer(data);
        return;
    }

    qDebug() << "Received message:" << data;

    // 解析 JSON 數據
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
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
        ui->clientName->setText("#" + myid + "Client");
        if(!chat_models.contains(myid)){
            chat_models.insert(myid, new QStandardItemModel(this));
            ui->chatroom->setModel(chat_models[myid]);
        }
    }
    else if(type == "msg"){
        QString from = jsonObj["from"].toString();
        QString message = jsonObj["content"].toString();
        dst = from;//收到來自from的訊息後，自動把下次傳送對象設定為from
        recvMessage(message, ":/icon/avatar.png", "receive", "text", from);
    }
    else if(type == "file_recv"){
        QString from = jsonObj["from"].toString();
        recv_file_name = jsonObj["file_name"].toString();
        file_size = jsonObj["file_size"].toInt();
        dst = from;//收到來自from的訊息後，自動把下次傳送對象設定為from
        recvMessage("", ":/icon/avatar.png", "receive", "file", from);

        // 彈出對話框詢問是否接收檔案
        QMessageBox msgBox;
        msgBox.setWindowTitle("File Receive");
        msgBox.setText(QString("File from %1 Accept?").arg(from));
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);

        // 根據用戶選擇處理
        if (msgBox.exec() == QMessageBox::Yes) {
            QJsonObject json;
            json["type"] = "file_recv_ack";  // 類型為msg
            json["from"] = myid;
            json["file_name"] = recv_file_name;
            json["file_size"] = file_size;
            QJsonDocument doc(json);
            QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
            socket->write(jsonData);
            filereceivemode = true;
            offset = 0;

        } else {
            // 如果用戶選擇拒絕，可以顯示一個提示或記錄
            qDebug() << "User refused the file from" << from;
        }
    }
    else if(type == "file_upload_ack"){
        filesendmode = true;
    }
}


void MainWindow::on_sended(){
    QString tmpdst = getSelectedRowId();
    if(tmpdst != "")//如果有選擇傳送對象，更新dst
        dst = tmpdst;

    QJsonObject json;
    json["type"] = "msg";  // 類型為msg
    json["from"] = myid;
    json["to"] = dst; //發送到目的client
    json["content"] = ui->msgEdit->toPlainText();//訊息內容



    if (socket->state() == QAbstractSocket::ConnectedState && !myid.isEmpty()) {
        // 將 JSON 對象轉為byte array
        QJsonDocument doc(json);
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
        socket->write(jsonData);

        sendMessage(ui->msgEdit->toPlainText(), ":/icon/avatar.png", "sent", "text", dst);
    } else {
        qDebug() << "Not connected to server!";
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

void MainWindow::sendMessage(const QString &message, const QString &avatarPath, const QString type, const QString datatype, QString to)
{
    if(dst != "")
        ui->current_contact->setText( "#" + to + current_contacts[to] ); //標籤更新成聊天對象名稱

    // 設置自定義角色值
    QStandardItem *item = new QStandardItem();

    item->setData(message, Qt::DisplayRole);
    item->setData(QPixmap(avatarPath), Qt::DecorationRole);
    item->setData(type, MessageDelegate::DirectionTypeRole);
    item->setData(datatype, MessageDelegate::DataTypeRole);

    if(!chat_models.contains(to)){
        chat_models[to] = new QStandardItemModel(this);
    }

    ui->chatroom->setModel(chat_models[to]);
    // 添加到模型中
    chat_models[to]->appendRow(item);

    // 自動滾動到底部
    ui->chatroom->scrollTo(chat_models[to]->index(chat_models[to]->rowCount() - 1, 0), QAbstractItemView::PositionAtBottom);
}

void MainWindow::recvMessage(const QString &message, const QString &avatarPath, const QString type, const QString datatype, QString from)
{
    ui->current_contact->setText( "#" + from + current_contacts[from] );//標籤更新成聊天對象名稱


    // 設置自定義角色值
    QStandardItem *item = new QStandardItem();

    item->setData(message, Qt::DisplayRole);    // 設置消息內容
    item->setData(QPixmap(avatarPath), Qt::DecorationRole);  // 設置頭像
    item->setData(type, MessageDelegate::DirectionTypeRole);  // 設置消息類型：sent 或 receive
    item->setData(datatype, MessageDelegate::DataTypeRole);

    if(!chat_models.contains(from)){
        chat_models[from] = new QStandardItemModel(this);
    }

    ui->chatroom->setModel(chat_models[from]);

    chat_models[from]->appendRow(item);

    // 自動滾動到底部
    ui->chatroom->scrollTo(chat_models[from]->index(chat_models[from]->rowCount() - 1, 0), QAbstractItemView::PositionAtBottom);
}

/*
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
*/

//切換聊天室
void MainWindow::onContactsClicked(){
    dst = getSelectedRowId();
    ui->chatroom->setModel(chat_models[dst]);//切換聊天室
    ui->current_contact->setText("#" + dst + current_contacts[dst]);
}


// 點擊後傳輸檔案
void MainWindow::onSendFileButtonClicked() {
    // 使用 QFileDialog 來選擇檔案
    QString filePath = QFileDialog::getOpenFileName(this, "Choose File", "", "All Files (*.*)");
    sendMessage("", ":/icon/avatar.png", "sent", "file", dst);

    if (!filePath.isEmpty()) {
        sendFileToServer(filePath);  // 呼叫檔案傳送函數
    }
}


// 傳輸檔案
void MainWindow::sendFileToServer(const QString &filePath) {
    QtConcurrent::run([=]() {
        QFile file(filePath);
        QFileInfo fileInfo(file);
        qint64 fileSize = fileInfo.size();  // 取得文件大小
        QString fileName = fileInfo.fileName();  // 取得檔案名稱
        qint64 offset = 0;

        QJsonObject json;
        json["type"] = "file_upload";
        json["file_name"] = fileName;
        json["file_size"] = fileSize;
        dst = getSelectedRowId();
        json["from"] = myid;
        json["to"] = dst;


        QJsonDocument doc(json);
        socket->write(doc.toJson(QJsonDocument::Compact));
        socket->waitForBytesWritten();


        while(!filesendmode){
            QThread::msleep(10);
        }

        QThread::msleep(1000);

        if (file.open(QIODevice::ReadOnly)) {
            const int chunkSize = 256*1024; // 每個區塊1KB
            while (offset < fileSize) {
                QByteArray chunk = file.read(chunkSize); // 讀取檔案塊
                socket->write(chunk);
                socket->waitForBytesWritten();  // 等待檔案區塊發送完畢
                offset += chunkSize;
                QThread::msleep(10);
            }
            file.close();
        }
        filesendmode = false;
    });
}

void MainWindow::recvFileFromServer(const QByteArray &byteArray) {

    //添加新數據到receiveBuffer，receiveBuffer在這邊充當queue的作用
    receiveBuffer.append(byteArray);

    const qint64 CHUNK_SIZE = 1024 * 1024; // 1MB

    while (!receiveBuffer.isEmpty()) {
        QFile file(recv_file_name);
        if (file.open(QIODevice::Append)) {
            qint64 bytesToWrite;

            if (receiveBuffer.size() >= CHUNK_SIZE) {
                bytesToWrite = CHUNK_SIZE;
            } else {
                // 處理最後的數據塊（小於 CHUNK_SIZE）
                bytesToWrite = receiveBuffer.size();
            }

            file.write(receiveBuffer.left(bytesToWrite));//取最前面的bytesToWrite寫入
            receiveBuffer = receiveBuffer.mid(bytesToWrite);//把bytesToWrite後的所有byte移到開頭
            offset += bytesToWrite;

            if (offset >= file_size) {
                // 文件接收完成
                filereceivemode = false;
                // 如果還有剩餘數據，可以在這裡清空
                receiveBuffer.clear();  // 可選的清空操作
                break;
            }
        }
        file.close();
    }
}
MainWindow::~MainWindow()
{
    delete ui;
}

