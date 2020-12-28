#ifndef STATEOFHEALTH_H
#define STATEOFHEALTH_H

#include "common.h"

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QTextCodec>
#include <QMutex>

namespace Ui {
class StateOfHealth;
}

class StateOfHealth : public QDialog
{
    Q_OBJECT

public:
    explicit StateOfHealth(QVector<_STATION> initStaVT, QWidget *parent = nullptr);
    ~StateOfHealth();

    void resetInUseInfo(QVector<_STATION>);
    void updateInfo(QVector<_STATION>, int);
    int datatime;

private:
    Ui::StateOfHealth *ui;

    QTextCodec *codec;
    QMutex mutex;

    QVector<_STATION> staVT;
    QVector<_NETWORK> netVT;

    QStringList networkList;
    int numNetwork;
    QLabel *netLB[MAX_NUM_NETWORK];
    QPushButton *staPB[MAX_NUM_STATION];

    void updateTable(int);
    int myIndex;

private slots:
    void doRepeatWork();
    void staPBClicked(bool);
};

#endif // STATEOFHEALTH_H
