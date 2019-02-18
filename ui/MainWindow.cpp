#include "ui/MainWindow.h"
#include "ui_MainWindow.h"


#include "LiveView.h"
#include "Remote.h"
//#include "ui/PreviewGallery.h"
#include "ui/GalleryItem.h"
//#include "ui/PreviewGalleryItemDelegate.h"

#include <QDebug>


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
    //liveview->simulate("zwei%1.jpg", 12);
    //liveview->simulate("test%1.raw", 28);

    connect(remote, &Remote::parameter_items, this, &MainWindow::handle_parameter_items);
    connect(remote, &Remote::parameter, this, &MainWindow::handle_parameter);
    connect(remote, &Remote::photo_info, this, &MainWindow::handle_photo_info);
    connect(remote, &Remote::photo_received, this, &MainWindow::handle_photo_received);
    connect(remote, &Remote::focus_info, this, &MainWindow::handle_focus_info);

    remote->get_props();

    //connect(ui->c_autoFocus, &QComboBox::currentTextChanged, this, &MainWindow::handle_parameter_cb_current_text_changed);

    connect(ui->c_reso, &QComboBox::currentTextChanged, this, &MainWindow::handle_parameter_cb_current_text_changed);
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
    ui->preview->setPixmap(image, true);
}


void MainWindow::handle_parameter_items(const QString type, const QStringList values)
{
    QComboBox *cb = this->findChild<QComboBox*>("c_"+type);
    bool prev = cb->blockSignals(true);
    cb->clear();
    cb->addItems(values);
    //qDebug() << values.at(0);
    cb->blockSignals(prev);
}


void MainWindow::handle_parameter(const QString type, const QString value, bool read_only)
{
    qDebug() << "handle_set_param" << type << value << read_only;
    if (read_only) {
        QLabel *l = this->findChild<QLabel*>("l_"+type);
        l->setText(value);
    } else {
        QComboBox *cb = this->findChild<QComboBox*>("c_"+type);
        bool prev = cb->blockSignals(true);
        //int index = cb->findText(value);
        cb->setCurrentText(value);
        //qDebug() << values.at(0);
        cb->blockSignals(prev);
    }
}


void MainWindow::handle_photo_info(const QString &path, const QJsonObject &json, bool latest)
{
    qDebug() << "handle_photos_latest_info" << path << json << latest;
    GalleryItem *item;
    if (!preview_gallery_storage.contains(path)) {
        qDebug() << "parsing info for new image";
        item = new GalleryItem(path);
        //item->parse_info(json);
        if (latest) {
            preview_gallery_storage.insert(path, item);
            ui->preview_gallery->addItem(item);
        } else {
            qDebug() << "Normal info receiving not implemented";
        }
    } else {
        qDebug() << "parsing info for existing image";
        item = preview_gallery_storage.value(path);
        //item->parse_info(json);
    }
    item->parse_info(json);
    if (ui->cb_liveview->isChecked()) {
        remote->fetch_photo(path, Remote::PhotoSize::Thumb);
    } else {
        remote->fetch_photo(path, Remote::PhotoSize::View);
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
    //ui->preview->setPixmap(pm);
    switch(size) {
        case Remote::PhotoSize::Thumb:
            item->set_thumbnail(pm);
        break;
        case Remote::PhotoSize::View:
            item->set_preview(pm);
            if (!ui->cb_liveview->isChecked()) {
                ui->preview->setPixmap(pm, false);
            }
        break;
        case Remote::PhotoSize::Full:
            item->set_full(pm);
        break;
    }
}


void MainWindow::handle_focus_info(bool focused, int center_x, int center_y, int eff_x, int eff_y)
{
    qDebug() << "handle_set_focus" << focused << center_x << center_y << eff_x << eff_y;
    // TODO: display focused value somewhere in MainWindow
    ui->preview->set_focus(focused, center_x, center_y, eff_x, eff_y);
}


void MainWindow::handle_parameter_cb_current_text_changed(const QString &value)
{
    QComboBox* s = static_cast<QComboBox*>(sender());
    QString id = s->objectName();
    id.remove(0, 2);

    //qDebug() << "handle_parameter_cb_current_text_changed" << id << value;
    remote->set_parameter(id, value);
}


void MainWindow::on_cb_liveview_toggled(bool state)
{
    qDebug() << "on_cb_liveview_toggled" << state;
    qDebug() << "size cb:" << ui->c_reso->currentText();
    // TODO: use value of resolution combobox
    if (state) {
        liveview->start(true);
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
    //ui->listView->setModel()
}


void MainWindow::on_pb_debug_clicked()
{
    qDebug() << "on_pb_debug_clicked";
    remote->latest();
    //liveview->simulate("test%1.raw", 28);
}


void MainWindow::on_preview_clicked(int x, int y)
{
    //qDebug() << "on_preview_clicked" << x << y;
    // focus with position only works if LiveView is enabled
    if (ui->cb_liveview->isChecked()) {
        remote->focus_at(x, y);
    } else {
        qDebug() << "Can only focus at position if LiveView is enabled.";
    }
}
