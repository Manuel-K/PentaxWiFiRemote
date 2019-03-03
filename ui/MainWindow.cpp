#include "ui/MainWindow.h"
#include "ui_MainWindow.h"


#include "LiveView.h"
#include "Remote.h"
//#include "ui/PreviewGallery.h"
#include "ui/GalleryItem.h"
//#include "ui/PreviewGalleryItemDelegate.h"

#include <QDebug>
#include <QClipboard>
#include <QMenu>
#include <QInputDialog>
#include <QMessageBox>

#if 0
#include <QFile> //debug
#include <QTextStream> //debug
#endif

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    liveview(new LiveView),
    remote(new Remote)//,
    //preview_gallery(new PreviewGallery)
{
    ui->setupUi(this);

    ui->c_autoFocus->addItem("On", Remote::AFOn);
    ui->c_autoFocus->addItem("Off", Remote::AFOff);
    ui->c_autoFocus->addItem("Auto", Remote::AFAuto);

    //PreviewGalleryItemDelegate *delegate = new PreviewGalleryItemDelegate(ui->preview_gallery);

    //QStyledItemDelegate *delegate = new QStyledItemDelegate(ui->preview_gallery);
    //ui->preview_gallery->setItemDelegate(delegate);
    ui->preview_gallery->setDragEnabled(false); // does not seem to work in designer

    connect(liveview, &LiveView::jpg_received, this, &MainWindow::handle_liveview_jpg_received);
    connect(liveview, &LiveView::stopped, this, &MainWindow::handle_liveview_stopped);

    connect(remote, &Remote::parameter_items, this, &MainWindow::handle_parameter_items);
    connect(remote, &Remote::parameter, this, &MainWindow::handle_parameter);
    connect(remote, &Remote::parameter_line_edit, this, &MainWindow::handle_parameter_line_edit);
    connect(remote, &Remote::photo_info, this, &MainWindow::handle_photo_info);
    connect(remote, &Remote::photo_received, this, &MainWindow::handle_photo_received);
    connect(remote, &Remote::focus_info, this, &MainWindow::handle_focus_info);
    connect(remote, &Remote::wifi_settings_changed, this, &MainWindow::handle_wifi_settings_changed);
    connect(remote, &Remote::photo_formats, this, &MainWindow::handle_photo_formats);

    remote->get_props();

    //connect(ui->c_reso, &QComboBox::currentTextChanged, this, &MainWindow::handle_parameter_cb_current_text_changed); // no event handler
    connect(ui->c_WBMode, &QComboBox::currentTextChanged, this, &MainWindow::handle_parameter_cb_current_text_changed);
    connect(ui->c_stillSize, &QComboBox::currentTextChanged, this, &MainWindow::handle_parameter_cb_current_text_changed);
    connect(ui->c_shootMode, &QComboBox::currentTextChanged, this, &MainWindow::handle_parameter_cb_current_text_changed);
    connect(ui->c_effect, &QComboBox::currentTextChanged, this, &MainWindow::handle_parameter_cb_current_text_changed);
    connect(ui->c_filter, &QComboBox::currentTextChanged, this, &MainWindow::handle_parameter_cb_current_text_changed);
    connect(ui->c_exposureMode, &QComboBox::currentTextChanged, this, &MainWindow::handle_parameter_cb_current_text_changed);
    connect(ui->c_stillSize, &QComboBox::currentTextChanged, this, &MainWindow::handle_parameter_cb_current_text_changed);
    connect(ui->c_av, &QComboBox::currentTextChanged, this, &MainWindow::handle_parameter_cb_current_text_changed);
    connect(ui->c_tv, &QComboBox::currentTextChanged, this, &MainWindow::handle_parameter_cb_current_text_changed);
    connect(ui->c_sv, &QComboBox::currentTextChanged, this, &MainWindow::handle_parameter_cb_current_text_changed);
    connect(ui->c_xv, &QComboBox::currentTextChanged, this, &MainWindow::handle_parameter_cb_current_text_changed);

    ignore_liveview = false;
    wifi_settings_locked = true;
    photo_format_available_dng = false;
    photo_format_available_jpg = false;
    photo_format_available_pef = false;
}


