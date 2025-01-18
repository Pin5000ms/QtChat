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
#include <unordered_map>



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
    QStandardItemModel *chat_model;  // 用來儲存訊息的模型
    QStandardItemModel *contact_model;
    std::unordered_map<int, std::string> current_contacts;
    QTcpSocket *socket;
    QString getSelectedRowId();
    void addMessage(const QString &message, const QString &avatarPath, const QString type);


private slots:
    void on_msg_received();  // 槽函數
};
#endif // MAINWINDOW_H
