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

    QFont font("Microsoft YaHei", 14);  // 使用 Arial 字体，字号为 12


    // 创建 QFontMetrics 对象，使用指定的字体
    QFontMetrics metrics(font);

    // 计算文本的宽度
    int textWidth = metrics.horizontalAdvance(text);//" "為margin

    // 输出宽度
    //qDebug() << "Text width:" << textWidth << "pixels";

    return textWidth;
}


QString GenWrappedText(const QRect &bubbleRect, const QString &text)
{
    int maxWidth = bubbleRect.width() - 25;
    QString wrappedText;

    int sum = 0;


    // 循環處理字符串，sum一旦超出maxWidth，加入換行符，重置sum
    for (int i = 0; i < text.length(); i++) {
        sum += calculateTextWidth(text[i]);
        wrappedText += text[i];
        if(sum >= maxWidth)
        {
            sum = 0;
            wrappedText += "\n";
        }
    }

    return wrappedText;
}


void DrawWrappedText(const QRect &bubbleRect, QPainter *painter, const QString &text)
{

    //margin
    QRect textRect = bubbleRect;//option.rect;
    textRect.setLeft(bubbleRect.left() + 10);
    textRect.setTop(bubbleRect.top() + 10);


    QString wrappedText = GenWrappedText(bubbleRect, text);

    // 設定字型和大小
    QFont font;
    font.setFamily("Microsoft YaHei");  // 設置字型
    font.setPointSize(14);    // 設置字體大小
    painter->setFont(font);   // 將字型設置到畫家

    // 創建文本選項，這樣可以讓文本自動換行
    QTextOption textOption;
    textOption.setWrapMode(QTextOption::WordWrap);  // 啟用自動換行

    // 繪製文本
    painter->drawText(textRect, wrappedText, textOption);

}



//設定包圍訊息的聊天氣泡的邊界
void SetTextBubble(QRect& bubbleRect, QString text, QString direction)
{
    int textWidth = calculateTextWidth(text);
    QRect itemRect = bubbleRect;

    // 設定範圍
    if(direction == "sent"){
        int origin = itemRect.right() - 10;// - AVATARW;
        bubbleRect.setRight(origin);

        if(textWidth < itemRect.width() * 0.8)
            bubbleRect.setLeft(origin - textWidth - 20);
        else
            bubbleRect.setLeft(origin - itemRect.width() * 0.8);
    }
    else if (direction == "receive"){
        int origin = itemRect.left() + AVATARW + 10;
        bubbleRect.setLeft(origin);

        if(textWidth < itemRect.width() * 0.8)
            bubbleRect.setRight(origin + textWidth + 20);
        else
            bubbleRect.setRight(origin + itemRect.width() * 0.8);
    }

    qDebug()<<"SetTextBubble bubbleRect : "<<bubbleRect.width();
}

//設定包圍file圖示的聊天氣泡的邊界
void SetFileBubble(QRect& bubbleRect, QPainter *&painter, QString direction)
{
    const int fileIconWidth = 75;
    QRect itemRect = bubbleRect;

    QPixmap filePixmap(":/icon/file.png");
    if(direction == "sent"){
        int origin = itemRect.right() - 10;// - AVATARW;
        bubbleRect.setRight(origin);
        bubbleRect.setLeft(origin - fileIconWidth - 10);
    }
    else if (direction == "receive"){
        int origin = itemRect.left() + AVATARW + 10;
        bubbleRect.setLeft(origin);
        bubbleRect.setRight(origin + fileIconWidth + 10);
        //如何在下方新增 接收 取消 兩個按鈕?
    }
    painter->drawPixmap(bubbleRect, filePixmap);
}