MainWindow::~MainWindow()
{
    delete ui;
    liveview->stop();
    delete liveview;
    delete remote;
    //delete preview_gallery;

    // TODO: delete contents of preview_gallery_storage
}


void MainWindow::handle_liveview_jpg_received(const QPixmap &image)
{
    //qDebug() << "handle_received_jpg_liveview";
    if (!ignore_liveview) {
        ui->preview->setPixmap(image, true);
    }
}


void MainWindow::handle_liveview_stopped()
{
    // FIXME: This was intended to catch if the camera is switched off during LiveView.
    //        It does not do that. Instead if is triggered if LiveView is switched off
    //        by the program.
    //qDebug() << "handle_liveview_stopped";
    //ui->cb_liveview->setChecked(false);
}


void MainWindow::handle_parameter_items(const QString &type, const QStringList &values)
{
    if (!wifi_settings_locked && type == "channel") {
        qDebug() << "ignoring handle_parameter_items for channel";
        return;
    }

    QComboBox *cb = this->findChild<QComboBox*>("c_"+type);
    bool prev = cb->blockSignals(true);
    cb->clear();

    if (values.size() > 0) {
        if (type != "channel" && type != "exposureMode" && type != "shootMode") {
            cb->setEnabled(true);
            cb->setEditable(false);
        }

        if (type == "tv") {
            for (const auto &item : values) {
                cb->addItem(GalleryItem::format_tv(item), item);
            }
        } else {
            cb->addItems(values);
        }

        //qDebug() << values.at(0);

        if (type == "reso") {
            cb->setCurrentIndex(find_minimum_resolution(values));
        }
    } else if (type != "channel") {
        cb->setEnabled(false);
        cb->setEditable(true);
    }

    cb->blockSignals(prev);
}


void MainWindow::handle_parameter(const QString type, const QString value, bool use_label)
{
    if (!wifi_settings_locked && type == "channel") {
        qDebug() << "ignoring handle_parameter for channel";
        return;
    }
    //qDebug() << "handle_parameter" << type << value << read_only;
    if (use_label) {
        QLabel *l = this->findChild<QLabel*>("l_" + type);
        l->setText(value);
    } else {
        QComboBox *cb = this->findChild<QComboBox*>("c_"+type);
        bool prev = cb->blockSignals(true);
        //int index = cb->findText(value);
        if (type == "tv") {
            cb->setCurrentText(GalleryItem::format_tv(value));
        } else {
            cb->setCurrentText(value);
        }
        //qDebug() << values.at(0);
        cb->blockSignals(prev);
        if (type == "channel") {
            cb->setProperty("previous_value", value);
        }
    }
}


void MainWindow::handle_parameter_line_edit(const QString type, const QString value)
{
    if (!wifi_settings_locked && (type == "key" || type == "ssid")) {
        return;
    }
    QLineEdit *le = this->findChild<QLineEdit*>("le_" + type);
    le->setText(value);
    if (type == "key" || type == "ssid") {
        le->setPlaceholderText(value);
    }
}


void MainWindow::handle_photo_info(const QString &path, const QJsonObject &json, bool latest)
{
    qDebug() << "handle_photo_info" << path << latest;
    //qDebug() << json;
    GalleryItem *item;
    if (latest) {
        if (!preview_gallery_storage.contains(path)) {
            qDebug() << "Parsing info for new image";
            item = new GalleryItem(path);
            //item->parse_info(json);
            if (latest) {
                preview_gallery_storage.insert(path, item);
                ui->preview_gallery->addItem(item);
            }
        } else {
            qDebug() << "Parsing info for existing image";
            item = preview_gallery_storage.value(path);
            //item->parse_info(json);
        }
        item->parse_info(json);
        if (photo_format_available_jpg && ui->cb_liveview->isChecked() && !ignore_liveview) {
            remote->fetch_photo(path, Remote::PhotoSize::Thumb);
        } else {
            remote->fetch_photo(path, Remote::PhotoSize::View);
        }
    }else {
        qDebug() << "Normal info receiving not implemented";
    }
}


