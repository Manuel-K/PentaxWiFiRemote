#include "ui/PreviewWidget.h"


#include <QDebug>
#include <QMouseEvent>
#include <math.h>

PreviewWidget::PreviewWidget(QWidget *parent, Qt::WindowFlags f)
    : QLabel(parent, f),
      focus_effective_area_border(new QFrame(this)),
      focus_marker(new QFrame(this))
{
    max_value = 100.0;
    focus_effective_area_x = 70;
    focus_effective_area_y = 64;
    focus_marker_x = -1;
    focus_marker_y = -1;

    focus_effective_area_border->setFrameStyle(QFrame::Box);
    focus_effective_area_border->setCursor(Qt::CrossCursor);
    focus_effective_area_border->hide();
    focus_marker->setFrameStyle(QFrame::Box);
    focus_marker->setLineWidth(2);
    focus_marker->hide();
}


PreviewWidget::~PreviewWidget()
{
    focus_effective_area_border->deleteLater();
    focus_marker->deleteLater();
}


void PreviewWidget::set_max_value(double val)
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
    qDebug() << "border" << border_x0_val << border_y0_val << border_x0_val+border_dx_val << border_y0_val + border_dy_val;

    int pm_dx = pixmap()->width();
    int pm_dy = pixmap()->height();
    int gap_dx = (width() - pm_dx)/2;
    int gap_dy = (height() - pm_dy)/2;


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
    int x = gap_dx + static_cast<int>(round(1.0 * pm_dx * focus_marker_x / max_value));
    int y = gap_dy + static_cast<int>(round(1.0 * pm_dy * focus_marker_y / max_value));
    focus_marker->setGeometry(x - dx/2, y - dy/2, dx, dy);
}


QPixmap PreviewWidget::scaled_pixmap() const
{
    // Do not enlarge image
    if (this->size().width() > pm.size().width() && this->size().height() > pm.size().height()) {
        return pm;
    }
    return pm.scaled(this->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
}


void PreviewWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        if (event->button() == Qt::RightButton) {
            emit rightClicked();
        }
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
    int x = static_cast<int>(round(max_value * ev_x / pm_dx));
    int y = static_cast<int>(round(max_value * ev_y / pm_dy));
    //qDebug() << x << y;
    emit clicked(x, y);
}
