#include "ui/PreviewWidget.h"


#include <QDebug>
#include <QMouseEvent>
#include <math.h>

PreviewWidget::PreviewWidget(QWidget *parent, Qt::WindowFlags f)
    : QLabel(parent, f),
      focus_marker(new QFrame(this)),
      focus_effective_area_border(new QFrame(this)),
      transform(new QTransform())
{
    allow_enlarge = false;
    max_value = 100.0;
    focus_effective_area_x = 70;
    focus_effective_area_y = 64;
    focus_marker_x = -1;
    focus_marker_y = -1;

    focus_effective_area_border->setFrameStyle(QFrame::Box);
    focus_effective_area_border->setStyleSheet("color: grey;");
    focus_effective_area_border->setCursor(Qt::CrossCursor);
    focus_effective_area_border->hide();
    focus_marker->setFrameStyle(QFrame::Box);
    focus_marker->setLineWidth(2);
    focus_marker->setStyleSheet("color: red;");
    focus_marker->hide();

    apply_transformation = false;
    rotation = Rotation::None;
}


PreviewWidget::~PreviewWidget()
{
    focus_effective_area_border->deleteLater();
    focus_marker->deleteLater();
    delete transform;
}


void PreviewWidget::set_max_value(int val)
{
    max_value = val;
}


void PreviewWidget::set_focus(bool focused, int center_x, int center_y, int eff_x, int eff_y)
{
    if (eff_x != focus_effective_area_x || eff_y != focus_effective_area_y) {
        eff_x = focus_effective_area_x;
        eff_y = focus_effective_area_y;
        update_focus_effective_area();
    }
    if (focused && center_x >= 0 && center_y >= 0) {
        focus_marker_x = center_x;
        focus_marker_y = center_y;
        update_focus_marker();
        focus_marker->show();
    } else {
        focus_marker->hide();
    }
}


void PreviewWidget::set_rotation(Rotation rot)
{
    transform->reset();
    switch (rot) {
        case Rotation::None:
            apply_transformation = false;
        break;
        case Rotation::Rotate90:
            apply_transformation = true;
            transform->rotate(90);
        break;
        case Rotation::Rotate180:
            apply_transformation = true;
            transform->rotate(180);
        break;
        case Rotation::Rotate270:
            apply_transformation = true;
            transform->rotate(270);
        break;
    }
    rotation = rot;
    if (!pm.isNull()) {
        QLabel::setPixmap(scaled_pixmap());
        update_focus_effective_area();
        update_focus_marker();
    }
}


PreviewWidget::Rotation PreviewWidget::get_rotation() const
{
    return rotation;
}


void PreviewWidget::setPixmap(const QPixmap &pixmap, bool show_af_border)
{
    if (pixmap.isNull())
        return;

    pm = pixmap;
    const QPixmap &scaled = scaled_pixmap();

    // Update border geometry if pm size has changed
    bool must_scale = this->pixmap() == Q_NULLPTR;

    if (!must_scale && (this->pixmap()->width() != scaled.width() ||
                        this->pixmap()->height() != scaled.height())) {
        must_scale = true;
    }

    QLabel::setPixmap(scaled);

    if (must_scale) {
        update_focus_effective_area();
        update_focus_marker();
    }

    if (show_af_border) {
        focus_effective_area_border->show();
    } else {
        focus_effective_area_border->hide();
        focus_marker->hide();
    }
}


void PreviewWidget::resizeEvent(QResizeEvent *)
{
    if (!pm.isNull()) {
        QLabel::setPixmap(scaled_pixmap());
        update_focus_effective_area();
        update_focus_marker();
    }
}


void PreviewWidget::update_focus_effective_area()
{
    // Note: it is not clear if it should be round(), floor() or ceil() as
    // the values reported by the k70 are even. Nethertheless xmin and ymin are
    // one less than the obvious values while xmax and ymax are the obvious values.
    int border_x0_val = static_cast<int>((max_value - focus_effective_area_x) / 2) - 1;
    int border_y0_val = static_cast<int>((max_value - focus_effective_area_y) / 2) - 1;
    int border_dx_val = focus_effective_area_x + 1;
    int border_dy_val = focus_effective_area_y + 1;
    //qDebug() << "border" << "x" << border_x0_val << border_x0_val+border_dx_val << "y" << border_y0_val << border_y0_val + border_dy_val;

    int pm_dx = pixmap()->width();
    int pm_dy = pixmap()->height();
    int gap_dx = (width() - pm_dx)/2;
    int gap_dy = (height() - pm_dy)/2;


    if (apply_transformation) {
        int border_x1_val = border_x0_val + border_dx_val;
        int border_y1_val = border_y0_val + border_dy_val;
        int rot_x0 = transform_value_x(border_x0_val, border_y0_val);
        int rot_y0 = transform_value_y(border_x0_val, border_y0_val);
        int rot_x1 = transform_value_x(border_x1_val, border_y1_val);
        int rot_y1 = transform_value_y(border_x1_val, border_y1_val);
        //qDebug() << "rot" << "x" << rot_x0 << rot_x1 << "y" << rot_y0 << rot_y1;

        border_x0_val = (rot_x0 <= rot_x1) ? rot_x0 : rot_x1;
        border_dx_val = ((rot_x0 <= rot_x1) ? rot_x1 : rot_x0) - border_x0_val;
        border_y0_val = (rot_y0 <= rot_y1) ? rot_y0 : rot_y1;
        border_dy_val = ((rot_y0 <= rot_y1) ? rot_y1 : rot_y0) - border_y0_val;
        //qDebug() << "fixed" << "x" << border_x0_val << border_dx_val << "y" << border_y0_val << border_dy_val;
    }

    int border_x0 = gap_dx + static_cast<int>(round(1.0 * border_x0_val / max_value * pm_dx));
    int border_y0 = gap_dy + static_cast<int>(round(1.0 * border_y0_val / max_value * pm_dy));
    int border_dx = static_cast<int>(round(1.0 * border_dx_val / max_value * pm_dx));
    int border_dy = static_cast<int>(round(1.0 * border_dy_val / max_value * pm_dy));
    focus_effective_area_border->setGeometry(border_x0, border_y0, border_dx, border_dy);
}