void MainWindow::handle_photo_received(const QString &path, const QByteArray &data, Remote::PhotoSize size)
{
    ui->preview_gallery->setUpdatesEnabled(true);
    qDebug() << "handle_photo_received" << path;
    GalleryItem *item;
    if (preview_gallery_storage.contains(path)) {
        item = preview_gallery_storage.value(path);
    } else {
        qDebug() << "Created new item without info"; //TODO
        item = new GalleryItem(path);
    }
    QPixmap pm;
    pm.loadFromData(data);
    switch(size) {
        case Remote::PhotoSize::Thumb:
            item->set_thumb(pm);
        break;
        case Remote::PhotoSize::View:
            item->set_prev(pm);
            if (!ui->cb_liveview->isChecked() || ignore_liveview) {
                ui->preview->setPixmap(pm, false);
            }
        break;
        case Remote::PhotoSize::Full:
            item->set_full(pm);
            if (!ui->cb_liveview->isChecked() || ignore_liveview) {
                ui->preview->setPixmap(pm, false);
            }
        break;
    }
}


void MainWindow::handle_focus_info(bool focused, int center_x, int center_y, int eff_x, int eff_y)
{
    qDebug() << "handle_set_focus" << focused << center_x << center_y << eff_x << eff_y;
    // TODO: display focused value somewhere in MainWindow
    ui->preview->set_focus(focused, center_x, center_y, eff_x, eff_y);
}


void MainWindow::handle_wifi_settings_changed(bool success)
{
    //qDebug() << "handle_wifi_settings_changed" << success;
    if (success) {
        if (!wifi_settings_locked) {
            sa_settings_cmaction_toggle_wifi_settings_lock();
        }
        QMessageBox::information(this, tr("Success"), tr("The WiFi settings have been changed.\nReconnect to the camera and restart the programm."));
    } else {
        QMessageBox::information(this, tr("Error"), tr("The WiFi settings have not been changed."));
    }
}


void MainWindow::handle_photo_formats(bool have_dng, bool have_jpg, bool have_pef)
{
    qDebug() << "handle_photo_formats: dng:" << have_dng << "   jpg:" << have_jpg << "   pef:" << have_pef;
    photo_format_available_dng = have_dng;
    photo_format_available_jpg = have_jpg;
    photo_format_available_pef = have_pef;
}


void MainWindow::handle_parameter_cb_current_text_changed(const QString &value)
{
    QComboBox* s = static_cast<QComboBox*>(sender());
    QString type = s->objectName();
    type.remove(0, 2);

    //qDebug() << "handle_parameter_cb_current_text_changed" << id << value;
    if (type == "tv") {
        remote->set_parameter(type, s->currentData().toString());
    } else {
        remote->set_parameter(type, value);
    }
}


void MainWindow::on_cb_liveview_toggled(bool state)
{
    qDebug() << "on_cb_liveview_toggled" << state;
    if (state) {
        if (!ui->c_reso->currentText().contains("x")) {
            qDebug() << "No valid resolution selected. Using fallback.";
            liveview->start();
        } else {
            liveview->start_res(ui->c_reso->currentText());
        }
        ignore_liveview = false;
    } else {
        liveview->stop();
    }
}


void MainWindow::on_pb_shoot_clicked()
{
    //qDebug() << "on_pb_shoot_clicked";
    remote->shoot(ui->c_autoFocus->currentData().toString());
}


void MainWindow::on_pb_green_clicked()
{
    qDebug() << "on_pb_green_clicked";
    remote->green_button();
}