void DrawProgress(QRect& bubbleRect, QPainter *&painter, int progress)
{
    QRect progressRect = bubbleRect;
    progressRect.setTop(bubbleRect.bottom() + 5); // 在文件圖標下方
    progressRect.setHeight(10); // 進度條高度
    progressRect.setLeft(progressRect.left() + 5);
    progressRect.setRight(progressRect.right() - 5);

    painter->setBrush(Qt::gray); // 進度條背景
    painter->drawRect(progressRect);

    QRect progressFillRect = progressRect;
    progressFillRect.setWidth(progressRect.width() * progress / 100); // 根據進度調整寬度
    painter->setBrush(Qt::blue); // 進度條填充顏色
    painter->drawRect(progressFillRect);
}


void DrawBubble(QRect& bubbleRect, QPainter *&painter, QString direction)
{
    // 根據消息類型設置不同底色
    QColor backgroundColor = (direction == "sent") ? QColor(180, 150, 255, 180) : QColor(200, 200, 200, 180);
    // 設置圓角背景顏色
    QPainterPath path;
    path.addRoundedRect(bubbleRect, 10, 10);  // 設置圓角
    painter->fillPath(path, backgroundColor); // 填充背景顏色
}



void MessageDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{

    // 根據自定義角色判斷消息類型
    QString messageDirection = index.data(DirectionTypeRole).toString();// sent or receive
    // 獲取消息內容和頭像
    QString text = index.data(Qt::DisplayRole).toString();
    QPixmap avatar = index.data(Qt::DecorationRole).value<QPixmap>();
    QString dataType = index.data(DataTypeRole).toString();


    QStyleOptionViewItem newOption(option);

    // 增加上下 margin
    QRect itemRect = newOption.rect;//高度從sizeHint來的
    itemRect.setTop(itemRect.top() + 10);  // Top margin
    itemRect.setBottom(itemRect.bottom() - 10);  // Bottom margin
    QRect bubbleRect = itemRect;





    if(dataType == "text")
    {
        SetTextBubble(bubbleRect, text, messageDirection);

        DrawBubble(bubbleRect, painter, messageDirection);

        DrawWrappedText(bubbleRect, painter, text);
    }
    else if(dataType == "file")
    {
        SetFileBubble(bubbleRect, painter, messageDirection);

        DrawBubble(bubbleRect, painter, messageDirection);

        int progress = index.data(ProgressRole).toInt(); // 取得進度值
        DrawProgress(bubbleRect, painter, progress);
    }





    // 根據消息類型決定頭像的位置
    int avatarX = (messageDirection == "sent") ? option.rect.right() - AVATARW : option.rect.left() + 10;
    QRect avatarRect(avatarX, option.rect.top() + 5, AVATARW, AVATARW);
    if(messageDirection == "receive")
        painter->drawPixmap(avatarRect, avatar);  // 繪製頭像

}




int getRequiredLines(QRect& itemRect, const QString &text) {

    int textWidth = calculateTextWidth(text);
    int maxWidth = 0;

    if(textWidth < itemRect.width() * 0.8)
        maxWidth = textWidth + 1;
    else
        maxWidth = itemRect.width() * 0.8 + 1;



    int lines = 1;
    int sum = 0;


    qDebug()<<"getRequiredLines itemRect : "<<itemRect.width();
    for (int i = 0; i < text.length(); i++) {
        sum += calculateTextWidth(text[i]);
        if(sum >= maxWidth)
        {
            sum = 0;
            lines++;
        }
    }
    return lines;

}

QSize MessageDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QString text = index.data(Qt::DisplayRole).toString();

    QString dataType = index.data(DataTypeRole).toString();
    if(dataType == "file"){
        return QSize(option.rect.width(), 120);
    }

    QRect itemRect = option.rect;
    int requiredLines = getRequiredLines(itemRect, text);


    QFont font("Microsoft YaHei", 14);
    QFontMetrics metrics(font);

    int lineHeight = metrics.lineSpacing();

    // 計算所需的總高度：每行的高度 * 行數
    int totalHeight = requiredLines * lineHeight + 40;

    //qDebug()<<totalHeight;

    return QSize(option.rect.width(), totalHeight);  // 返回調整後的行高
}
