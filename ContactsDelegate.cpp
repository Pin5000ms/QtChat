#include "ContactsDelegate.h"


ContactsDelegate::ContactsDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

QSize ContactsDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    return QSize(option.rect.width(), 50);
}