void MainWindow::on_pb_focus_clicked()
{
    qDebug() << "on_pb_focus_clicked";
    remote->focus();
}


#if 0
void MainWindow::on_pb_debug_clicked()
{
    qDebug() << "on_pb_debug_clicked" << ui->le_focus_adjust->text();
    QFile log("log.txt");
    log.open(QFile::WriteOnly | QFile::Append);
    QTextStream out(&log);
    out << ui->le_focus_adjust->text();
    out << "\n";
    log.close();
    remote->focus_adjust(ui->le_focus_adjust->text());
}
#endif


void MainWindow::on_preview_clicked(int x, int y)
{
    //qDebug() << "on_preview_clicked" << x << y;
    // focus at position only works if LiveView is enabled
    if (ui->cb_liveview->isChecked()) {
        if (ignore_liveview) {
            ignore_liveview = false;
        } else {
            int pos = ui->c_autoFocus->findData(Remote::AFOff);
            ui->c_autoFocus->setCurrentIndex(pos);
            remote->focus_at(x, y);
        }
    } else {
        qDebug() << "Can only focus at position if LiveView is enabled.";
    }
}


void MainWindow::preview_gallery_display_item(bool force_full)
{
    GalleryItem *gi = static_cast<GalleryItem*>(ui->preview_gallery->currentItem());
    qDebug() << "preview_gallery_display_item" << gi->get_path() << "force_full:" << force_full;
    if (ui->cb_liveview->isChecked()) {
        ignore_liveview = true;
    }
    ui->preview->setPixmap(gi->get_best_pixmap(), false);

    if (!force_full && !gi->has_view_or_full()) {
        //qDebug() << "fetching view size image";
        remote->fetch_photo(gi->get_path(), Remote::PhotoSize::View);
    } else if (force_full && !gi->has_full()) {
        //qDebug() << "fetching full size image" << force_full;
        remote->fetch_photo(gi->get_path(), Remote::PhotoSize::Full, Remote::ForceFormat::JPG);
    }
}


int MainWindow::find_minimum_resolution(const QStringList &res)
{
    int min_index = -1;
    int min_value = 1000000000;
    for (const auto &item : res) {
        if (!item.contains("x")) {
            continue;
        }
        int pos = item.indexOf("x");
        int pixels = item.mid(0, pos).toInt() * item.mid(pos+1).toInt();
        if (pixels > 0 && pixels < min_value) {
            min_value = pixels;
            min_index = res.indexOf(item);
        }

        qDebug() << item << pos << pixels << res.indexOf(item);
    }
    return min_index;
}


void MainWindow::on_preview_gallery_itemDoubleClicked()
{
    preview_gallery_display_item(false);
}


