#ifndef SETSTATION_H
#define SETSTATION_H

#include "common.h"

#include <QDialog>
#include <QQuickView>
#include <QtQuick>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QTextCodec>
#include <QTableWidget>
#include <QHeaderView>
#include <QMessageBox>

namespace Ui {
class SetStation;
}

class SetStation : public QDialog
{
    Q_OBJECT

public:
    explicit SetStation(QVector<_STATION> initStaVT, QWidget *parent = nullptr);
    ~SetStation();

    void setup(QVector<_STATION>);

private:
    Ui::SetStation *ui;

    QTextCodec *codec;

    QObject *obj;
    QVariant returnedValue;
    QWidget *mapContainer;

    QVector<_STATION> staVT;

    QStringList networkList;
    QStringList netBoldStatus;
    QLabel *netLB[MAX_NUM_NETWORK];
    QTableWidget *netTW[MAX_NUM_NETWORK];
    QPushButton *netBoldPB[MAX_NUM_NETWORK];
    int numNetwork;

    void setCheckBoxInTable(int);
    void getNumInUseStation();

private slots:
    void netBoldPBClicked(bool);
};

#endif // SETSTATION_H
