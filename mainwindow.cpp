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
    writeToSerial(line);

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
    QString data = port.readAll();
    ui->inText->insertPlainText(data);
    ui->inText->ensureCursorVisible();
    if (socket && socket->isOpen()) {
        socket->write(data.toLocal8Bit());
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
    QString data = socket->readAll();
    ui->inText->insertPlainText("TCP: ");
    writeToSerial(data);
}

void MainWindow::slot_tcp_disconnected()
{
    ui->outText->append("TCP client disconnected\r\n");
}

void MainWindow::writeToSerial(QString &data)
{
    ui->outText->insertPlainText(data);
    port.write(data.toLocal8Bit());
    ui->outText->ensureCursorVisible();
}
