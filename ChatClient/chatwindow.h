#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <QWidget>
#include "usermanager.h"

class QTextEdit;
class QLineEdit;
class QPushButton;
class QLabel;

class ChatWindow : public QWidget
{
    Q_OBJECT
public:
    explicit ChatWindow(QWidget *parent = nullptr);

    // 设置当前用户信息
    void setMyInfo(int uid, QString name);

private slots:
    void onSendClicked();

private:
    void handleOneChatMsg(const QByteArray &data);

    QLabel *m_lblTitle;
    QTextEdit *m_txtHistory;
    
    QLineEdit *m_edtPeerId; // 对方ID
    QLineEdit *m_edtMsg;    // 消息输入框
    QPushButton *m_btnSend;
    
    int m_myUid;
    QString m_myName;
};

#endif // CHATWINDOW_H