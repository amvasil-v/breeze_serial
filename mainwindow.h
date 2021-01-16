#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QTcpServer>
#include <QTcpSocket>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_sendButton_clicked();
    void slot_port_menu_clicked(bool);
    void slot_port_ready_read();
    void on_tcpButton_clicked();
    void slot_new_tcp_connection();
    void slot_tcp_ready_read();
    void slot_tcp_disconnected();

    void on_actionClear_triggered();

    void on_actionCWJAP_triggered();

    void on_actionCIPCLOSE_triggered();

    void on_actionATE0_triggered();

    void on_actionATE1_triggered();

private:
    Ui::MainWindow *ui;
    QSerialPort port;
    QTcpServer server;
    QTcpSocket *socket;
    size_t total_sent;

    void writeToSerial(const QByteArray& data);

    static QString rawToString(const QByteArray& data);

};
#endif // MAINWINDOW_H
