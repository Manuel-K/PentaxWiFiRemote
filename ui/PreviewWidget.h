#ifndef PREVIEWWIDGET_H
#define PREVIEWWIDGET_H

#include <QLabel>
#include <Qt>
#include <QWidget>

class PreviewWidget : public QLabel
{
    Q_OBJECT

public:
    explicit PreviewWidget(QWidget *parent = Q_NULLPTR, Qt::WindowFlags f = Qt::WindowFlags());
    ~PreviewWidget();
    void set_max_value(double val);
    void set_focus(bool focused, int center_x, int center_y, int eff_x, int eff_y);


public slots:
    void setPixmap(const QPixmap &pixmap, bool show_af_border);
    void resizeEvent(QResizeEvent *);


private:
    void update_focus_effective_area();
    void update_focus_marker();
    QPixmap scaled_pixmap() const;

    double max_value; // k70 stops reacting at values >= 1000, but the app seems to indicate that 80 is max, some tests indicate 100
    QPixmap pm;
    QFrame *focus_effective_area_border;
    QFrame *focus_marker;
    int focus_effective_area_x;
    int focus_effective_area_y;
    int focus_marker_x;
    int focus_marker_y;


protected:
    void mousePressEvent(QMouseEvent *event);


signals:
    void clicked(int x, int y);
    void rightClicked();

};

#endif // PREVIEWWIDGET_H
