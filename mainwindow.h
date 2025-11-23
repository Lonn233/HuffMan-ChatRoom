#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QThread>
#include <QMutex>
#include <QString>
#include <QDateTime>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void on_btnSend_clicked();          // 发送按钮点击事件
    void on_txtInput_returnPressed();   // 输入框回车发送事件
    void slotDisconnected();            // 连接断开处理
    void slotConnectStatusChanged(bool isConnected);  // 连接状态变化处理

private:
    Ui::MainWindow* ui;
    QTcpSocket* m_tcpSocket;            // TCP通信socket
    bool m_isConnected;                 // 连接状态标记

    void sendMsg();                     // 发送消息核心逻辑
    void initUI();                      // 初始化界面样式
    void RecvMsg(const QString& time);  // 接收消息更新界面
};

#endif // MAINWINDOW_H
