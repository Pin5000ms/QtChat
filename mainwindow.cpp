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


    //TitleBar
    setWindowFlags(Qt::FramelessWindowHint);
    setRoundedCorners(10);
    connect(ui->minimize, &QPushButton::clicked, this, &MainWindow::showMinimized);
    connect(ui->maximize, &QPushButton::clicked, this, &MainWindow::toggleMaximize);
    connect(ui->close, &QPushButton::clicked, this, &MainWindow::close);


    ui->contacts->setFocusPolicy(Qt::NoFocus);//移除ListView選中項目的虛線



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
        json["content"] = ui->clientName->text();

        JsonSend(json);
    });


    socket->connectToHost(QHostAddress("127.0.0.1"), 9527);



    // 發送按鈕點擊事件
    connect(ui->sendButton, &QPushButton::clicked, this, &MainWindow::on_sended);
    connect(ui->msgEdit, &CustomTextEdit::enterPressed, this, &MainWindow::on_sended);



    // TCP接收事件
    connect(socket, &QTcpSocket::readyRead, this, &MainWindow::on_received);

    //選擇聯絡人點擊事件
    connect(ui->contacts, &QListView::clicked, this, &MainWindow::onContactsClicked);

    //傳送檔案按鈕的點擊事件
    connect(ui->sendFileButton, &QPushButton::clicked, this, &MainWindow::onSendFileButtonClicked);



    // 在聊天室視窗中創建並設置自定義的委託，當中的每個Item會以委託類呈現
    MessageDelegate *delegate1 = new MessageDelegate(this);
    ui->chatroom->setItemDelegate(delegate1);
    ui->chatroom->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    //ui->chatroom->setModel(chat_model);// 設置 QListView 的模型


    // 在聯絡人清單中創建並設置自定義的委託，當中的每個Item會以委託類呈現
    ContactsDelegate *delegate2 = new ContactsDelegate(this);
    ui->contacts->setItemDelegate(delegate2);
    ui->contacts->setModel(contact_model);


}


void MainWindow::on_received()
{

    // 從 socket 讀取所有可用的數據
    QByteArray data = socket->readAll();


    if(filereceivemode){
        recvFileFromServer(data, file_index, file_from);
        return;
    }

    // 取得 JSON 對象
    QJsonObject jsonObj = JsonRecv(data);

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
    else if(type == MSG){
        QString from = jsonObj["from"].toString();
        QString message = jsonObj["content"].toString();
        dst = from;//收到來自from的訊息後，自動把下次傳送對象設定為from
        recvMessage(message, ":/icon/avatar.png", "receive", "text", from);
    }
    else if(type == "file_recv"){
        file_from = jsonObj["from"].toString();
        recv_file_name = jsonObj["file_name"].toString();
        file_size = jsonObj["file_size"].toInt();
        dst = file_from;//收到來自from的訊息後，自動把下次傳送對象設定為from
        file_index = recvMessage("", ":/icon/avatar.png", "receive", "file", file_from);

        // 彈出對話框詢問是否接收檔案
        QMessageBox msgBox;
        msgBox.setWindowTitle("File Receive");
        msgBox.setText(QString("File from %1 Accept?").arg(file_from));
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
            qDebug() << "User refused the file from" << file_from;
        }
    }
    else if(type == "file_upload_ack"){
        filesendmode = true;
    }
}


void MainWindow::on_sended(){

    //如果有選擇傳送對象，更新dst
    QString tmpdst = getSelectedContactId();
    if(tmpdst != "")
        dst = tmpdst;

    QJsonObject json;
    json["type"] = MSG;  // 類型為msg
    json["from"] = myid;
    json["to"] = dst; //發送到目的client
    json["content"] = ui->msgEdit->toPlainText();//訊息內容



    if (socket->state() == QAbstractSocket::ConnectedState && !myid.isEmpty()) {
        JsonSend(json);
        sendMessage(ui->msgEdit->toPlainText(), ":/icon/avatar.png", "sent", "text", dst);
    }
    else
        qDebug() << "Not connected to server!";


    ui->msgEdit->clear();//發送訊息後清空輸入欄
}



QModelIndex MainWindow::sendMessage(const QString &message, const QString &avatarPath, const QString type, const QString datatype, QString to)
{
    if(dst != "")
        ui->current_contact->setText( "#" + to + current_contacts[to] ); //標籤更新成聊天對象名稱

    // 設置自定義角色值
    QStandardItem *item = new QStandardItem();

    item->setData(message, MessageDelegate::TextRole);
    item->setData(QPixmap(avatarPath), Qt::DecorationRole);
    item->setData(type, MessageDelegate::DirectionTypeRole);
    item->setData(datatype, MessageDelegate::DataTypeRole);

    if(!chat_models.contains(to)){
        chat_models[to] = new QStandardItemModel(this);
    }

    ui->chatroom->setModel(chat_models[to]);


    // 添加到模型中
    chat_models[to]->appendRow(item);

    QModelIndex rowIndex = chat_models[to]->indexFromItem(item);//插入的row位置

    // 自動滾動到底部
    ui->chatroom->scrollTo(chat_models[to]->index(chat_models[to]->rowCount() - 1, 0), QAbstractItemView::PositionAtBottom);

    return rowIndex;
}

