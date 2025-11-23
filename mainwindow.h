#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QThread>
#include <QMutex>
#include <QString>
#include <QDateTime>

// 接收消息的子线程（独立线程，避免阻塞界面）
class RecvThread : public QThread {
    Q_OBJECT
public:
    explicit RecvThread(QTcpSocket* socket, QObject* parent = nullptr);
    ~RecvThread();
    void stop();  // 停止线程的接口

signals:
    // 收到消息后，发送信号给主线程更新界面
    void sigRecvMsg(const QString& time);
    // 连接断开信号
    void sigDisconnected();

protected:
    void run() override;  // 线程核心执行函数

private:
    QTcpSocket* m_socket;  // 通信socket（与主线程共享）
    bool m_running;        // 线程运行标记
    QMutex m_mutex;        // 互斥锁（保证线程安全）
};

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
    void slotRecvMsg(const QString& time);  // 接收消息更新界面
    void slotDisconnected();            // 连接断开处理
    void slotConnectStatusChanged(bool isConnected);  // 连接状态变化处理

private:
    Ui::MainWindow* ui;
    QTcpSocket* m_tcpSocket;            // TCP通信socket
    RecvThread* m_recvThread;           // 接收消息线程
    bool m_isConnected;                 // 连接状态标记

    void sendMsg();                     // 发送消息核心逻辑
    void initUI();                      // 初始化界面样式
};

#endif // MAINWINDOW_H