void MainWindow::on_preview_customContextMenuRequested(const QPoint &pos)
{
    //qDebug() << "on_preview_customContextMenuRequested";
    QMenu submenu(ui->preview);
    submenu.addAction(tr("Shoot"), this, &MainWindow::on_pb_shoot_clicked);
    if (ignore_liveview) {
        submenu.addAction(tr("Stop ignoring LiveView"), this, &MainWindow::preview_cmaction_toggle_ignore_liveview);
    } else {
        submenu.addAction(tr("Start ignoring LiveView"), this, &MainWindow::preview_cmaction_toggle_ignore_liveview);
    }
    if (!ui->preview->get_allow_enlarge()) {
        submenu.addAction(tr("Allow enlargement of images"), this, &MainWindow::preview_cmaction_toggle_enlarge);
    } else {
        submenu.addAction(tr("Never enlarge images"), this, &MainWindow::preview_cmaction_toggle_enlarge);
    }
    submenu.addAction(tr("Fetch latest photo"), remote, &Remote::latest);
    submenu.addSection(tr("Rotation"));
    const PreviewWidget::Rotation &rot = ui->preview->get_rotation();
    if (rot != PreviewWidget::Rotation::None) {
        submenu.addAction(tr("Reset rotation"), this, [=]() { ui->preview->set_rotation(); });
    }
    if (rot != PreviewWidget::Rotation::Rotate90) {
        submenu.addAction(tr("Rotate 90"), this, [=]() { ui->preview->set_rotation(PreviewWidget::Rotation::Rotate90); });
    }
    if (rot != PreviewWidget::Rotation::Rotate180) {
        submenu.addAction(tr("Rotate upside down"), this, [=]() { ui->preview->set_rotation(PreviewWidget::Rotation::Rotate180); });
    }
    if (rot != PreviewWidget::Rotation::Rotate270) {
        submenu.addAction(tr("Rotate 270"), this, [=]() { ui->preview->set_rotation(PreviewWidget::Rotation::Rotate270); });
    }

#ifdef ENABLE_DEBUG_CONTEXTMENU
    submenu.addSection("DEBUG");
    submenu.addAction("Add dummy gallery items");
    submenu.addAction("Simulate LiveView small");
    submenu.addAction("Simulate LiveView large");
    submenu.addAction("Test force_format in Remote::generate_download_url()");
    submenu.addAction("Test TV formatter");
    submenu.addAction("Test resolution check");

    QAction *ac =
#endif //ENABLE_DEBUG_CONTEXTMENU
    submenu.exec(ui->preview->mapToGlobal(pos));
#ifdef ENABLE_DEBUG_CONTEXTMENU
    if (!ac) {
        return;
    }
    if (ac->text() == "Add dummy gallery items") {
        GalleryItem *item = new GalleryItem("dir1/file1.JPG");
        //preview_gallery_storage.insert(path, item);
        ui->preview_gallery->addItem(item);
        item = new GalleryItem("dir1/file2.JPG");
        ui->preview_gallery->addItem(item);
    } else if (ac->text() == "Simulate LiveView small") {
        liveview->simulate("zwei%1.jpg", 12);
        ui->preview->set_focus(true, 15, 30, 70, 64);
    } else if (ac->text() == "Simulate LiveView large") {
        liveview->simulate("test%1.raw", 28);
        ui->preview->set_focus(true, 44, 25, 70, 64);
    } else if (ac->text() == "Test force_format in Remote::generate_download_url()") {
        qDebug() << "KeepFormat:";
        qDebug() << Remote::generate_download_url("dir1/file1.JPG", Remote::PhotoSize::View, Remote::ForceFormat::KeepFormat);
        qDebug() << Remote::generate_download_url("dir1/file2.DNG", Remote::PhotoSize::View, Remote::ForceFormat::KeepFormat);
        qDebug() << Remote::generate_download_url("dir1/file3.PEF", Remote::PhotoSize::View, Remote::ForceFormat::KeepFormat);
        qDebug() << "DNG:";
        qDebug() << Remote::generate_download_url("dir1/file1.JPG", Remote::PhotoSize::View, Remote::ForceFormat::DNG);
        qDebug() << Remote::generate_download_url("dir1/file2.DNG", Remote::PhotoSize::View, Remote::ForceFormat::DNG);
        qDebug() << Remote::generate_download_url("dir1/file3.PEF", Remote::PhotoSize::View, Remote::ForceFormat::DNG);
        qDebug() << "JPG:";
        qDebug() << Remote::generate_download_url("dir1/file1.JPG", Remote::PhotoSize::View, Remote::ForceFormat::JPG);
        qDebug() << Remote::generate_download_url("dir1/file2.DNG", Remote::PhotoSize::View, Remote::ForceFormat::JPG);
        qDebug() << Remote::generate_download_url("dir1/file3.PEF", Remote::PhotoSize::View, Remote::ForceFormat::JPG);
        qDebug() << "PEF:";
        qDebug() << Remote::generate_download_url("dir1/file1.JPG", Remote::PhotoSize::View, Remote::ForceFormat::PEF);
        qDebug() << Remote::generate_download_url("dir1/file2.DNG", Remote::PhotoSize::View, Remote::ForceFormat::PEF);
        qDebug() << Remote::generate_download_url("dir1/file3.PEF", Remote::PhotoSize::View, Remote::ForceFormat::PEF);
    } else if (ac->text() == "Test TV formatter") {
        QStringList values({"30.1", "25.1", "20.1", "15.1", "13.1", "10.1", "8.1", "6.1", "5.1", "4.1", "3.1", "25.10", "2.1", "16.10", "13.10", "1.1", "8.10", "6.10", "5.10", "4.10", "3.10", "1.4", "1.5", "1.6", "1.8", "1.10", "1.13", "1.15", "1.20", "1.25", "1.30", "1.40", "1.50", "1.60", "1.80", "1.100", "1.125", "1.160", "1.200", "1.250", "1.320", "1.400", "1.500", "1.640", "1.800", "1.1000", "1.1250", "1.1600", "1.2000", "1.2500", "1.3200", "1.4000", "1.5000", "1.6000"});
        for (const auto &item : values) {
            qDebug() << item << GalleryItem::format_tv(item);
        }
    } else if (ac->text() == "Test resolution check") {
        QStringList res({"1080x720", "720x480", "x400", "500x"});
        qDebug() << "should be 1:" << find_minimum_resolution(res);
        QStringList res2;
        qDebug() << "should be -1:" << find_minimum_resolution(res2);
    }
#endif //ENABLE_DEBUG_CONTEXTMENU
}


