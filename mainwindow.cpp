#include "mainwindow.h"
#include "ui_MainWindow.h"
#include <QMessageBox>
#include <QTextCursor>
#include <QStyleOption>
#include <QPainter>

// -------------------------- 接收线程（RecvThread）实现 --------------------------
RecvThread::RecvThread(QTcpSocket* socket, QObject* parent)
    : QThread(parent), m_socket(socket), m_running(true) {}

RecvThread::~RecvThread() {
    stop();
    wait();  // 等待线程完全退出，避免内存泄漏
}

void RecvThread::stop() {
    m_mutex.lock();
    m_running = false;  // 设置退出标记
    m_mutex.unlock();
}

void RecvThread::run() {
    while (true) {
        // 检查线程是否需要退出
        m_mutex.lock();
        bool running = m_running;
        m_mutex.unlock();
        if (!running) break;

        // 检查socket是否有效
        if (!m_socket || !m_socket->isValid()) break;

        // 接收服务器发送的「时间」（固定40字节，与服务器协议一致）
        if(!m_socket->waitForReadyRead(-1));        //阻塞接收
        QByteArray Data = m_socket->readAll();
        if (Data.isEmpty()) {
            qDebug("shit");
            // 数据为空 → 连接断开
            emit sigDisconnected();
            break;
        }
        QString timeStr = QString::fromUtf8(Data).trimmed();  // 去除补全的空格

        // 接收服务器发送的「消息内容」（固定1024字节）
        qDebug("noway");
        // 发送信号给主线程，更新界面（线程不能直接操作UI）
        emit sigRecvMsg(timeStr);
    }
}

// -------------------------- 主窗口（MainWindow）实现 --------------------------
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_isConnected(false) {
    ui->setupUi(this);
    initUI();  // 初始化界面样式

    // 初始化TCP socket
    m_tcpSocket = new QTcpSocket(this);

    // 连接服务器（127.0.0.1:8080，与Qt服务器端口一致）
    m_tcpSocket->connectToHost("127.0.0.1", 8080);

    // 连接成功信号：更新状态、启动接收线程
    connect(m_tcpSocket, &QTcpSocket::connected, [this]() {
        m_isConnected = true;
        slotConnectStatusChanged(true);
        ui->txtMsgDisplay->append("【系统提示】连接服务器成功！");

        // 创建并启动接收线程
        m_recvThread = new RecvThread(m_tcpSocket, this);
        connect(m_recvThread, &RecvThread::sigRecvMsg, this, &MainWindow::slotRecvMsg);
        //这里是检测服务器那边导致的断开连接
        connect(m_recvThread, &RecvThread::sigDisconnected, this, &MainWindow::slotDisconnected);
        m_recvThread->start();
    });

    // 连接断开信号
    //这里检测的是客户端试图主动关闭窗口导致的断开连接。
    connect(m_tcpSocket, &QTcpSocket::disconnected, this, &MainWindow::slotDisconnected);

    // 连接失败信号
    connect(m_tcpSocket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error), [this](QAbstractSocket::SocketError err) {
        m_isConnected = false;
        slotConnectStatusChanged(false);
        ui->txtMsgDisplay->append(QString("【错误】连接失败：%1").arg(m_tcpSocket->errorString()));
    });
}

MainWindow::~MainWindow() {
    // 资源释放：先停止线程，再断开连接
    if (m_recvThread) {
        m_recvThread->stop();
        delete m_recvThread;
    }
    m_tcpSocket->disconnectFromHost();
    delete ui;
}

// 初始化界面样式（美化+功能配置）
void MainWindow::initUI() {
    this->setWindowTitle("Qt 聊天客户端");

    // 消息显示区配置
    ui->txtMsgDisplay->setReadOnly(true);  // 只读，不能编辑
    ui->txtMsgDisplay->setStyleSheet("background-color: #F5F5F5; font-size: 14px; padding: 10px;");
    ui->txtMsgDisplay->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);  // 始终显示滚动条

    // 输入框配置
    ui->txtInput->setStyleSheet("font-size: 14px; padding: 5px;");
    ui->txtInput->setPlaceholderText("请输入消息，按回车或点击发送按钮发送...");

    // 发送按钮配置
    ui->btnSend->setStyleSheet("background-color: #4A90E2; color: white; font-size: 14px; padding: 6px 20px; border: none; border-radius: 4px;");
    ui->btnSend->setCursor(Qt::PointingHandCursor);  // 鼠标悬浮变手型

    // 状态标签配置
    ui->lblStatus->setStyleSheet("color: red; font-size: 12px;");
    ui->lblStatus->setText("状态：未连接");
}

// 发送按钮点击事件
void MainWindow::on_btnSend_clicked() {
    sendMsg();
}

// 输入框回车发送事件
void MainWindow::on_txtInput_returnPressed() {
    sendMsg();
}

// 发送消息核心逻辑
void MainWindow::sendMsg() {
    if (!m_isConnected) {
        QMessageBox::warning(this, "警告", "未连接服务器，无法发送消息！");
        return;
    }

    QString inputMsg = ui->txtInput->toPlainText().trimmed();
    if (inputMsg.isEmpty()) {
        QMessageBox::warning(this, "警告", "消息不能为空！");
        return;
    }

    // 发送消息（转成GBK编码，避免中文乱码）
    QByteArray sendData = inputMsg.toUtf8();
    qint64 ret = m_tcpSocket->write(sendData);
    if (ret > 0) {
        // 清空输入框并聚焦
        ui->txtInput->clear();
        ui->txtInput->setFocus();
    } else {
        QMessageBox::warning(this, "错误", "消息发送失败，请重试！");
    }
}

// 接收消息更新界面（主线程执行，安全操作UI）
void MainWindow::slotRecvMsg(const QString& time) {
    // 显示他人消息（蓝色时间+黑色内容）
    ui->txtMsgDisplay->append(QString("<span style='color: #4A90E2;'>%1</span> ")
                              .arg(time));

    // 自动滚动到最新消息
    QTextCursor cursor = ui->txtMsgDisplay->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->txtMsgDisplay->setTextCursor(cursor);
}

// 连接断开处理
void MainWindow::slotDisconnected() {
    m_isConnected = false;
    slotConnectStatusChanged(false);
    ui->txtMsgDisplay->append("【系统提示】与服务器断开连接！");

    // 停止接收线程
    if (m_recvThread) {
        m_recvThread->stop();
    }
}

// 连接状态变化：更新状态标签
void MainWindow::slotConnectStatusChanged(bool isConnected) {
    if (isConnected) {
        ui->lblStatus->setText("状态：已连接");
        ui->lblStatus->setStyleSheet("color: green; font-size: 12px;");
    } else {
        ui->lblStatus->setText("状态：未连接");
        ui->lblStatus->setStyleSheet("color: red; font-size: 12px;");
    }
}
