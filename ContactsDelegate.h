#ifndef CONTACTSDELEGATE_H
#define CONTACTSDELEGATE_H
#include <QStyledItemDelegate>
#include <QPainter>
class ContactsDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit ContactsDelegate(QObject *parent = nullptr);

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

#endif // CONTACTSDELEGATE_H
