#include "RenderingManager.h"
#include "SelectionManager.h"

#include "WorkspaceManager.moc"

WorkspaceManager *WorkspaceManager::_instance = 0;


WorkspaceManager *WorkspaceManager::getInstance() {

    if (_instance == 0) {
        _instance = new WorkspaceManager;
        Q_CHECK_PTR(_instance);
    }

    return _instance;
}


WorkspaceManager::WorkspaceManager() : QObject(), _count(0), _actions(0), _sourceActions(0), _exclusive(false)
{
    // create action list with N workspaces
    _actions = new QActionGroup(this);
    _sourceActions = new QActionGroup(this);
    for (int i=0; i < WORKSPACE_MAX; ++i) {
        QIcon icon;
        icon.addPixmap( getPixmap(i+1, true), QIcon::Normal, QIcon::On);
        icon.addPixmap( getPixmap(i+1, false), QIcon::Normal, QIcon::Off);

        QAction *a = _actions->addAction(icon, tr("Workspace %1").arg(i+1));
        a->setShortcut( QKeySequence( Qt::Key_1 + i) );
        a->setCheckable(true);
        a->setData(i);

        QAction *sa = _sourceActions->addAction(icon, QString("Workspace %1").arg(i+1));
        sa->setCheckable(true);
        sa->setData(i);

        QToolButton *b = new QToolButton;
        b->setDefaultAction(a);
        _buttons.append(b);
    }

    _actions->actions()[0]->setChecked(true);
    _sourceActions->actions()[0]->setChecked(true);

    QObject::connect(_actions, SIGNAL(triggered(QAction *)), this, SLOT(onWorkspaceAction(QAction *) ) );
    QObject::connect(_sourceActions, SIGNAL(triggered(QAction *)), this, SLOT(onSourceWorkspaceAction(QAction *) ) );

    // default num workspace
    setCount();
}


void WorkspaceManager::incrementCount()
{
    setCount(_count+1);
}

void WorkspaceManager::decrementCount()
{
    setCount(_count-1);
}

void WorkspaceManager::setCount(int n)
{
    // minimum is 2 workspaces
    _count = qBound(WORKSPACE_MIN, n, WORKSPACE_MAX);

    // show active actions
    int i=0;
    for (; i < _count; ++i) {
        _actions->actions()[i]->setVisible(true);
        _sourceActions->actions()[i]->setVisible(true);
        _buttons[i]->setVisible(true);
    }
    // hide inactive actions
    for (; i < WORKSPACE_MAX; ++i) {

        _actions->actions()[i]->setVisible(false);
        _sourceActions->actions()[i]->setVisible(false);
        _buttons[i]->setVisible(false);
    }

    // adjust current if beyond count
    if ( current() > _count -1 )
        setCurrent();

    // broadcast
    emit countChanged(_count);
}


int WorkspaceManager::current() const
{
    return _actions->checkedAction()->data().toInt();
}

void WorkspaceManager::setCurrent(int n)
{
    n = qBound(0, n, _count - 1);
    if ( n != current()) {
        // select corresponding action
        _actions->actions()[n]->setChecked(true);
        // broadcast
        emit currentChanged(n);
    }
}


void  WorkspaceManager::onWorkspaceAction(QAction *a)
{
    RenderingManager::getInstance()->unsetCurrentSource();
    SelectionManager::getInstance()->clearSelection();

    // in exclusive mode, select all sources in workspace
    if (_exclusive) {
        SelectionManager::getInstance()->selectAll();
    }
}

void  WorkspaceManager::onSourceWorkspaceAction(QAction *a)
{
    int w = a->data().toInt();

    if (RenderingManager::getInstance()->setWorkspaceCurrentSource(w) )
        setCurrent(w);

}


QPixmap WorkspaceManager::getPixmap(int index, bool active)
{
    int s = 64;
    int b = 2;

    QSize size(s, s);
    QPixmap pix(size);
    pix.fill(Qt::transparent);

    QBrush brush(QColor(78, 78, 78));
    QRect rect = pix.rect();
    rect.adjust(b, b, -b, -b);
    rect.moveCenter(pix.rect().center());

    // Draw the workspace icon
    QPainter painter(&pix);
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.setPen(QPen(brush, 2));
    if (!active)
        painter.setBrush(brush.color().lighter(200));
    else
        painter.setBrush(Qt::white);
    painter.drawRoundedRect(rect, s / 4, s / 4);

    painter.setFont(QFont(getMonospaceFont(), s / 2 + 2));
    painter.drawText(rect, Qt::AlignCenter, QString::number(index));

//    pix.save(tr("ws_%1_%2.png").arg(index).arg(active));

    return pix;
}