void MainWindow::preview_cmaction_toggle_ignore_liveview()
{
    ignore_liveview = !ignore_liveview;
}


void MainWindow::preview_cmaction_toggle_enlarge()
{
    qDebug() << "preview_cmaction_toggle_enlarge";
    ui->preview->set_allow_enlarge(!ui->preview->get_allow_enlarge());
}


void MainWindow::on_preview_gallery_customContextMenuRequested(const QPoint &pos)
{
    //qDebug() << "on_preview_gallery_customContextMenuRequested";
    QMenu submenu(ui->preview_gallery);
    submenu.addAction(tr("Display in preview"), this, &MainWindow::on_preview_gallery_itemDoubleClicked);
    if (photo_format_available_jpg) {
        submenu.addAction(tr("Display in preview (full)"), this, &MainWindow::preview_gallery_cmaction_display_item_full);
    }
    submenu.addAction(tr("Copy selected url(s) to clipboard"), this, [=]() {
        preview_gallery_cmaction_copy_urls_to_clipboard();
    });

    if (photo_format_available_dng) {
        submenu.addAction(tr("Copy selected url(s) to clipboard (DNG)"), this, [=]() {
            preview_gallery_cmaction_copy_urls_to_clipboard(Remote::ForceFormat::DNG);
        });
    }
    if (photo_format_available_jpg) {
        submenu.addAction(tr("Copy selected url(s) to clipboard (JPG)"), this, [=]() {
            preview_gallery_cmaction_copy_urls_to_clipboard(Remote::ForceFormat::JPG);
        });
    }
    if (photo_format_available_pef) {
        submenu.addAction(tr("Copy selected url(s) to clipboard (PEF)"), this, [=]() {
            preview_gallery_cmaction_copy_urls_to_clipboard(Remote::ForceFormat::PEF);
        });
    }
    submenu.addSeparator();
    submenu.addAction(tr("Select all"), ui->preview_gallery, &QListWidget::selectAll);
    submenu.exec(ui->preview_gallery->mapToGlobal(pos));
}


