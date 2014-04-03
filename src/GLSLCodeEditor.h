#ifndef GLSLCODEEDITOR_H
#define GLSLCODEEDITOR_H

#include <QTextEdit>
#include <QScrollBar>


class CodeEditor : public QTextEdit
{
    Q_OBJECT

public:
    CodeEditor(QWidget *parent, QTextEdit *lineNumberArea);

    // shift in line number display, 0 for none
    void setShiftLineNumber(int i) { _shiftLineNumber = i; }

    // past code as plain text
    void insertFromMimeData ( const QMimeData * source ) {
        insertPlainText ( source->text() );
    }

    // allow wheel scrolling only for main code editor
    void wheelEvent ( QWheelEvent * e ) {
        if (_lineNumberArea)
            QTextEdit::wheelEvent(e);
    }

    // update line numbers when resized (in case block height changed)
    void resizeEvent ( QResizeEvent * e ) {
        if (_lineNumberArea)
            updateLineNumbers();
        QTextEdit::resizeEvent(e);
    }

public slots:
    // update the line numbers area
    void updateLineNumbers();

private slots:
    void highlightCurrentLine();

private:
    // associated line Number area, if any
    QTextEdit *_lineNumberArea;
    // shift in line numbers (to start from a non-zero line number)
    int _shiftLineNumber;
};


class GLSLCodeEditor : public QWidget
{
    Q_OBJECT

public:
    GLSLCodeEditor(QWidget *parent = 0);

    // 'ala' QTextEdit API
    void setCode(QString text);
    QString code();
    void clear();
    void setReadOnly(bool);

    // line count management
    int lineCount();
    void setShiftLineNumber(int i) { codeArea->setShiftLineNumber(i); }

private:
    CodeEditor *codeArea;
    QTextEdit *lineNumberArea;
};


#endif // GLSLCODEEDITOR_H
