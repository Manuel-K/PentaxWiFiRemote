#include "ui/GalleryItem.h"

#include <QDebug>
#include <QJsonObject>

GalleryItem::GalleryItem(QString path)
{
    this->path = path;
    //setTextAlignment(Qt::AlignLeft);
    setText(path);
    setData(Qt::UserRole, path);
}


void GalleryItem::set_thumb(const QPixmap &pm)
{
    qDebug() << "GalleryItem::set_thumbnail";
    QIcon prev = icon();
    pm_thumb = pm;
    setIcon(pm_thumb);
    //setBestIcon();
}


void GalleryItem::set_prev(const QPixmap &pm)
{
    pm_view = pm;
    set_best_icon();
}


void GalleryItem::set_full(const QPixmap &pm)
{
    pm_view = pm;
    set_best_icon();
}


void GalleryItem::parse_info(const QJsonObject &json)
{
    //qDebug() << "GalleryItem::parse_info";
    //qDebug() << json;
    info = "f/" + json.value("av").toString() + " ";
    info.append(format_tv(json.value("tv").toString()) + " ");
    info.append("ISO" + json.value("sv").toString() + " ");
    const QString &xv = json.value("xv").toString();
    if (xv.toDouble() > 0) {
        info.append("+");
    }
    info.append(xv + "EV");
    setText(path + "\n" + info);
}


const QPixmap &GalleryItem::get_best_pixmap() const
{
    // Prefer largest of smaller versions.
    if (!pm_full.isNull()) {
        return pm_full;
    } else if (!pm_view.isNull()) {
        return pm_view;
    } else {
        return pm_thumb;
    }
}


bool GalleryItem::has_view_or_full() const
{
    //qDebug() << "GalleryItem::has_view_or_full()" << !pm_view.isNull() << !pm_full.isNull();
    return !pm_view.isNull() || !pm_full.isNull();
}


bool GalleryItem::has_full() const
{
    return !pm_full.isNull();
}


QString GalleryItem::format_tv(const QString &str1)
{
    int pos = str1.indexOf(".");
    const QString &numerator = str1.mid(0, pos);
    const QString &denominator = str1.mid(pos+1);
    //QString res;

    if (denominator == "1") {
            return QString("%1\"").arg(numerator);
    } else if (numerator == "1") {
        return QString("1/%1").arg(denominator);
    } else if (denominator == "10") {
        int size = numerator.size();
        return QString("%1.%2\"").arg(size > 1 ? numerator.mid(0, size-1) : "0", numerator.mid(size-1));
    }
    qDebug() << "GalleryItem::format_tv() format not implemented" << str1 << pos << numerator << denominator;
    return str1;
}


const QString &GalleryItem::get_path() const
{
    return path;
}


void GalleryItem::set_best_icon()
{
    // Prefer view over thumbnail and full.
    if (!pm_view.isNull()) {
        setIcon(pm_view);
    } else if (!pm_thumb.isNull()) {
        setIcon(pm_thumb);
    } else if (!pm_full.isNull()) {
        setIcon(pm_full);
    }
}
