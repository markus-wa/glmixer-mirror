#include "HistoryManagerWidget.moc"



HistoryModel::HistoryModel(History *h, QWidget *parent) : QAbstractTableModel(parent)
{
    _history = h;
}


int HistoryModel::rowCount(const QModelIndex &parent) const
{
    if (_history)
        return _history->size();

    return 0;
}

int HistoryModel::columnCount(const QModelIndex &parent) const
{
    return 5;
}

QVariant HistoryModel::data(const QModelIndex &index, int role) const
{
//    static QFont standardFont("Monospace", QFont().pointSize() * 0.8);
//    static QFont keyFont("Monospace", QFont().pointSize() * 0.8, QFont::Bold, true);

    if (!index.isValid() || !_history)
           return QVariant();

    QVariant returnvalue;
    if (role == Qt::TextAlignmentRole) {

        returnvalue = int(Qt::AlignRight | Qt::AlignVCenter);
    }
    else if (role == Qt::DisplayRole) {

        History::EventMap::iterator i = _history->begin() + index.row();

        if ( index.column() == 0 )
            returnvalue = QString::number( (double) i.key() / 1000.0, 'f', 3);
        else if ( index.column() == 1 )
            returnvalue = i.value().objectName();
        else if ( index.column() == 2 )
            returnvalue = i.value().signature().section('(',0,0);
        else if ( index.column() == 3 )
            returnvalue = i.value().arguments(History::BACKWARD);
        else if ( index.column() == 4 )
            returnvalue = i.value().arguments(History::FORWARD);

    }

    return returnvalue;
}

QVariant HistoryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    QVariant returnvalue;
    if ( orientation == Qt::Horizontal ) {
        static QStringList header  = QStringList() << "t" << "object" << "method" << "before" << "after";
        returnvalue = header.at(section);
    }
//    else if ( orientation == Qt::Vertical ) {
//        returnvalue = section;
//    }

    return returnvalue;
}

HistoryWidget::HistoryWidget(History *h, QWidget *parent) : QTableView(parent)
{
    _historyModel = new HistoryModel(h, this);
    setModel(_historyModel);

    resizeColumnsToContents();
//    setAutoScroll(true);

//    setColumnWidth(0, 60);
//    setColumnWidth(1, 100);
//    setColumnWidth(2, 180);
//    setColumnWidth(3, 220);
//    setColumnWidth(4, 220);

}
