#ifndef CUSTOMTEXTEDIT_H
#define CUSTOMTEXTEDIT_H

#include <QTextEdit>
#include <QKeyEvent>

class CustomTextEdit : public QTextEdit {
    Q_OBJECT
public:
    explicit CustomTextEdit(QWidget *parent = nullptr) : QTextEdit(parent) {}

signals:
    void enterPressed(); // 當按下 Enter 時發送此訊號

protected:
    void keyPressEvent(QKeyEvent *event) override {
        if ((event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) && !(event->modifiers() & Qt::ShiftModifier)) {
            emit enterPressed();  // 觸發訊號
            event->accept();      // 阻止 QTextEdit 預設的換行行為
        } else {
            QTextEdit::keyPressEvent(event);  // 保持其他鍵的正常行為
        }
    }
};

#endif // CUSTOMTEXTEDIT_H
