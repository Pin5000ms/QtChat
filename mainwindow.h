#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMainWindow>
#include <QListView>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QPushButton>
#include <QLineEdit>
#include <QTcpSocket>
#include <QHostAddress>
#include <QDebug>
#include <QPushButton>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QMessageBox>


#include <QFile>
#include <QFileInfo>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QFileDialog>
#include <QThread>
#include <QtConcurrent>

using namespace std;


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    //QStandardItemModel *chat_model;
    QString myid;
    QString dst;
    QMap<QString, QStandardItemModel*> chat_models; // 用來儲存訊息的模型
    QStandardItemModel *contact_model;
    QMap<QString, QString> current_contacts;
    QTcpSocket *socket;
    QString getSelectedRowId();
    QModelIndex sendMessage(const QString &message, const QString &avatarPath, const QString type, const QString datatype, QString to);
    QModelIndex recvMessage(const QString &message, const QString &avatarPath, const QString type, const QString datatype, QString from);

    //void setupDragAndDrop();
    //void dragEnterEvent(QDragEnterEvent *event);
    //void dropEvent(QDropEvent *event);
    void onSendFileButtonClicked();
    void sendFileToServer(const QString &filePath, QModelIndex index, QString to);
    void recvFileFromServer(const QByteArray &byteArray, QModelIndex index, QString from);
    bool filesendmode = false;
    bool filereceivemode = false;
    QString file_from;
    QModelIndex file_index;
    int file_size = 0;
    int offset = 0;
    QString recv_file_name;
    QByteArray receiveBuffer;

private slots:
    void on_received();  // 槽函數
    void on_sended();  // 槽函數
    void onContactsClicked();  // 槽函數
};
#endif // MAINWINDOW_H
