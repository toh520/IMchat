#include "chatwindow.h"
#include "public.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QDateTime>

ChatWindow::ChatWindow(QWidget *parent) : QWidget(parent)
{
    // UI 布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 标题
    m_lblTitle = new QLabel("未登录", this);
    mainLayout->addWidget(m_lblTitle);

    // 历史消息区
    m_txtHistory = new QTextEdit(this);
    m_txtHistory->setReadOnly(true);
    mainLayout->addWidget(m_txtHistory);

    // 发送区
    QHBoxLayout *inputLayout = new QHBoxLayout();
    
    inputLayout->addWidget(new QLabel("对方ID:", this));
    m_edtPeerId = new QLineEdit(this);
    m_edtPeerId->setPlaceholderText("ID");
    m_edtPeerId->setFixedWidth(100);
    inputLayout->addWidget(m_edtPeerId);

    m_edtMsg = new QLineEdit(this);
    m_edtMsg->setPlaceholderText("请输入消息...");
    inputLayout->addWidget(m_edtMsg);

    m_btnSend = new QPushButton("发送", this);
    inputLayout->addWidget(m_btnSend);

    mainLayout->addLayout(inputLayout);

    resize(600, 400);

    // 连接信号
    connect(m_btnSend, &QPushButton::clicked, this, &ChatWindow::onSendClicked);

    // 注册单个聊天消息的回调
    UserManager::instance().registerHandler(ONE_CHAT_MSG, std::bind(&ChatWindow::handleOneChatMsg, this, std::placeholders::_1));
}

void ChatWindow::setMyInfo(int uid, QString name)
{
    m_myUid = uid;
    m_myName = name;
    m_lblTitle->setText(QString("我是: %1 (ID: %2)").arg(name).arg(uid));
    setWindowTitle(QString("ChatClient - %1").arg(uid));
}

void ChatWindow::onSendClicked()
{
    QString peerIdStr = m_edtPeerId->text();
    QString msg = m_edtMsg->text();

    if (peerIdStr.isEmpty() || msg.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入对方ID和消息内容");
        return;
    }

    int peerId = peerIdStr.toInt();

    // 构造请求
    chat::OneChatRequest req;
    req.set_from_id(m_myUid);
    req.set_to_id(peerId);
    req.set_msg(msg.toStdString());

    std::string data;
    req.SerializeToString(&data);

    // 发送
    UserManager::instance().send(ONE_CHAT_MSG, data);

    // 自己界面显示
    QString time = QDateTime::currentDateTime().toString("MM-dd hh:mm:ss");
    m_txtHistory->append(QString("[%1] 我 对 %2 说: %3").arg(time).arg(peerId).arg(msg));
    
    m_edtMsg->clear();
}

void ChatWindow::handleOneChatMsg(const QByteArray &data)
{
    std::string str = data.toStdString();
    chat::OneChatRequest req;
    if (req.ParseFromString(str)) {
        // UI 更新必须在主线程，这里实际上是在 Socket 的 readyRead 槽函数里调用的，是主线程，没问题
        QString time = QDateTime::currentDateTime().toString("MM-dd hh:mm:ss");
        QString msg = QString::fromStdString(req.msg());
        
        m_txtHistory->append(QString("[%1] 用户%2 说: %3").arg(time).arg(req.from_id()).arg(msg));
    }
}