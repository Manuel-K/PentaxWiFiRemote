#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "Remote.h"

//#define ENABLE_DEBUG_CONTEXTMENU

class LiveView;
//class Remote;
//class PreviewGallery;
class GalleryItem;
class QListWidgetItem;

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
    void preview_gallery_display_item(bool force_full);
    int find_minimum_resolution(const QStringList &res);

    Ui::MainWindow *ui;
    LiveView *liveview;
    Remote *remote;
    //PreviewGallery *preview_gallery;
    QHash<QString, GalleryItem*> preview_gallery_storage;

    bool ignore_liveview;
    bool wifi_settings_locked;
    bool photo_format_available_dng;
    bool photo_format_available_jpg;
    bool photo_format_available_pef;

private slots:
    void handle_liveview_jpg_received(const QPixmap &image);
    void handle_liveview_stopped();
    void handle_parameter_items(const QString &type, const QStringList &values);
    void handle_parameter(const QString type, const QString value, bool use_label);
    void handle_parameter_line_edit(const QString type, const QString value);
    void handle_photo_info(const QString &path, const QJsonObject &json, bool latest);
    void handle_photo_received(const QString &path, const QByteArray &data, Remote::PhotoSize size);
    void handle_focus_info(bool focused, int center_x, int center_y, int eff_x, int eff_y);
    void handle_wifi_settings_changed(bool success);
    void handle_photo_formats(bool have_dng, bool have_jpg, bool have_pef);

    void handle_parameter_cb_current_text_changed(const QString &value);
    void on_cb_liveview_toggled(bool state);
    void on_pb_shoot_clicked();
    void on_pb_green_clicked();
    void on_pb_focus_clicked();
#if 0
    void on_pb_debug_clicked();
#endif
    void on_preview_clicked(int x, int y);
    void on_preview_gallery_itemDoubleClicked(); // aka preview_gallery_cmaction_display_item
    void on_preview_customContextMenuRequested(const QPoint &pos);
    void preview_cmaction_toggle_ignore_liveview();
    void preview_cmaction_toggle_enlarge();
    void on_preview_gallery_customContextMenuRequested(const QPoint &pos);
    void preview_gallery_cmaction_copy_urls_to_clipboard(Remote::ForceFormat force_format = Remote::ForceFormat::KeepFormat);
    void preview_gallery_cmaction_display_item_full();
    void on_sa_settings_customContextMenuRequested(const QPoint &pos);
    void sa_settings_cmaction_toggle_wifi_settings_lock();
    void sa_settings_cmaction_update_wifi_settings();

};

#endif // MAINWINDOW_H
