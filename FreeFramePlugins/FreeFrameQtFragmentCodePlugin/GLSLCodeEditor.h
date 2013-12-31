#ifndef GLSLCODEEDITOR_H
#define GLSLCODEEDITOR_H

#include <QTextEdit>
class LineNumberArea;


class GlslCodeEditor : public QTextEdit
{
    Q_OBJECT

public:
    GlslCodeEditor(QWidget *parent = 0);

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();

protected:
    void resizeEvent(QResizeEvent *event);

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &, int);

private:
    QWidget *lineNumberArea;
};


#endif // GLSLCODEEDITOR_H
