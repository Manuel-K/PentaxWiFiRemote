#ifndef REMOTE_H
#define REMOTE_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QJsonObject>

class QTimer;
class QNetworkReply;
class QNetworkAccessManager;

class Remote : public QObject
{
    Q_OBJECT

public:
    Remote();
    ~Remote();

    enum class PhotoSize {
        Thumb,
        View,
        Full
    };

    enum class Type {
        Shoot,
        Latest,
        Params,
        WiFiSettings,
        Props,
        Focus,
        FocusLock,
        PhotoList,
        PhotoInfo,
        PhotoThumb,
        PhotoView,
        PhotoFull
    };

    enum class ForceFormat {
        KeepFormat,
        DNG,
        JPG,
        PEF
    };

    Q_ENUM(Type)


    void get_props();
    void shoot(QString af);
    void set_parameter(const QString &type, const QString &value);
    void set_device_wifi_settings(const QString &ssid, const QString &key, const QString &channel);
    void green_button();
    void focus();
    void focus_at(int x, int y);
    void focus_adjust(const QString &arg);
    void fetch_photo(const QString &path, PhotoSize size, ForceFormat force_format = ForceFormat::KeepFormat);
    static QString generate_download_url(const QString &path, PhotoSize size, ForceFormat force_format = ForceFormat::KeepFormat);

    static const QString AFOn;
    static const QString AFOff;
    static const QString AFAuto;

public slots:
    void latest();
    void get_parameter();

private:


    QNetworkReply *request(const QString &url, const Type &type);
    QNetworkReply *request(const QString &url, const QByteArray &data, bool post, Type type);

    void parse_prop(QString type, const QJsonObject &json, bool as_int = false);
    void parse_param(QString type, const QJsonObject &json, bool use_label = false);


    static const QString camera_ip;
    static const QString url_shoot;
    static const QString url_latest;
    static const QString url_param;
    static const QString url_wifi_settings;
    static const QString url_props;
    static const QString url_photos;
    static const QString url_focus;

    QNetworkAccessManager *network_access_manager;
    QTimer *timer_latest;
    QTimer *timer_refresh_parameter;



private slots:
    void http_finished();




signals:
    void parameter_items(const QString &type, const QStringList &values);
    void parameter(const QString &type, const QString &value, bool use_label);
    void parameter_line_edit(const QString &type, const QString &value);
    void photo_info(const QString &path, const QJsonObject &json, bool latest);
    void photo_received(const QString &path, const QByteArray &data, PhotoSize size);
    void focus_info(bool focused, int center_x, int center_y, int eff_x, int eff_y);
    void wifi_settings_changed(bool success);
    void photo_formats(bool have_dng, bool have_jpg, bool have_pef);

};

#endif // REMOTE_H
