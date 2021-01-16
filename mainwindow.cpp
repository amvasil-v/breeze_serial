#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSerialPortInfo>
#include <QAction>
#include <QDebug>
#include <QTextEdit>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    socket = nullptr;
    auto ports = QSerialPortInfo::availablePorts();
    for(auto& port_info: ports) {
       auto *action = ui->menuSerial->addAction(port_info.portName());
       QObject::connect(action, &QAction::triggered, this, &MainWindow::slot_port_menu_clicked);
    }
}

MainWindow::~MainWindow()
{
    if (port.isOpen())
        port.close();
    server.close();
    delete ui;
}


void MainWindow::on_sendButton_clicked()
{
    if (!port.isOpen())
        return;
    QString line = ui->outEdit->text();
    line.append("\r\n");
    line.replace("\\r","\r");
    line.replace("\\n","\n");
    QByteArray raw = line.toLocal8Bit();
    writeToSerial(raw);

}

void MainWindow::slot_port_menu_clicked(bool)
{
    if (port.isOpen())
        return;
    auto *action = qobject_cast<QAction*>(QObject::sender());
    if (!action)
        return;
    QString port_name = action->text();
    ui->outText->append("Opening port " + port_name);
    port.setPortName(port_name);
    port.setBaudRate(QSerialPort::Baud115200);
    if (port.open(QIODevice::ReadWrite))
        ui->outText->append("Port opened\r\n");
    else {
        ui->outText->append("Failed to open port\r\n");
        return;
    }
    connect(&port, &QSerialPort::readyRead, this, &MainWindow::slot_port_ready_read);

}

void MainWindow::slot_port_ready_read()
{
    QByteArray data = port.readAll();
    ui->inText->insertPlainText(rawToString(data));
    ui->inText->ensureCursorVisible();
    if (socket && socket->isOpen()) {
        socket->write(data);
    }
}

void MainWindow::on_tcpButton_clicked()
{
    if (server.isListening())
        return;
    if (server.listen(QHostAddress::Any, 5555)) {
        ui->outText->append("TCP server started\r\n");
    }
    else {
        ui->outText->append("Failed to start TCP server\r\n");
        return;
    }
    connect(&server, &QTcpServer::newConnection, this, &MainWindow::slot_new_tcp_connection);
}

void MainWindow::slot_new_tcp_connection()
{
    ui->outText->append("New TCP connection\r\n");
    if (socket) {
        socket->close();
        socket->deleteLater();
    }
    socket = server.nextPendingConnection();
    if (!socket) {
        ui->outText->append("Connection failed\r\n");
        return;
    }
    connect(socket, &QTcpSocket::readyRead, this, &MainWindow::slot_tcp_ready_read);
    connect(socket, &QTcpSocket::disconnected, this, &MainWindow::slot_tcp_disconnected);
}

void MainWindow::slot_tcp_ready_read()
{
    QByteArray data = socket->readAll();
    ui->inText->insertPlainText("->: ");
    writeToSerial(data);
}

void MainWindow::slot_tcp_disconnected()
{
    ui->outText->append("TCP client disconnected\r\n");
}

void MainWindow::writeToSerial(const QByteArray &data)
{
    ui->outText->insertPlainText(rawToString(data));
    port.write(data);
    ui->outText->ensureCursorVisible();
    total_sent += static_cast<size_t>(data.size());
    ui->statusBar->showMessage("written " + QString::number(total_sent) + " bytes");


}

QString MainWindow::rawToString(const QByteArray &data)
{
    QString res;
    for (int i=0; i<data.size(); i++) {
        char c = data[i];
        if ((c >= '0' && c <= '9') ||
            (c >= 'a' && c<= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c == ' ' || c == '\r' || c == '\n') ||
            (c == '+' || c == '-' || c == '?' ||
             c == ',' || c == ';' || c == ':' ||
             c == '\'' || c == '\"' || c == '.')) {
            res.append(c);
        } else {
            res.append(QString("%1").arg((int)c, 2, 16, QChar('0')));
        }
    }
    return res;
}

void MainWindow::on_actionClear_triggered()
{
    ui->inText->clear();
    ui->outText->clear();
}

void MainWindow::on_actionCWJAP_triggered()
{
    writeToSerial("AT+CWJAP?\r\n");
}

void MainWindow::on_actionCIPCLOSE_triggered()
{
    writeToSerial("AT+CIPCLOSE\r\n");
}

void MainWindow::on_actionATE0_triggered()
{
    writeToSerial("ATE0\r\n");
}

void MainWindow::on_actionATE1_triggered()
{
    writeToSerial("ATE1\r\n");
}
