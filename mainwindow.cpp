#include "mainwindow.h"
#include "ui_MainWindow.h"
#include <QMessageBox>
#include <QTextCursor>
#include <QStyleOption>
#include <QPainter>


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
        connect(m_tcpSocket,&QTcpSocket::readyRead,this,[this]()
        {
            if(m_tcpSocket->bytesAvailable()<=0)
                return;
            //注意收发两端文本要使用对应的编解码
            const QByteArray recv_byteArr=m_tcpSocket->readAll();
            const QString recv_text=QString::fromUtf8(recv_byteArr);
            RecvMsg(recv_text);
            qDebug()<<recv_text;
        });
    });

    // 连接断开信号
    //这里检测的是tcp连接是否中断，并终止客户端程序
    connect(m_tcpSocket, &QTcpSocket::disconnected, this, &MainWindow::slotDisconnected);

    // 连接失败信号
    connect(m_tcpSocket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error), [this](QAbstractSocket::SocketError err) {
        m_isConnected = false;
        slotConnectStatusChanged(false);
        ui->txtMsgDisplay->append(QString("【错误】连接失败：%1").arg(m_tcpSocket->errorString()));
    });
}

MainWindow::~MainWindow() {
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
void MainWindow::RecvMsg(const QString& msg) {
    // 显示他人消息（蓝色时间+黑色内容）
    ui->txtMsgDisplay->append(msg);
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

}

// 连接状态变化：更新状态标签
//刚开始，socket检查到连接上服务器后执行一次，socket连接断开时通过断开处理槽函数再执行一次。
void MainWindow::slotConnectStatusChanged(bool isConnected) {
    if (isConnected) {
        ui->lblStatus->setText("状态：已连接");
        ui->lblStatus->setStyleSheet("color: green; font-size: 12px;");
    } else {
        ui->lblStatus->setText("状态：未连接");
        ui->lblStatus->setStyleSheet("color: red; font-size: 12px;");
    }
}
