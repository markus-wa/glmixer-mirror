
#include <QPainter>
#include <QtGui>

#include "GLSLSyntaxHighlighter.h"
#include "GLSLCodeEditor.moc"



CodeEditor::CodeEditor(QWidget *parent, QTextEdit *lineNumberArea) : QTextEdit(parent), _lineNumberArea(lineNumberArea), _shiftLineNumber(0)
{
    setFontFamily("monospace");
    setTabStopWidth(30);
    highlightCurrentLine();

    // highlight syntaxt of code area
    GlslSyntaxHighlighter *highlighter = new GlslSyntaxHighlighter((QTextEdit *)this);
    highlighter->rehighlight ();

    // synchronize scrolling with line numbers area
    connect(verticalScrollBar(), SIGNAL(valueChanged(int)), _lineNumberArea->verticalScrollBar(), SLOT(setValue(int)));

    // update line numbers
    connect(this, SIGNAL(textChanged()), this, SLOT(updateLineNumbers()));

    // highlight line if editable
    if (!isReadOnly())
        connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(highlightCurrentLine()));

}

void CodeEditor::highlightCurrentLine()
{
    // use extra selection mechanism for highligt line
    QList<QTextEdit::ExtraSelection> extraSelections;
    // only editable text edit higlight line for edit
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;

        selection.format.setBackground(QColor(Qt::gray));
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }
    // apply highlight line (empty if not set)
    setExtraSelections(extraSelections);

    // needed to fix a display bug
    setFontFamily("monospace");
    setTabStopWidth(30);

    // rescroll line area in case the change of line caused a scroll in code area
    _lineNumberArea->verticalScrollBar()->setValue(verticalScrollBar()->value());
}

void CodeEditor::updateLineNumbers ()
{

    QString linenumbers;
    QTextBlock block = document()->firstBlock();
    while (block.isValid()) {
        // compute number of lines taken by the block
        QRectF r = document()->documentLayout()->blockBoundingRect(block) ;
        int n = (int) ( r.height() / (float) fontMetrics().height() );

        // insert line number
        linenumbers.append( QString::number(block.blockNumber() + _shiftLineNumber + 1) );
        // append lines where block is more than 1 line
        linenumbers.append( QString().fill('\n', n));

        // loop next block of text
        block = block.next();
    }

    _lineNumberArea->setPlainText(linenumbers);
}

GLSLCodeEditor::GLSLCodeEditor(QWidget *parent) : QWidget(parent)
{
    // layout to place two Code editors
    QHBoxLayout *horizontalLayout = new QHBoxLayout(this);
    horizontalLayout->setSpacing(0);
    horizontalLayout->setContentsMargins(0, 0, 0, 0);

    // Code editor special for line numbers
    lineNumberArea = new QTextEdit(this);
    lineNumberArea->setObjectName(QString::fromUtf8("lineNumberArea"));
    lineNumberArea->setMaximumWidth(40);
    lineNumberArea->setStyleSheet(QString::fromUtf8("background-color: rgb(88, 88, 88);\n"
    "color: white;"));
    lineNumberArea->setFrameShape(QFrame::NoFrame);
    lineNumberArea->setReadOnly(true);
    lineNumberArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    lineNumberArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    horizontalLayout->addWidget(lineNumberArea);

    codeArea = new CodeEditor( this, lineNumberArea);
    codeArea->setObjectName(QString::fromUtf8("codeArea"));
    codeArea->setFrameShape(QFrame::NoFrame);

    horizontalLayout->addWidget(codeArea);

}



void GLSLCodeEditor::setCode(QString code)
{
    codeArea->setText(code);
}

QString GLSLCodeEditor::code()
{
    return codeArea->toPlainText();
}

void GLSLCodeEditor::clear()
{
    codeArea->clear();
    lineNumberArea->clear();
}

int GLSLCodeEditor::lineCount()
{
    return codeArea->document()->lineCount();
}


void GLSLCodeEditor::setReadOnly(bool on)
{
    codeArea->setReadOnly(on);
    codeArea->setLineWrapMode(QTextEdit::NoWrap);
}

//int GLSLCodeEditor::lineNumberAreaWidth()
//{
//    int digits = 3;
//    int max = qMax(1, document()->blockCount());
//    while (max >= 10) {
//        max /= 10;
//        ++digits;
//    }

//    int space = 5 + fontMetrics().width(QLatin1Char('0')) * digits;

//    return space;
//}



//void GLSLCodeEditor::updateLineNumberAreaWidth(int /* newBlockCount */)
//{
//    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
//}



//void GLSLCodeEditor::updateLineNumberArea(const QRect &rect, int dy)
//{
//    if (dy)
//        lineNumberArea->scroll(0, dy);
//    else
//        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

//    if (rect.contains(viewport()->rect()))
//        updateLineNumberAreaWidth(0);
//}



//void GLSLCodeEditor::resizeEvent(QResizeEvent *e)
//{
//    QTextEdit::resizeEvent(e);

//    QRect cr = contentsRect();
//    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
//}






//void GLSLCodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
//{
//    QPainter painter(lineNumberArea);
//    painter.fillRect(event->rect(), Qt::lightGray);


//    QTextBlock block = document()->firstBlock();
//    int blockNumber = block.blockNumber();

//    int top = (int) document()->documentMargin ();
////    int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
//    int bottom = top + (int) document()->documentLayout()->blockBoundingRect(block).height();

//    while (block.isValid() && top <= event->rect().bottom()) {
//        if (block.isVisible() && bottom >= event->rect().top()) {
//            QString number = QString::number(blockNumber + 1 + _shiftLineNumber);
//            painter.setPen(Qt::black);
//            painter.drawText(0, top, lineNumberArea->width() - 2, fontMetrics().height(),
//                             Qt::AlignRight, number);
//        }

//        block = block.next();
//        top = bottom;
//        bottom = top + (int) document()->documentLayout()->blockBoundingRect(block).height();
//        ++blockNumber;
//    }
//}
