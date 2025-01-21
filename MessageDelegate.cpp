#include "MessageDelegate.h"
#include <QPixmap>
#include <QStyleOptionViewItem>
#include <QDebug>

#define AVATARW 45
#define AVATARW_HALF 22


MessageDelegate::MessageDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

int calculateTextWidth(const QString &text) {

    QFont font("Arial", 12);  // 使用 Arial 字体，字号为 12


    // 创建 QFontMetrics 对象，使用指定的字体
    QFontMetrics metrics(font);

    // 计算文本的宽度
    int textWidth = metrics.horizontalAdvance(text + " ");//" "為margin

    // 输出宽度
    qDebug() << "Text width:" << textWidth << "pixels";

    return textWidth;
}

void drawWrappedText(QPainter *painter, const QRect &textRect, const QString &text)
{
    int maxLineLength = 33;  // 每行最多顯示35個字符
    QString wrappedText;

    // 循環處理字符串，按每33個字符分割
    for (int i = 0; i < text.length(); i += maxLineLength) {
        // 使用 mid() 方法獲取每段文本並添加換行符
        wrappedText += text.mid(i, maxLineLength) + "\n";
    }

    // 設定字型和大小
    QFont font;
    font.setFamily("Arial");  // 設置字型
    font.setPointSize(12);    // 設置字體大小
    painter->setFont(font);   // 將字型設置到畫家

    // 創建文本選項，這樣可以讓文本自動換行
    QTextOption textOption;
    textOption.setWrapMode(QTextOption::WordWrap);  // 啟用自動換行

    // 繪製文本
    painter->drawText(textRect, wrappedText, textOption);
}

void MessageDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    // 根據自定義角色判斷消息類型
    QString messageType = index.data(DirectionTypeRole).toString();// sent or receive
    // 獲取消息內容和頭像
    QString text = index.data(Qt::DisplayRole).toString();
    QPixmap avatar = index.data(Qt::DecorationRole).value<QPixmap>();

    QString dataType = index.data(DataTypeRole).toString();



    QStyleOptionViewItem newOption(option);

    // 增加上下 margin
    QRect itemRect = newOption.rect;
    itemRect.setTop(itemRect.top() + 10);  // Top margin
    itemRect.setBottom(itemRect.bottom() - 10);  // Bottom margin



    QRect bubbleRect = itemRect;//option.rect;


    if(dataType == "text"){
        int textWidth = calculateTextWidth(text);
        // 設定範圍
        if(messageType == "sent"){
            int origin = itemRect.right() - 10;// - AVATARW;
            bubbleRect.setRight(origin);

            if(textWidth < itemRect.width()*0.8)
                bubbleRect.setLeft(origin - textWidth - 10);
            else
                bubbleRect.setLeft(itemRect.center().x() + 10);
        }
        else if (messageType == "receive"){
            int origin = itemRect.left() + AVATARW + 10;
            bubbleRect.setLeft(origin);

            if(textWidth < itemRect.width()*0.8)
                bubbleRect.setRight(origin + textWidth + 10);
            else
                bubbleRect.setRight(itemRect.center().x() - 10);
        }
    }
    else if(dataType == "file"){
        const int fileIconWidth = 75;
        QPixmap filePixmap(":/icon/file.png");
        if(messageType == "sent"){
            int origin = itemRect.right() - 10;// - AVATARW;
            bubbleRect.setRight(origin);
            bubbleRect.setLeft(origin - fileIconWidth - 10);
        }
        else if (messageType == "receive"){
            int origin = itemRect.left() + AVATARW + 10;
            bubbleRect.setLeft(origin);
            bubbleRect.setRight(origin + fileIconWidth + 10);
            //如何在下方新增 接收 取消 兩個按鈕?
        }
        painter->drawPixmap(bubbleRect, filePixmap);

    }



    // 根據消息類型設置不同底色
    QColor backgroundColor = (messageType == "sent") ? QColor(0, 240, 0, 100) : QColor(220, 220, 220, 100);



    // 設置圓角背景顏色
    QPainterPath path;
    path.addRoundedRect(bubbleRect, 10, 10);  // 設置圓角
    painter->fillPath(path, backgroundColor); // 填充背景顏色


    if(dataType == "text"){

        //margin
        QRect textRect = bubbleRect;//option.rect;
        textRect.setLeft(bubbleRect.left() + 10);
        textRect.setTop(bubbleRect.top() + 10);

        drawWrappedText(painter, textRect, text);
    }



    // 根據消息類型決定頭像的位置
    int avatarX = (messageType == "sent") ? option.rect.right() - AVATARW : option.rect.left() + 10;
    QRect avatarRect(avatarX, option.rect.top() + 5, AVATARW, AVATARW);
    if(messageType == "receive")
        painter->drawPixmap(avatarRect, avatar);  // 繪製頭像

}




int getRequiredLines(const QString &text) {
    const int charsPerLine = 29;  // 每行总计字符宽度为30个字节
    int totalWidth = 0;

    // 遍历字符串中的每个字符
    for (int i = 0; i < text.length(); ++i) {
        QChar ch = text[i];

        // 判断是否是中文字符（通常在Unicode范围内）
        if (ch.unicode() >= 0x4E00 && ch.unicode() <= 0x9FFF) {
            totalWidth += 2;  // 中文字符算作2字节宽度
        } else {
            totalWidth += 1;  // 其他字符算作1字节宽度
        }
    }

    // 计算所需行数，向上取整
    int requiredLines = (totalWidth + charsPerLine - 1) / charsPerLine;
    return requiredLines;
}

QSize MessageDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QString text = index.data(Qt::DisplayRole).toString();

    QString dataType = index.data(DataTypeRole).toString();
    if(dataType == "file"){
        return QSize(option.rect.width(), 100);
    }
    // 根據每行30個字元來計算所需行數
    int requiredLines = getRequiredLines(text);

    // 假設每行的高度是 20px
    int lineHeight = 33;

    // 計算所需的總高度：每行的高度 * 行數
    int totalHeight = requiredLines * lineHeight + 30;

    //qDebug()<<totalHeight;

    return QSize(option.rect.width(), totalHeight);  // 返回調整後的行高
}
