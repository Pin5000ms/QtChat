#ifndef MESSAGEDELEGATE_H
#define MESSAGEDELEGATE_H

#include <QStyledItemDelegate>
#include <QPainter>

class MessageDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit MessageDelegate(QObject *parent = nullptr);

    // 在 QStandardItemModel 中設定自定義角色
    enum MessageRoles {
        MessageTypeRole = Qt::UserRole + 1,  // 自定義角色
        AvatarRole
    };

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    // 覆寫 paint 函數來自定義每一個項目的顯示方式
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

#endif // MESSAGEDELEGATE_H
