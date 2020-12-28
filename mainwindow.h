#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QQuickView>
#include <QtQuick>
#include <QMessageBox>
#include <QTextCodec>
#include <QProgressDialog>
#include <QTimer>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QSqlRecord>
#include <QSqlError>

#include "common.h"
#include "writelog.h"
#include "recvmessage.h"
#include "configuration.h"
#include "setstation.h"
#include "stateofhealth.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QString configFile = nullptr, QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent* event);
    void closeEvent(QCloseEvent *event);

private:
    Ui::MainWindow *ui;

    QTextCodec *codec;
    _CONFIGURE cfg;

    bool changedTitle;
    QTimer *titleTimer;

    WriteLog *log;
    Configuration *configuration;
    StateOfHealth *soh;

    RecvMessage *recvkissmessage;
    RecvMessage *recvmpssmessage;
    RecvEEWMessage *recveew;
    RecvUpdateMessage *recvupdatemessage;

    QVector<_STATION> staListVT;
    QVector<QString> staVTForGetIndex;
    QMultiMap<int, _QSCD_FOR_MULTIMAP> dataHouse;
    QMutex mutex;
    QTimer *systemTimer;

    QQuickView *realTimeMapView;
    QQuickView *stackedMapView;
    QObject *realTimeObj;
    QObject *stackedObj;
    QVariant returnedValue;
    QWidget *realTimeMapContainer;
    QWidget *stackedMapContainer;

    QVector<_EVENT> eventV;
    bool isEvent;
    int eventTime;
    QString evtFileFullName;
    int evtDuration;
    int evtDataTime;
    int evtStartTime;
    _EEWInfo publicEEWInfo;

    QLabel *eventNetLB[MAX_NUM_EVENTINITSTA];
    QLabel *eventStaLB[MAX_NUM_EVENTINITSTA];
    QLabel *eventComLB[MAX_NUM_EVENTINITSTA];
    QLabel *eventTimeLB[MAX_NUM_EVENTINITSTA];
    QLabel *eventPGALB[MAX_NUM_EVENTINITSTA];

    void makeEvent(_STATION);
    void checkEvent(QDateTime);
    void findMaxStations();

    void openDB();
    // About Database & table
    QSqlDatabase mydb;
    QSqlQueryModel *networkModel;
    QSqlQueryModel *affiliationModel;
    QSqlQueryModel *siteModel;
    QSqlQueryModel *criteriaModel;

    void decorationGUI();
    void initMap();
    void readCFG();
    void readStationListfromDB(bool);
    void readCriteriafromDB();
    void changedChannel();

    void createStaCircleOnMap();
    void resetStaCircleOnRealTimeMap();
    void resetEvent();

    void changeCircleOnMap(QString, int, int, QColor, int);

private slots:
    void rvEEWInfo(_EEWInfo);
    void doRepeatWork();
    void showConfiguration();
    void showSetStation();
    void showSOH();
    void extractQSCD(QMultiMap<int, _QSCD_FOR_MULTIMAP>);
    void recvUpdateMessage(_UPDATE_MESSAGE);
    void recvConFromConfigure(_CONFIGURE);
    void _qmlSignalfromRealTimeMap(QString);
    void _qmlSignalfromStackedMap(QString);
    void resetEventGUI();
    void animationTitle();
};

#endif // MAINWINDOW_H
