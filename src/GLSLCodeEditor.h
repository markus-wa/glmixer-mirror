#ifndef GLSLCODEEDITOR_H
#define GLSLCODEEDITOR_H

#include <QTextEdit>
class LineNumberArea;


class GLSLCodeEditor : public QTextEdit
{
    Q_OBJECT

public:
    GLSLCodeEditor(QWidget *parent = 0);

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();

    void setShiftLineNumber(int i) { _shiftLineNumber = i; }
    int lineCount() { return document()->blockCount(); }

protected:
    void resizeEvent(QResizeEvent *event);

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &, int);

private:
    QWidget *lineNumberArea;
    int _shiftLineNumber;
};


#endif // GLSLCODEEDITOR_H
