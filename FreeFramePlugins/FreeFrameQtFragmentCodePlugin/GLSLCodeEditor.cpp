
#include <QPainter>
#include <QtGui>

#include "GLSLSyntaxHighlighter.h"
#include "GLSLCodeEditor.moc"


class LineNumberArea : public QWidget
{
public:
    LineNumberArea(GlslCodeEditor *editor) : QWidget(editor) {
        CodeEditor = editor;
    }

    QSize sizeHint() const {
        return QSize(CodeEditor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) {
        CodeEditor->lineNumberAreaPaintEvent(event);
    }

private:
    GlslCodeEditor *CodeEditor;
};

GlslCodeEditor::GlslCodeEditor(QWidget *parent) : QTextEdit(parent)
{
    lineNumberArea = new LineNumberArea(this);

    setFontFamily("monospace");
    setTabStopWidth(30);
    GlslSyntaxHighlighter *highlighter = new GlslSyntaxHighlighter(document());

    connect(document(), SIGNAL(blockCountChanged(int)), this, SLOT(updateLineNumberAreaWidth(int)));
    connect(this, SIGNAL(updateRequest(QRect,int)), this, SLOT(updateLineNumberArea(QRect,int)));
//    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(highlightCurrentLine()));

    updateLineNumberAreaWidth(0);
//    highlightCurrentLine();
}



int GlslCodeEditor::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, document()->blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    int space = 3 + fontMetrics().width(QLatin1Char('0')) * digits;

    return space;
}



void GlslCodeEditor::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}



void GlslCodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}



void GlslCodeEditor::resizeEvent(QResizeEvent *e)
{
    QTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}



void GlslCodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;

        QColor lineColor = QColor(Qt::gray);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}


void GlslCodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), Qt::lightGray);


    QTextBlock block = document()->firstBlock();
    int blockNumber = block.blockNumber();

    int top = 0;
    //      int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int) document()->documentLayout()->blockBoundingRect(block).height();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(Qt::black);
            painter.drawText(0, top, lineNumberArea->width(), fontMetrics().height(),
                             Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + (int) document()->documentLayout()->blockBoundingRect(block).height();
        ++blockNumber;
    }
}