void MainWindow::preview_gallery_cmaction_copy_urls_to_clipboard(Remote::ForceFormat force_format)
{
    qDebug() << "on_preview_gallery_cm_copy_url_to_clipboard";
    const QList<QListWidgetItem*> &items = ui->preview_gallery->selectedItems();
    QString urls;
    for (const auto &item : items) {
        const QString &path = item->data(Qt::UserRole).toString();
        urls.append(Remote::generate_download_url(path, Remote::PhotoSize::Full, force_format));
        urls.append(" ");
    }
    if (urls.size()) {
        QApplication::clipboard()->setText(urls);
    }
}


void MainWindow::preview_gallery_cmaction_display_item_full()
{
    preview_gallery_display_item(true);
}


void MainWindow::on_sa_settings_customContextMenuRequested(const QPoint &pos)
{
    //qDebug() << "on_sa_settings_customContextMenuRequested";
    QMenu submenu(ui->sa_settings);
    submenu.addAction(tr("Force full refresh"), remote, &Remote::get_props);
    submenu.addAction(tr("Force value refresh"), remote, &Remote::get_parameter);
    submenu.addSeparator();
    if (wifi_settings_locked) {
        submenu.addAction(tr("Unlock WiFi settings"), this, &MainWindow::sa_settings_cmaction_toggle_wifi_settings_lock);
    } else {
        submenu.addAction(tr("Lock WiFi settings"), this, &MainWindow::sa_settings_cmaction_toggle_wifi_settings_lock);
        submenu.addAction(tr("Change WiFi settings..."), this, &MainWindow::sa_settings_cmaction_update_wifi_settings);
    }
    submenu.exec(ui->sa_settings->mapToGlobal(pos));
}


void MainWindow::sa_settings_cmaction_toggle_wifi_settings_lock()
{
    if (wifi_settings_locked) {
        ui->le_ssid->setEnabled(true);
        ui->le_key->setEnabled(true);
        ui->le_key->setEchoMode(QLineEdit::Normal);
        ui->c_channel->setEnabled(true);
    } else {
        ui->le_ssid->setEnabled(false);
        ui->le_key->setEnabled(false);
        ui->le_key->setEchoMode(QLineEdit::Password);
        ui->c_channel->setEnabled(false);

    }
    wifi_settings_locked = !wifi_settings_locked;
}


void MainWindow::sa_settings_cmaction_update_wifi_settings()
{
    if (ui->c_channel->count() <= 0) {
        QMessageBox::critical(this, tr("Oops"), tr("No valid channel options found.\nYou might want to force a full refresh."));
        return;
    }

    const QString &new_ssid = ui->le_ssid->text();
    const QString &new_key = ui->le_key->text();
    const QString &new_channel = ui->c_channel->currentText();
    const QString &sec = "confirm";

    // check if ssid, key and channel are the previous values:
    if (ui->le_ssid->placeholderText() == new_ssid && ui->le_key->placeholderText() == new_key && ui->c_channel->property("previous_value").toString() == new_channel) {
        QMessageBox::information(this, tr("Oops"), tr("SSID, key and channel have not changed."));
        return;
    }

    if (new_ssid.size() <= 4) {
        QMessageBox::critical(this, tr("Oops"), tr("New SSID is too short. It has to be at least 4 characters long."));
        return;
    }
    if (new_key.size() <= 4) {
        QMessageBox::critical(this, tr("Oops"), tr("New key is too short. It has to be at least 4 characters long."));
        return;
    }
    if (new_channel.size() <= 0) {
        QMessageBox::critical(this, tr("Oops"), tr("New channel is invalid."));
        return;
    }

    bool ok;
    QString warning_msg = QString(tr("Changing the WiFi settings will break your connection\nto the camera! If you want to change the\nSSID to '%1', the\nkey to '%2' and the\nchannel to '%3',\ntype '%4' into the following box:")).arg(new_ssid, new_key, new_channel, sec);
    QString text = QInputDialog::getText(this, tr("Confirm WiFi settings change"),
                                         warning_msg, QLineEdit::Normal, "", &ok);
    if (ok && text == sec) {
        remote->set_device_wifi_settings(new_ssid, new_key, new_channel);
    }
}


