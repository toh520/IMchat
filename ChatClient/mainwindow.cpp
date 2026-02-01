#include "mainwindow.h"
#include "public.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent), m_chatWindow(nullptr)
{
    setWindowTitle("IM 登录");
    resize(300, 250);

    QVBoxLayout *layout = new QVBoxLayout(this);

    // IP Port
    QHBoxLayout *netLayout = new QHBoxLayout();
    m_edtIp = new QLineEdit("127.0.0.1", this);
    m_edtPort = new QLineEdit("8888", this);
    netLayout->addWidget(new QLabel("IP:", this));
    netLayout->addWidget(m_edtIp);
    netLayout->addWidget(new QLabel("Port:", this));
    netLayout->addWidget(m_edtPort);
    layout->addLayout(netLayout);

    // User Pwd
    layout->addWidget(new QLabel("用户ID (Reg时填名字):", this));
    m_edtUser = new QLineEdit("1", this);
    layout->addWidget(m_edtUser); // [修复] 加入布局
    layout->addWidget(new QLabel("密码:", this));
    m_edtPwd = new QLineEdit("123456", this);
    m_edtPwd->setEchoMode(QLineEdit::Password);
    layout->addWidget(m_edtPwd);

    // Buttons
    QHBoxLayout *btnLayout = new QHBoxLayout();
    m_btnLogin = new QPushButton("登录", this);
    m_btnReg = new QPushButton("注册", this);
    btnLayout->addWidget(m_btnLogin);
    btnLayout->addWidget(m_btnReg);
    layout->addLayout(btnLayout);

    connect(m_btnLogin, &QPushButton::clicked, this, &MainWindow::onLoginClicked);
    connect(m_btnReg, &QPushButton::clicked, this, &MainWindow::onRegClicked);

    // 注册回调
    UserManager::instance().registerHandler(LOGIN_MSG_ACK, std::bind(&MainWindow::handleLoginResponse, this, std::placeholders::_1));
    UserManager::instance().registerHandler(REG_MSG_ACK, std::bind(&MainWindow::handleRegResponse, this, std::placeholders::_1));
}

MainWindow::~MainWindow()
{
}

void MainWindow::onLoginClicked()
{
    // 1. 连接服务器
    QString ip = m_edtIp->text();
    quint16 port = m_edtPort->text().toUShort();
    UserManager::instance().connectToServer(ip, port);

    // 2. 发送登录请求 (稍微延迟一点或者依靠 connect 成功的回调，但 QTcpSocket write 会 buffering，直接写没问题)
    chat::LoginRequest req;
    req.set_username(m_edtUser->text().toStdString()); // 暂时用 username 字段发 ID 字符串
    req.set_password(m_edtPwd->text().toStdString());

    std::string data;
    req.SerializeToString(&data);
    
    UserManager::instance().send(LOGIN_MSG, data);
}

void MainWindow::onRegClicked()
{
    QString ip = m_edtIp->text();
    quint16 port = m_edtPort->text().toUShort();
    UserManager::instance().connectToServer(ip, port);

    chat::RegRequest req;
    req.set_username(m_edtUser->text().toStdString()); // 注册时此处是名字
    req.set_password(m_edtPwd->text().toStdString());

    std::string data;
    req.SerializeToString(&data);

    UserManager::instance().send(REG_MSG, data);
}

void MainWindow::handleLoginResponse(const QByteArray &data)
{
    chat::LoginResponse resp;
    if (resp.ParseFromString(data.toStdString())) {
        if (resp.success()) {
            
            // 跳转到聊天界面
            // [Fix] 先初始化 ChatWindow，注册 MessageHandler。
            // 否则如果在 QMessageBox 阻塞期间收到离线消息，会导致
            // 1. 没有 Handler 被丢弃
            // 2. 或者积压在 Buffer 中直到下一条消息触发 readyRead
            if (!m_chatWindow) {
                m_chatWindow = new ChatWindow();
            }
            m_chatWindow->setMyInfo(resp.uid(), m_edtUser->text());
            m_chatWindow->show();
            this->hide();

            // 登录成功提示（可选，放在 show 之后体验更好，或者直接去掉）
            // QMessageBox::information(this, "成功", "登录成功");

        } else {
            QMessageBox::critical(this, "失败", QString::fromStdString(resp.msg()));
        }
    }
}

void MainWindow::handleRegResponse(const QByteArray &data)
{
    chat::RegResponse resp;
    if (resp.ParseFromString(data.toStdString())) {
        if (resp.success()) {
            QMessageBox::information(this, "成功", QString("注册成功，您的ID是: %1").arg(resp.uid()));
        } else {
            QMessageBox::critical(this, "失败", QString::fromStdString(resp.msg()));
        }
    }
}