QModelIndex MainWindow::recvMessage(const QString &message, const QString &avatarPath, const QString type, const QString datatype, QString from)
{
    ui->current_contact->setText( "#" + from + current_contacts[from] );//標籤更新成聊天對象名稱


    // 設置自定義角色值
    QStandardItem *item = new QStandardItem();

    item->setData(message, MessageDelegate::TextRole);    // 設置消息內容
    item->setData(QPixmap(avatarPath), Qt::DecorationRole);  // 設置頭像
    item->setData(type, MessageDelegate::DirectionTypeRole);  // 設置消息類型：sent 或 receive
    item->setData(datatype, MessageDelegate::DataTypeRole);

    if(!chat_models.contains(from)){
        chat_models[from] = new QStandardItemModel(this);
    }

    ui->chatroom->setModel(chat_models[from]);

    chat_models[from]->appendRow(item);

    QModelIndex rowIndex = chat_models[from]->indexFromItem(item);//插入的row位置

    // 自動滾動到底部
    ui->chatroom->scrollTo(chat_models[from]->index(chat_models[from]->rowCount() - 1, 0), QAbstractItemView::PositionAtBottom);

    return rowIndex;
}


QString MainWindow::getSelectedContactId()
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

//切換聊天對象
void MainWindow::onContactsClicked(){
    dst = getSelectedContactId();
    ui->chatroom->setModel(chat_models[dst]);//切換聊天室
    ui->current_contact->setText("#" + dst + current_contacts[dst]);
}




// 點擊後傳輸檔案
void MainWindow::onSendFileButtonClicked() {

    // 使用 QFileDialog 來選擇檔案
    QString filePath = QFileDialog::getOpenFileName(this, "Choose File", "", "All Files (*.*)");

    if (!filePath.isEmpty()) {
        QModelIndex index = sendMessage("", ":/icon/avatar.png", "sent", "file", dst);
        sendFileToServer(filePath, index, dst);  // 呼叫檔案傳送函數
    }
}

// 傳輸檔案
void MainWindow::sendFileToServer(const QString &filePath, QModelIndex index, QString to) {
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
        dst = getSelectedContactId();
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


                // 更新模型中的進度
                int newProgressValue = offset*100/fileSize;
                chat_models[to]->setData(index, newProgressValue, MessageDelegate::ProgressRole);
                ui->chatroom->viewport()->update();


                QThread::msleep(10);
            }
            file.close();
        }
        filesendmode = false;
    });
}

// 接收檔案
void MainWindow::recvFileFromServer(const QByteArray &byteArray, QModelIndex index, QString from) {


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



            // 更新模型中的進度
            int newProgressValue = static_cast<float>(offset)*100/file_size;
            chat_models[from]->setData(index, newProgressValue, MessageDelegate::ProgressRole);
            ui->chatroom->viewport()->update();


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




void MainWindow::JsonSend(QJsonObject json){
    // 將 JSON 對象轉為byte array
    QJsonDocument doc(json);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    // 發送 JSON 數據
    socket->write(jsonData);
}

QJsonObject MainWindow::JsonRecv(QByteArray data)
{
    qDebug() << "Received data:" << data;

    // 解析 JSON 數據
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
    if (jsonDoc.isNull()) {
        qDebug() << "Failed to create JSON doc.";
        return QJsonObject();
    }

    // 檢查 JSON 是否為對象
    if (!jsonDoc.isObject()) {
        qDebug() << "JSON is not an object.";
        return QJsonObject();
    }

    return jsonDoc.object();
}

void MainWindow::toggleMaximize() {
    if (isMaximized()) {
        showNormal();  // 取消最大化
    } else {
        showMaximized();  // 最大化
    }
}

void MainWindow::setRoundedCorners(int radius) {
    if (isMaximized()) {
        this->clearMask(); // 最大化時移除圓角
    } else {
        QPainterPath path;
        path.addRoundedRect(this->rect(), radius, radius);
        QRegion mask(path.toFillPolygon().toPolygon());
        this->setMask(mask);
    }
}


void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);  // 呼叫基類方法
    setRoundedCorners(10);  // 重新設定圓角 (15px 可調整)
}

MainWindow::~MainWindow()
{
    delete ui;
}

