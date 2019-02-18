#include "Remote.h"


#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>

#include <QTimer>

const QString Remote::AFOn = "?af=on";
const QString Remote::AFOff = "?af=off";
const QString Remote::AFAuto = "?af=auto";


const QString Remote::camera_ip = "192.168.0.1";;
const QString Remote::url_shoot = "http://%1/v1/camera/shoot";
const QString Remote::url_latest = "http://%1/v1/photos/latest/info";
const QString Remote::url_wifi_settings = "http://%1/v1/params/device";
const QString Remote::url_param = "http://%1/v1/params/camera";
const QString Remote::url_props = "http://%1/v1/props";
const QString Remote::url_photos = "http://%1/v1/photos";
const QString Remote::url_focus = "http://%1/v1/lens/focus";


Remote::Remote() :
    network_access_manager(new QNetworkAccessManager(this)),
    timer_latest(new QTimer(this)),
    timer_refresh_parameter(new QTimer(this))
{
    connect(timer_latest, &QTimer::timeout, this, &Remote::latest);
    timer_latest->setInterval(250);
    connect(timer_refresh_parameter, &QTimer::timeout, this, &Remote::get_parameter);
    timer_refresh_parameter->setInterval(100);
    timer_refresh_parameter->setSingleShot(true);
}


Remote::~Remote()
{
    delete network_access_manager;
    delete timer_latest;
}


QNetworkReply* Remote::request(const QString &url, const Type &type)
{
    QNetworkReply *reply = network_access_manager->get(QNetworkRequest(QUrl(url)));

    reply->setProperty("type", QVariant::fromValue(type));

    connect(reply, &QNetworkReply::finished, this, &Remote::http_finished);
    return reply;
}


