#include "LiveView.h"

#include <QChar> //debug
#include <QFile> //debug


LiveView::LiveView()
{
    socket = new QTcpSocket(this);
    connect(socket, &QTcpSocket::connected, this, &LiveView::socket_connected);
    connect(socket, &QTcpSocket::disconnected, this, &LiveView::socket_disconnected);
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(socket_error(QAbstractSocket::SocketError)));
    connect(socket, &QTcpSocket::readyRead, this, &LiveView::socket_readyRead);
}


void LiveView::start(bool highres)
{
    if (highres) {
        resolution = "1080x720";
    } else {
        resolution = "720x480";
    }
    socket->connectToHost("192.168.0.1", 80);
}


void LiveView::stop()
{
    socket->disconnectFromHost();
}


void LiveView::simulate(const QString &filename, int count)
{
    qDebug() << "simulate LiveView";

    for (int i = 1; i <= count; i++) {
        QString path = QString(filename).arg(i, 2, 10, QChar('0'));
        //qDebug() << path;
        QFile debug_file(path);
        debug_file.open(QFile::ReadOnly);
        image_buffer.append(debug_file.readAll());
        handle_image_buffer();
    }
}


void LiveView::socket_connected()
{
    //qDebug() << "socket_connected()";
    QString requestLiveView = QString("GET /v1/liveview?reso=%1 HTTP/1.1\r\nhost: 192.168.0.1\r\n\r\n").arg(resolution);
    qDebug() << requestLiveView;
    QByteArray request;
    request.append(requestLiveView);
    socket->write(request);
}


void LiveView::socket_disconnected()
{
    //qDebug() << "socket_disconnected()";
    emit stopped();
}


void LiveView::socket_error(QAbstractSocket::SocketError socketError)
{
    qDebug() << "socket_error()" << socketError;
}


void LiveView::socket_readyRead()
{
    image_buffer.append(socket->readAll());
    handle_image_buffer();
}


void LiveView::handle_image_buffer() {
    // jpg start signature:
    static const QByteArray jpeg_start = QByteArrayLiteral("\xFF\xD8\xFF");
    // jpg end signature and http double line break:
    static const QByteArray jpeg_end   = QByteArrayLiteral("\xFF\xD9\r\n\r\n");


    int pos_st  = image_buffer.indexOf(jpeg_start);
    int pos_end = image_buffer.indexOf(jpeg_end);

    if (pos_st >= 0 && pos_end >= 0) {
        // contains both jpeg_start and jpeg_end:
        if (pos_st > pos_end) {
            qDebug() << "buffer contains invalid data";
            qDebug() << "dumping everything before first jpeg start marker";
            image_buffer = image_buffer.mid(pos_st);
        } else {
            QByteArray image_data = image_buffer.mid(pos_st, pos_end-pos_st+2);
            image_buffer = image_buffer.mid(pos_end+2);
            image_last.loadFromData(image_data);
            emit jpg_received(image_last);
        }
    }
}
