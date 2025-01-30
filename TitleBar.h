#ifndef TITLEBAR_H
#define TITLEBAR_H

#include <QMouseEvent>
#include <QWidget>
#include <QPainter>

class TitleBar : public QWidget {
    Q_OBJECT
public:
    explicit TitleBar(QWidget *parent = nullptr) : QWidget(parent) {}

protected:
    void paintEvent(QPaintEvent *event) override {
        QPainter painter(this);
        painter.fillRect(this->rect(), Qt::black); // 強制背景黑色
    }

    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            // 獲取 MainWindow
            QWidget *mainWin = this->window();
            if (mainWin && mainWin->isWindow()) {  // 確保 mainWin 是個窗口
                lastMousePosition = event->globalPos() - mainWin->pos();
                dragging = true;
            }
            event->accept();
        }
    }

    void mouseMoveEvent(QMouseEvent *event) override {
        if (dragging && (event->buttons() & Qt::LeftButton)) {
            QWidget *mainWin = this->window();
            if (mainWin && mainWin->isWindow()) {
                mainWin->move(event->globalPos() - lastMousePosition);
            }
            event->accept();
        }
    }

    void mouseReleaseEvent(QMouseEvent *event) override {
        dragging = false;
        event->accept();
    }

private:
    QPoint lastMousePosition;
    bool dragging = false;
};

#endif // TITLEBAR_H
