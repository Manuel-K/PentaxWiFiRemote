#ifndef LIVEVIEW_H
#define LIVEVIEW_H

#include <QObject>
#include <QTcpSocket>
#include <QPixmap>

class LiveView : public QObject
{
    Q_OBJECT

public:
    LiveView();
    void start(bool highres = false);
    void stop();
    void simulate(const QString &filename, int count);

private slots:
    void socket_connected();
    void socket_disconnected();
    void socket_error(QAbstractSocket::SocketError socketError);
    void socket_readyRead();

private:
    void handle_image_buffer();

    QString resolution;
    QTcpSocket *socket;
    QByteArray image_buffer;
    QPixmap image_last;


signals:
    //void received_jpg(const QIcon &icon);
    void jpg_received(const QPixmap &image);
    void stopped();

};

#endif // LIVEVIEW_H