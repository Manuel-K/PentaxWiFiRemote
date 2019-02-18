#ifndef GALLERYITEM_H
#define GALLERYITEM_H

#include <QObject>
#include <QListWidgetItem>

class GalleryItem : public QListWidgetItem
{
public:
    GalleryItem(QString path);
    void set_thumbnail(const QPixmap &pm);
    void set_preview(const QPixmap &pm);
    void set_full(const QPixmap &pm);
    void parse_info(const QJsonObject &json);

private:
    void setBestIcon();

    QString path;
    QString info;
    QPixmap thumbnail;
    QPixmap preview;
    QPixmap full;

};

#endif // GALLERYITEM_H