QNetworkReply* Remote::request(const QString &url, const QByteArray &data, bool post, Remote::Type type)
{
    QNetworkRequest req((QUrl(url)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QString("text/xml"));

    QNetworkReply *reply;
    if (post) {
        reply = network_access_manager->post(req, data);
        qDebug() << "post" << url << data;
    } else {
        reply = network_access_manager->put(req, data);
        qDebug() << "put" << url << data;
    }
    reply->setProperty("type", QVariant::fromValue(type));

    connect(reply, &QNetworkReply::finished, this, &Remote::http_finished);
    return reply;
}


void Remote::http_finished()
{
    //qDebug() << "http_finished";
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    QByteArray buffer = reply->readAll();
    if (buffer.size() == 0) {
        qDebug() << "no response";
        return;
    }

    Type type = reply->property("type").value<Type>();

    QJsonObject json = QJsonDocument::fromJson(buffer).object();

    switch (type) {
        case Type::Props:
        {
            qDebug() << "parsing props...";
            qDebug() << json;
            parse_prop("reso", json);
            //parse_prop("movieReso", json);
            parse_prop("WBMode", json);
            parse_prop("stillSize", json);
            //parse_prop("movieSize", json);
            parse_prop("shootMode", json);
            parse_prop("effect", json);
            parse_prop("filter", json);
            parse_prop("exposureMode", json);
            parse_prop("stillSize", json);
            parse_prop("av", json);
            parse_prop("tv", json);
            parse_prop("sv", json);
            parse_prop("xv", json);


            emit parameter("battery", QString("%1%").arg(json.value("battery").toInt()), true);
            emit parameter("hot", json.value("hot").toBool() ? tr("Yes") : tr("No"), true);
            parse_param("model", json, true);
            parse_param("firmwareVersion", json, true);
            emit parameter_line_edit("ssid", json.value("ssid").toString());
            emit parameter_line_edit("key", json.value("key").toString());
            parse_prop("channel", json, true);
            parse_param("channel", json);

            {
                const auto &storages = json.value("storages").toArray();
                QString remain;
                bool have_dng = false;
                bool have_jpg = false;
                bool have_pef = false;
                for (const auto &storage : storages) {
                    const auto &data = storage.toObject();
                    if (!data.value("writable").toBool()) {
                        continue;
                    }
                    if (remain.size() > 0) {
                        remain.append("/");
                    }
                    remain.append(QString("%1").arg(data.value("remain").toInt()));
                    const auto &format = data.value("format").toString();
                    if (format == "dng" || format == "rawdng") {
                        have_dng = true;
                    }
                    if (format == "jpeg" || format == "rawpef" || format == "rawdng") {
                        have_jpg = true;
                    }
                    if (format == "pef" || format == "rawpef") {
                        have_pef = true;
                    }
                    //qDebug() << format << have_dng << have_jpg << have_pef;
                }
                if (remain.size() == 0) {
                    remain.append("0");
                }
                emit parameter("remain", remain, true);
                emit photo_formats(have_dng, have_jpg, have_pef);
            }
        }
        //break;
        [[clang::fallthrough]];
        case Type::Params:
        {
            qDebug() << "parsing params...";
            qDebug() << json;
            parse_param("reso", json);
            //parse_param("movieReso", json);
            parse_param("WBMode", json);
            parse_param("stillSize", json);
            //parse_param("movieSize", json);
            parse_param("shootMode", json);
            parse_param("effect", json);
            parse_param("filter", json);
            parse_param("exposureMode", json);
            parse_param("stillSize", json);
            parse_param("av", json);
            parse_param("tv", json);
            parse_param("sv", json);
            parse_param("xv", json);
            parse_param("state", json, true);
            if (reply->property("refresh").toBool()) {
                qDebug() << "needs refresh";
                timer_refresh_parameter->start();
            }
        }
        break;
        case Type::Shoot:
        {
            qDebug() << "Type::Shoot";
            qDebug() << buffer;
            timer_latest->start();
        }
        //break;
        [[clang::fallthrough]];
        case Type::Focus:
        {
            qDebug() << "Type::Focus";
            //qDebug() << buffer;
            bool focused = json.value("focused").toBool();
            const QJsonArray &focusCenters = json.value("focusCenters").toArray().at(0).toArray(); //TODO: do not throw away any additional points
            int x = focusCenters.at(0).toInt(-1);
            int y = focusCenters.at(1).toInt(-1);
            const QJsonArray &focusEffectiveArea = json.value("focusEffectiveArea").toArray();
            int eff_x = focusEffectiveArea.at(0).toInt();
            int eff_y = focusEffectiveArea.at(1).toInt();
            emit focus_info(focused, x, y, eff_x, eff_y);
        }
        break;
        case Type::Latest:
        {
            qDebug() << "Type::Latest";
            if (json.value("captured").toBool() == true) {
                //qDebug() << buffer;
                timer_latest->stop();
                QString dir = json.value("dir").toString();
                QString file = json.value("file").toString();
                QString path = dir + "/" + file;
                //qDebug() << "new image" << path;
                emit photo_info(path, json, true);
            }
        }
        break;
        case Type::PhotoThumb:
        {
            qDebug() << "Type::PhotoThumb";
            const QString &path = reply->property("path").toString();
            emit photo_received(path, buffer, PhotoSize::Thumb);
        }
        break;
        case Type::PhotoView:
        {
            qDebug() << "Type::PhotoView";
            const QString &path = reply->property("path").toString();
            emit photo_received(path, buffer, PhotoSize::View);
        }
        break;
        case Type::PhotoFull:
        {
            qDebug() << "Type::PhotoView";
            const QString &path = reply->property("path").toString();
            emit photo_received(path, buffer, PhotoSize::Full);
        }
        break;
        case Type::WiFiSettings:
        {
            qDebug() << "Type::WifiSettings";
            emit wifi_settings_changed(json.value("errMsg").toString() == "OK");
        }
        break;
        default:
        {
            qDebug() << "default";
            qDebug() << buffer;
        }
        break;
    }
    reply->deleteLater();
}









void Remote::get_props()
{
    request(url_props.arg(camera_ip), Type::Props);
}


void Remote::shoot(QString af)
{
    //qDebug() << "shoot" << af;
    request(url_shoot.arg(camera_ip), af.toUtf8(), true, Type::Shoot);
}


void Remote::set_parameter(const QString &type, const QString &value)
{
    //qDebug() << "set_parameter" << type << value;
    QByteArray val("?");
    val.append(type);
    val.append("=");
    val.append(value);
    QNetworkReply *reply = request(url_param.arg(camera_ip), val, false, Type::Params);
    reply->setProperty("refresh", true);
}


void Remote::set_device_wifi_settings(const QString &ssid, const QString &key, const QString &channel)
{
    QString args = QString("?ssid=%1&key=%2&channel=%3").arg(ssid).arg(key).arg(channel);
    qDebug() << "DEBUG: not sending new wifi settings:" << args;
    request(url_wifi_settings.arg(camera_ip), args.toUtf8(), false, Type::WiFiSettings);
    //request(url_wifi_settings.arg(camera_ip), args.toUtf8(), true, Type::WiFiSettings); // DEBUG: messed up version to trigger error
}


void Remote::get_parameter()
{
    request(url_param.arg(camera_ip), Type::Params);
}


void Remote::green_button()
{
    //qDebug() << "green_button";
    request(url_param.arg(camera_ip), QByteArray(""), false, Type::Params);
}


void Remote::focus()
{
    //qDebug() << "focus";
    request(url_focus.arg(camera_ip), QByteArray(""), true, Type::Focus);
}


void Remote::focus_at(int x, int y)
{
    //qDebug() << "focus_at" << x << y;
    request(url_focus.arg(camera_ip), QString("?pos=%1,%2").arg(x).arg(y).toUtf8(), true, Type::Focus);
}


void Remote::focus_adjust(const QString &arg)
{
    request("http://192.168.0.1/v1/lens/focus/lock", arg.toUtf8(), false, Type::FocusLock);
}


void Remote::fetch_photo(const QString &path, Remote::PhotoSize size, ForceFormat force_format)
{
    Type type;
    const QString &url = generate_download_url(path, size, force_format);
    switch (size) {
        case PhotoSize::Thumb:
            type = Type::PhotoThumb;
        break;
        case PhotoSize::View:
            type = Type::PhotoView;
        break;
        case PhotoSize::Full:
            type = Type::PhotoFull;
        break;
    }
    qDebug() << url;
    QNetworkReply *reply = request(url, type);
    reply->setProperty("path", path);
}


QString Remote::generate_download_url(const QString &path, Remote::PhotoSize size, ForceFormat force_format)
{
    QString url = url_photos.arg(camera_ip) + "/";
    // NOTE: thumb only works for jpg, view and full work for jpg and dng
    switch (size) {
        case PhotoSize::Thumb:
        {
            url.append(path);
            url.append("?size=thumb");
        }
        break;
        case PhotoSize::View:
            url.append(path);
            url.append("?size=view");
        break;
        case PhotoSize::Full:
            url.append(path);
            //url.append("?size=full"); //default
        break;
    }

    if  (size == PhotoSize::Thumb) {
        force_format = ForceFormat::JPG;
    }
    switch (force_format) {
        case ForceFormat::KeepFormat:
        break;
        case ForceFormat::DNG:
            url.replace(".JPG", ".DNG");
            url.replace(".PEF", ".DNG");
        break;
        case ForceFormat::JPG:
            url.replace(".DNG", ".JPG");
            url.replace(".PEF", ".JPG");
        break;
        case ForceFormat::PEF:
            url.replace(".DNG", ".PEF");
            url.replace(".JPG", ".PEF");
        break;
    }
    return url;
}


void Remote::latest()
{
    //qDebug() << "latest";
    request(url_latest.arg(camera_ip), Type::Latest);
}


void Remote::parse_prop(QString type, const QJsonObject &json, bool as_int)
{
    const QJsonArray &array = json.value(type+"List").toArray();
    QStringList sl;

    if (as_int) {
        for (const auto &item : array) {
            sl.append(QString("%1").arg(item.toInt()));
        }
    } else {
        for (const auto &item : array) {
            sl.append(item.toString());
        }
    }
    emit parameter_items(type, sl);
}


void Remote::parse_param(QString type, const QJsonObject &json, bool use_label)
{
    emit parameter(type, json.value(type).toString(), use_label);
}


