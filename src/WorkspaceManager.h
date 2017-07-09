#ifndef WORKSPACEMANAGER_H
#define WORKSPACEMANAGER_H

#include <QObject>
#include <QActionGroup>
#include <QToolButton>

#define WORKSPACE_MIN 2
#define WORKSPACE_DEFAULT 3
#define WORKSPACE_MAX 9

#define WORKSPACE_MAX_ALPHA 0.4f
#define WORKSPACE_COLOR_SHIFT 130

class WorkspaceManager : public QObject
{
    Q_OBJECT

public:
    static WorkspaceManager *getInstance();
    static QPixmap getPixmap(int index, bool active = true);

    int current() const;
    int count() const { return _count; }
    bool isExclusiveDisplay() { return _exclusive; }

    QList<QAction *> getActions() const { return _actions->actions(); }
    QList<QToolButton *> getButtons() const { return _buttons; }
    QList<QAction *> getSourceActions() const { return _sourceActions->actions(); }

signals:
    void currentChanged(int);
    void countChanged(int);

public slots:

    void setCount(int n = WORKSPACE_DEFAULT);
    void incrementCount();
    void decrementCount();
    void setCurrent(int n = WORKSPACE_MAX);
    void setExclusiveDisplay(bool on) { _exclusive = on; }

    void onWorkspaceAction(QAction *a);
    void onSourceWorkspaceAction(QAction *a);

private:
    WorkspaceManager();
    static WorkspaceManager *_instance;

    int _count;
    QActionGroup *_actions;
    QActionGroup *_sourceActions;
    QList<QToolButton *> _buttons;
    bool _exclusive;
};

#endif // WORKSPACEMANAGER_H
