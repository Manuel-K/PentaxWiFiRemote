#ifndef GALLERYITEM_H
#define GALLERYITEM_H

#include <QObject>
#include <QListWidgetItem>

class GalleryItem : public QListWidgetItem
{
public:
    GalleryItem(QString path);
    void set_thumb(const QPixmap &pm);
    void set_prev(const QPixmap &pm);
    void set_full(const QPixmap &pm);
    void parse_info(const QJsonObject &json);
    const QString &get_path() const;
    const QPixmap &get_best_pixmap() const;
    bool has_view_or_full() const;
    bool has_full() const;

    static QString format_tv(const QString &str1);


private:
    void set_best_icon();

    QString path;
    QString info;
    QPixmap pm_thumb;
    QPixmap pm_view;
    QPixmap pm_full;

};

#endif // GALLERYITEM_H
