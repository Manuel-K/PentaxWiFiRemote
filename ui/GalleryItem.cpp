#include "ui/GalleryItem.h"

#include <QDebug>
#include <QJsonObject>

GalleryItem::GalleryItem(QString path)
{
    this->path = path;
    //setTextAlignment(Qt::AlignLeft);
    setText(path);
}


void GalleryItem::set_thumbnail(const QPixmap &pm)
{
    qDebug() << "GalleryItem::set_thumbnail";
    QIcon prev = icon();
    thumbnail = pm;
    setIcon(thumbnail);
    //setBestIcon();
}


void GalleryItem::set_preview(const QPixmap &pm)
{
    preview = pm;
    setBestIcon();
}


void GalleryItem::set_full(const QPixmap &pm)
{
    preview = pm;
    setBestIcon();
}


void GalleryItem::parse_info(const QJsonObject &json)
{
    //qDebug() << "GalleryItem::parse_info";
    //qDebug() << json;
    info = "f/" + json.value("av").toString() + " ";
    info.append(json.value("tv").toString() + "s ");
    info.append("ISO" + json.value("sv").toString() + " ");
    info.append("EV" + json.value("xv").toString());
    setText(path + "\n" + info);
}


void GalleryItem::setBestIcon()
{
    if (!thumbnail.isNull()) {
        setIcon(thumbnail);
    } else if (!preview.isNull()) {
        setIcon(preview);
    } else if (!full.isNull()) {
        setIcon(full);
    }
}