void PreviewWidget::update_focus_marker()
{
    int pm_dx = pixmap()->width();
    int pm_dy = pixmap()->height();
    int gap_dx = (width() - pm_dx)/2;
    int gap_dy = (height() - pm_dy)/2;

    int dx = 40; // TODO: value
    int dy = 40; // TODO: value
    int x = gap_dx + static_cast<int>(round(1.0 * pm_dx * transform_value_x(focus_marker_x, focus_marker_y) / max_value));
    int y = gap_dy + static_cast<int>(round(1.0 * pm_dy * transform_value_y(focus_marker_x, focus_marker_y) / max_value));
    focus_marker->setGeometry(x - dx/2, y - dy/2, dx, dy);
}


QPixmap PreviewWidget::scaled_pixmap() const
{

    if(apply_transformation) {
        //qDebug() << "apply transformation";
        const QPixmap &pm_rot = pm.transformed(*transform, Qt::SmoothTransformation);

        if (!allow_enlarge && this->size().width() > pm_rot.size().width() && this->size().height() > pm_rot.size().height()) {
            return pm_rot;
        }

        return pm_rot.scaled(this->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    // Do not enlarge image
    if (!allow_enlarge && this->size().width() > pm.size().width() && this->size().height() > pm.size().height()) {
        return pm;
    }
    return pm.scaled(this->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
}


int PreviewWidget::transform_value_x(int x0, int y0)
{
    switch (rotation) {
        case Rotation::None:
            return x0;
        case Rotation::Rotate90:
            return max_value - y0;
        case Rotation::Rotate180:
            return max_value - x0;
        case Rotation::Rotate270:
            return y0;
    }
}


int PreviewWidget::transform_inv_value_x(int x0, int y0)
{
    switch (rotation) {
        case Rotation::None:
            return x0;
        case Rotation::Rotate90:
            return y0;
        case Rotation::Rotate180:
            return max_value - x0;
        case Rotation::Rotate270:
            return max_value - y0;
    }
}


int PreviewWidget::transform_value_y(int x0, int y0)
{
    switch (rotation) {
        case Rotation::None:
            return y0;
        case Rotation::Rotate90:
            return x0;
        case Rotation::Rotate180:
            return max_value - y0;
        case Rotation::Rotate270:
            return max_value - x0;
    }
}


int PreviewWidget::transform_inv_value_y(int x0, int y0)
{
    switch (rotation) {
        case Rotation::None:
            return y0;
        case Rotation::Rotate90:
            return max_value - x0;
        case Rotation::Rotate180:
            return max_value - y0;
        case Rotation::Rotate270:
            return x0;
    }
}


bool PreviewWidget::get_allow_enlarge() const
{
    return allow_enlarge;
}


void PreviewWidget::set_allow_enlarge(bool value)
{
    allow_enlarge = value;
    resizeEvent(Q_NULLPTR);
}


void PreviewWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }

    if (pixmap()->isNull()) {
        return;
    }

    int pm_dx = pixmap()->width();
    int pm_dy = pixmap()->height();

    if (pm_dy == 0 || pm_dy == 0) {
        return;
    }

    int gap_dx = (width() - pixmap()->width())/2;
    int gap_dy = (height() - pixmap()->height())/2;

    int ev_x = event->x() - gap_dx;
    int ev_y = event->y() - gap_dy;

    if (ev_x < 0 || ev_y < 0 || ev_x > pm_dx || ev_y > pm_dy) {
        qDebug() << "dropping event outside of pixmap";
        return;
    }

    // Calculation that assumes that everything (including the non-focusable area) is in range [0, max_value]
    int x = static_cast<int>(round(1.0 * max_value * ev_x / pm_dx));
    int y = static_cast<int>(round(1.0 * max_value * ev_y / pm_dy));

    int t_x = transform_inv_value_x(x, y);
    int t_y = transform_inv_value_y(x, y);
    //qDebug() << "normal" << x << y << "transformed" << t_x << t_y;
    //qDebug() << "clicked" << t_x << t_y;
    emit clicked(t_x, t_y);
}
