#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include "usermanager.h"
#include "chatwindow.h"

class QLineEdit;
class QPushButton;

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onLoginClicked();
    void onRegClicked();

private:
    void handleLoginResponse(const QByteArray &data);
    void handleRegResponse(const QByteArray &data);

    QLineEdit *m_edtIp;
    QLineEdit *m_edtPort;
    QLineEdit *m_edtUser; // 同时也作为 ID 输入
    QLineEdit *m_edtPwd;
    
    QPushButton *m_btnLogin;
    QPushButton *m_btnReg;

    ChatWindow *m_chatWindow;
};
#endif // MAINWINDOW_H