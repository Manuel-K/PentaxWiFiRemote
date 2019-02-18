#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "Remote.h"

class LiveView;
//class Remote;
//class PreviewGallery;
class GalleryItem;

namespace Ui {
class MainWindow;
}



class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    LiveView *liveview;
    Remote *remote;
    //PreviewGallery *preview_gallery;
    QHash<QString, GalleryItem*> preview_gallery_storage;


private slots:
    void handle_liveview_jpg_received(const QPixmap &image);
    void handle_parameter_items(const QString type, const QStringList values);
    void handle_parameter(const QString type, const QString value, bool read_only);
    void handle_photo_info(const QString &path, const QJsonObject &json, bool latest);
    void handle_photo_received(const QString &path, const QByteArray &data, Remote::PhotoSize size);
    void handle_focus_info(bool focused, int center_x, int center_y, int eff_x, int eff_y);

    void handle_parameter_cb_current_text_changed(const QString &value);
    void on_cb_liveview_toggled(bool state);
    void on_pb_shoot_clicked();
    void on_pb_green_clicked();
    void on_pb_focus_clicked();
    void on_pb_debug_clicked();
    void on_preview_clicked(int x, int y);


};

#endif // MAINWINDOW_H
