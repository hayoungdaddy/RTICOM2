#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "QtConcurrent/qtconcurrentrun.h"

MainWindow::MainWindow(QString configFile, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    if(configFile.startsWith("--version") || configFile.startsWith("-version"))
    {
        qDebug() << "--- RTICOM2CORE ---";
        qDebug() << "Version " + QString::number(VERSION, 'f', 1) + " (2019-05-27)";
        qDebug() << "From KIGAM KERC.";
        exit(1);
    }

    /*
    QString temp = "rm -rf /home/sysop/.cache/QtLocation/5.8/tiles/osm/*";
    system(temp.toLatin1().data());
    temp = "rm -rf /home/sysop/.cache/RTICOM2/qmlcache";
    system(temp.toLatin1().data());
    */

    ui->setupUi(this);

    activemq::library::ActiveMQCPP::initializeLibrary();

    cfg.configFileName = configFile;
    codec = QTextCodec::codecForName("utf-8");
    log = new WriteLog();

    qRegisterMetaType< QMultiMap<int,_QSCD_FOR_MULTIMAP> >("QMultiMap<int,_QSCD_FOR_MULTIMAP>");
    qRegisterMetaType<_EEWInfo>("_EEWInfo");
    qRegisterMetaType<_UPDATE_MESSAGE>("_UPDATE_MESSAGE");

    decorationGUI();
    readCFG();

    mydb = QSqlDatabase::addDatabase("QMYSQL");
    mydb.setHostName(cfg.db_ip);
    mydb.setDatabaseName(cfg.db_name);
    mydb.setUserName(cfg.db_user);
    mydb.setPassword(cfg.db_passwd);

    networkModel = new QSqlQueryModel();
    affiliationModel = new QSqlQueryModel();
    siteModel = new QSqlQueryModel();
    criteriaModel = new QSqlQueryModel();

    changedTitle = false;
    titleTimer = new QTimer(this);
    titleTimer->start(2000);
    connect(titleTimer, SIGNAL(timeout()), this, SLOT(animationTitle()));

    readStationListfromDB(false);
    readCriteriafromDB();

    changedChannel();
    initMap();

    createStaCircleOnMap();   
    resetEvent();

    dataHouse.clear();
    systemTimer = new QTimer(this);
    systemTimer->start(1000);
    connect(systemTimer, SIGNAL(timeout()), this, SLOT(doRepeatWork()));

    configuration = new Configuration(this);
    configuration->hide();
    connect(ui->conPB, SIGNAL(clicked()), this, SLOT(showConfiguration()));
    connect(configuration, SIGNAL(sendCFGtoMain(_CONFIGURE)), this, SLOT(recvConFromConfigure(_CONFIGURE)));

    connect(ui->setStaPB, SIGNAL(clicked()), this, SLOT(showSetStation()));
    connect(ui->refrashPB, SIGNAL(clicked()), this, SLOT(resetEventGUI()));

    soh = new StateOfHealth(staListVT);
    soh->resetInUseInfo(staListVT);
    soh->hide();
    connect(ui->sohPB, SIGNAL(clicked()), this, SLOT(showSOH()));

    QProgressDialog progress(codec->toUnicode("시작 준비중. 서버 연결중입니다."), codec->toUnicode("취소"), 0, 4, this);
    progress.setWindowTitle(codec->toUnicode("RTICOM2"));
    progress.setMinimumWidth(700);
    progress.setWindowModality(Qt::WindowModal);

    // consumer
    if(cfg.kiss_amq_topic != "")
    {
        QString kissFailover = "failover:(tcp://" + cfg.kiss_amq_ip + ":" + cfg.kiss_amq_port + "?wireFormat.tightEncodingEnabled=true&wireFormat.cacheEnabled=true)";

        recvkissmessage = new RecvMessage(this);
        if(!recvkissmessage->isRunning())
        {
            QString temp = "Connect to " + kissFailover;
            qDebug() << temp;
            log->write(cfg.logDir, temp);
            recvkissmessage->setup(kissFailover, cfg.kiss_amq_user, cfg.kiss_amq_passwd, cfg.kiss_amq_topic, true, false);
            connect(recvkissmessage, SIGNAL(sendQSCDtoMain(QMultiMap<int, _QSCD_FOR_MULTIMAP>)), this, SLOT(extractQSCD(QMultiMap<int, _QSCD_FOR_MULTIMAP>)));
            recvkissmessage->start();
            progress.setValue(1);
        }
    }

    if(cfg.mpss_amq_topic != "")
    {
        QString mpssFailover = "failover:(tcp://" + cfg.mpss_amq_ip + ":" + cfg.mpss_amq_port + "?wireFormat.tightEncodingEnabled=true&wireFormat.cacheEnabled=true)";

        recvmpssmessage = new RecvMessage(this);
        if(!recvmpssmessage->isRunning())
        {
            QString temp = "Connect to " + mpssFailover;
            qDebug() << temp;
            log->write(cfg.logDir, temp);
            recvmpssmessage->setup(mpssFailover, cfg.mpss_amq_user, cfg.mpss_amq_passwd, cfg.mpss_amq_topic, true, false);
            connect(recvmpssmessage, SIGNAL(sendQSCDtoMain(QMultiMap<int, _QSCD_FOR_MULTIMAP>)), this, SLOT(extractQSCD(QMultiMap<int, _QSCD_FOR_MULTIMAP>)));
            recvmpssmessage->start();
            progress.setValue(2);
        }
    }

    if(cfg.eew_amq_topic != "")
    {
        QString eewFailover = "failover:(tcp://" + cfg.eew_amq_ip + ":" + cfg.eew_amq_port + ")";

        recveew = new RecvEEWMessage(this);
        if(!recveew->isRunning())
        {
            QString temp = "Connect to " + eewFailover;
            qDebug() << temp;
            log->write(cfg.logDir, temp);
            recveew->setup(eewFailover, cfg.eew_amq_user, cfg.eew_amq_passwd, cfg.eew_amq_topic, true, false);
            connect(recveew, SIGNAL(_rvEEWInfo(_EEWInfo)), this, SLOT(rvEEWInfo(_EEWInfo)));
            recveew->start();
            progress.setValue(3);
        }
    }

    if(cfg.update_amq_topic != "")
    {
        QString updateFailover = "failover:(tcp://" + cfg.update_amq_ip + ":" + cfg.update_amq_port + ")";

        recvupdatemessage = new RecvUpdateMessage(this);
        if(!recvupdatemessage->isRunning())
        {
            QString temp = "Connect to " + updateFailover;
            qDebug() << temp;
            log->write(cfg.logDir, temp);
            recvupdatemessage->setup(updateFailover, cfg.update_amq_user, cfg.update_amq_passwd, cfg.update_amq_topic, true, false);
            connect(recvupdatemessage, SIGNAL(_rvUpdateInfo(_UPDATE_MESSAGE)), this, SLOT(recvUpdateMessage(_UPDATE_MESSAGE)));
            recvupdatemessage->start();
            progress.setValue(4);
        }
    }

    log->write(cfg.logDir, "======================================================");
    log->write(cfg.logDir, "RTICOM2 Started.");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::animationTitle()
{
    if(!changedTitle)
    {
        changedTitle = true;
        QPixmap titlePX("/.RTICOM2/images/RTICOM2Logo_W.png");
        ui->titleLB->setPixmap(titlePX.scaled(ui->titleLB->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    }
    else
    {
        changedTitle = false;
        QPixmap titlePX("/.RTICOM2/images/RTICOM2Logo_G.png");
        ui->titleLB->setPixmap(titlePX.scaled(ui->titleLB->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    }
}

void MainWindow::initMap()
{
    QVariantMap params
    {
        /*
        {"osm.mapping.cache.disk.cost_strategy", 0 },
        {"osm.mapping.cache.disk.size", 0 },
        {"osm.mapping.cache.memory.cost_strategy", 0},
        {"osm.mapping.cache.memory.size", 0 },
        */
        {"osm.mapping.highdpi_tiles", true},
        {"osm.mapping.offline.directory", cfg.mapType}
    };

    realTimeMapView = new QQuickView();
    realTimeMapContainer = QWidget::createWindowContainer(realTimeMapView, this);
    realTimeMapView->setResizeMode(QQuickView::SizeRootObjectToView);
    realTimeMapContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    realTimeMapContainer->setFocusPolicy(Qt::TabFocus);
    realTimeMapView->setSource(QUrl(QStringLiteral("qrc:/ViewMap.qml")));
    ui->realTimeMapLO->addWidget(realTimeMapContainer);
    realTimeObj = realTimeMapView->rootObject();

    stackedMapView = new QQuickView();
    stackedMapContainer = QWidget::createWindowContainer(stackedMapView, this);
    stackedMapView->setResizeMode(QQuickView::SizeRootObjectToView);
    stackedMapContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    stackedMapContainer->setFocusPolicy(Qt::TabFocus);
    stackedMapView->setSource(QUrl(QStringLiteral("qrc:/ViewMap.qml")));

    ui->stackedMapLO->addWidget(stackedMapContainer);
    stackedObj = stackedMapView->rootObject();

    QObject::connect(this->realTimeObj, SIGNAL(sendStationIndexSignal(QString)), this, SLOT(_qmlSignalfromRealTimeMap(QString)));
    QObject::connect(this->stackedObj, SIGNAL(sendStationIndexSignal(QString)), this, SLOT(_qmlSignalfromStackedMap(QString)));

    QMetaObject::invokeMethod(realTimeObj, "initializePlugin", Q_ARG(QVariant, QVariant::fromValue(params)));
    QMetaObject::invokeMethod(stackedObj, "initializePlugin", Q_ARG(QVariant, QVariant::fromValue(params)));

    QMetaObject::invokeMethod(this->realTimeObj, "createRealTimeMapObject", Q_RETURN_ARG(QVariant, returnedValue));
    QMetaObject::invokeMethod(this->stackedObj, "createStackedMapObject", Q_RETURN_ARG(QVariant, returnedValue));
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    /*
    ui->mainFR1->setGeometry(5, 60, (this->width()-470-15)/2, ui->mainFR1->height());
    ui->mainFR2->setGeometry(10+(this->width()-470-15)/2, 60, (this->width()-470-15)/2, ui->mainFR2->height());
    */
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if( !QMessageBox::question( this,
                                codec->toUnicode("프로그램 종료"),
                                codec->toUnicode("프로그램을 종료합니다."),
                                codec->toUnicode("종료"),
                                codec->toUnicode("취소"),
                                QString::null, 1, 1) )
    {
        recvkissmessage->requestInterruption();
        recvmpssmessage->requestInterruption();

        log->write(cfg.logDir, "RTICOM2 Terminated.");
        event->accept();
    }
    else
    {
        event->ignore();
    }
}

void MainWindow::recvUpdateMessage(_UPDATE_MESSAGE message)
{
    if(QString(message.MType).startsWith("S"))
    {
        log->write(cfg.logDir, "Recevied a update message for stations list.");
        log->write(cfg.logDir, "Restart RTICOM2 after reloading a stations list.");
        readStationListfromDB(true);
        soh->resetInUseInfo(staListVT);
        createStaCircleOnMap();       
        resetEvent();

        QDateTime timeKST;
        QDateTime time = QDateTime::currentDateTimeUtc();
        timeKST = convertKST(time);

        QMessageBox msgBox;
        msgBox.setText(codec->toUnicode("관측소 정보가 변경되었습니다.") + timeKST.toString("(yyyy-MM-dd hh:mm:ss)"));
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.button(QMessageBox::Ok)->animateClick(5000);
        msgBox.exec();
    }
    else if(QString(message.MType).startsWith("C"))
    {
        log->write(cfg.logDir, "Recevied a update message for event criteria.");
        readCriteriafromDB();
        resetEvent();

        QDateTime timeKST;
        QDateTime time = QDateTime::currentDateTimeUtc();
        timeKST = convertKST(time);

        QMessageBox msgBox;
        msgBox.setText(codec->toUnicode("이벤트 발생 조건이 변경되었습니다.") + timeKST.toString("(yyyy-MM-dd hh:mm:ss)"));
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.button(QMessageBox::Ok)->animateClick(5000);
        msgBox.exec();
    }
    else if(QString(message.MType).startsWith("P"))
    {
        log->write(cfg.logDir, "Recevied new version RTICOM2.");
        QDir dir(cfg.homeDir + "/bin/OLD_VERSION");
        if(!dir.exists())
        {
            dir.mkpath(".");
        }
        QProcess process;
        QString cmd = "cp " + cfg.homeDir + "/bin/RTICOM2 " + cfg.homeDir + "/bin/OLD_VERSION/RTICOM2_V" + QString::number(VERSION, 'f', 1);
        process.start(cmd);
        process.waitForFinished(-1);

        cmd = "scp kisstool@210.98.8.76:/kisstool/data/RTICOM2/bin/RTICOM2_V" + QString::number(message.value, 'f', 1) + cfg.homeDir + "/bin/";
        process.start(cmd);
        process.waitForFinished(-1);

        cmd = cfg.homeDir + "/bin/updateRTICOM2.sh  " + QString::number(VERSION, 'f', 1) + " " + QString::number(message.value, 'f', 1) + " " + cfg.homeDir;
        process.start(cmd);
        process.waitForFinished(-1);
    }
}

void MainWindow::extractQSCD(QMultiMap<int, _QSCD_FOR_MULTIMAP> mmFromAMQ)
{
    QMapIterator<int, _QSCD_FOR_MULTIMAP> i(mmFromAMQ);
    mutex.lock();
    while(i.hasNext())
    {
        i.next();
        dataHouse.insert(i.key(), i.value());
    }
    mutex.unlock();
}

void MainWindow::showConfiguration()
{
    configuration->setup(cfg);
    if(configuration->isHidden())
        configuration->show();
}

void MainWindow::showSetStation()
{
    SetStation *setstation = new SetStation(staListVT);
    setstation->setup(staListVT);
    if(setstation->isHidden())
        setstation->show();
}

void MainWindow::showSOH()
{
    if(soh->isHidden())
        soh->show();
}

void MainWindow::decorationGUI()
{
    QPixmap titlePX("/.RTICOM2/images/RTICOM2Logo_W.png");
    ui->titleLB->setPixmap(titlePX.scaled(ui->titleLB->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    //ui->titleLB->setPixmap(titlePX.scaled(ui->titleLB->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));

    QPixmap conPX("/.RTICOM2/images/configuration_disabled.png");
    QIcon conIC(conPX);
    ui->conPB->setIcon(conIC);
    ui->conPB->setIconSize(QSize(50, 50));

    //QPixmap manStaPX("/.RTICOM2/images/manStaIcon.png");
    QPixmap manStaPX("/.RTICOM2/images/setting_disabled.png");
    QIcon manStaIC(manStaPX);
    ui->setStaPB->setIcon(manStaIC);
    ui->setStaPB->setIconSize(QSize(50, 50));

    //QPixmap sohPX("/.RTICOM2/images/sohIcon.png");
    QPixmap sohPX("/.RTICOM2/images/soh_disabled.png");
    QIcon sohIC(sohPX);
    ui->sohPB->setIcon(sohIC);
    ui->sohPB->setIconSize(QSize(50, 50));

    //QPixmap refrashPX("/.RTICOM2/images/refrashIcon.png");
    QPixmap refrashPX("/.RTICOM2/images/refresh_disabled.png");
    QIcon refrashIC(refrashPX);
    ui->refrashPB->setIcon(refrashIC);
    ui->refrashPB->setIconSize(QSize(50, 50));

    for(int i=0;i<MAX_NUM_EVENTINITSTA;i++)
    {
        eventNetLB[i] = new QLabel;
        eventNetLB[i]->setFixedSize(65, 21);
        eventNetLB[i]->setAlignment(Qt::AlignCenter);
        eventNetLB[i]->setStyleSheet("background-color: rgb(243,243,243); color: rgb(46, 52, 54);");
        eventNetLB[i]->setFrameShape(QFrame::StyledPanel);
        ui->eventInitLO->addWidget(eventNetLB[i], i+1, 0);
        eventStaLB[i] = new QLabel;
        eventStaLB[i]->setFixedSize(70, 21);
        eventStaLB[i]->setAlignment(Qt::AlignCenter);
        eventStaLB[i]->setStyleSheet("background-color: rgb(243,243,243); color: rgb(46, 52, 54);");
        eventStaLB[i]->setFrameShape(QFrame::StyledPanel);
        ui->eventInitLO->addWidget(eventStaLB[i], i+1, 1);
        eventComLB[i] = new QLabel;
        eventComLB[i]->setFixedSize(163, 21);
        eventComLB[i]->setAlignment(Qt::AlignCenter);
        eventComLB[i]->setStyleSheet("background-color: rgb(243,243,243); color: rgb(46, 52, 54);");
        eventComLB[i]->setFrameShape(QFrame::StyledPanel);
        ui->eventInitLO->addWidget(eventComLB[i], i+1, 2);
        eventTimeLB[i] = new QLabel;
        eventTimeLB[i]->setFixedSize(80, 21);
        eventTimeLB[i]->setAlignment(Qt::AlignCenter);
        eventTimeLB[i]->setStyleSheet("background-color: rgb(243,243,243); color: rgb(46, 52, 54);");
        eventTimeLB[i]->setFrameShape(QFrame::StyledPanel);
        ui->eventInitLO->addWidget(eventTimeLB[i], i+1, 3);
        eventPGALB[i] = new QLabel;
        eventPGALB[i]->setFixedSize(80, 21);
        eventPGALB[i]->setAlignment(Qt::AlignCenter);
        eventPGALB[i]->setStyleSheet("background-color: rgb(243,243,243); color: rgb(46, 52, 54);");
        eventPGALB[i]->setFrameShape(QFrame::StyledPanel);
        ui->eventInitLO->addWidget(eventPGALB[i], i+1, 4);
    }

    ui->eventInitFR->show();

    ui->eventMaxPGATW->setColumnWidth(0, 60);
    ui->eventMaxPGATW->setColumnWidth(1, 60);
    ui->eventMaxPGATW->setColumnWidth(2, 140);
    ui->eventMaxPGATW->setColumnWidth(3, 80);
}

void MainWindow::recvConFromConfigure(_CONFIGURE con)
{
    cfg.monChanID = con.monChanID;
    changedChannel();
}

void MainWindow::changedChannel()
{
    if(cfg.monChanID == 0) ui->monChanLB->setText("Up/Down PGA");
    else if(cfg.monChanID == 1) ui->monChanLB->setText("North/South PGA");
    else if(cfg.monChanID == 2) ui->monChanLB->setText("East/West PGA");
    else if(cfg.monChanID == 3) ui->monChanLB->setText("Horizontal PGA");
    else if(cfg.monChanID == 4) ui->monChanLB->setText("Total PGA");
}

void MainWindow::readCFG()
{
    QFile file(cfg.configFileName);
    if(!file.exists())
    {
        qDebug() << "Failed configuration. Parameter file doesn't exist.";
        exit(1);
    }
    if(file.open(QIODevice::ReadOnly))
    {
        QTextStream stream(&file);
        QString line, _line;

        while(!stream.atEnd())
        {
            line = stream.readLine();
            _line = line.simplified();

            if(_line.startsWith(" ") || _line.startsWith("#"))
                continue;
            else if(_line.startsWith("HOME_DIR"))
            {
                cfg.homeDir = _line.section("=", 1, 1);
                cfg.logDir = cfg.homeDir + "/logs";
                cfg.eventDir = cfg.homeDir + "/events";
            }
            else if(_line.startsWith("DIFF_TIME"))
                cfg.timeDiffForMon = _line.section("=", 1, 1).toInt();

            else if(_line.startsWith("DB_IP"))
                cfg.db_ip = _line.section("=", 1, 1);
            else if(_line.startsWith("DB_NAME"))
                cfg.db_name = _line.section("=", 1, 1);
            else if(_line.startsWith("DB_USERNAME"))
                cfg.db_user = _line.section("=", 1, 1);
            else if(_line.startsWith("DB_PASSWD"))
                cfg.db_passwd = _line.section("=", 1, 1);

            else if(_line.startsWith("UPDATE_AMQ_IP"))
                cfg.update_amq_ip = _line.section("=", 1, 1);
            else if(_line.startsWith("UPDATE_AMQ_PORT"))
                cfg.update_amq_port = _line.section("=", 1, 1);
            else if(_line.startsWith("UPDATE_AMQ_USERNAME"))
                cfg.update_amq_user = _line.section("=", 1, 1);
            else if(_line.startsWith("UPDATE_AMQ_PASSWD"))
                cfg.update_amq_passwd = _line.section("=", 1, 1);
            else if(_line.startsWith("UPDATE_AMQ_TOPIC"))
                cfg.update_amq_topic = _line.section("=", 1, 1);

            else if(_line.startsWith("KISS_AMQ_IP"))
                cfg.kiss_amq_ip = _line.section("=", 1, 1);
            else if(_line.startsWith("KISS_AMQ_PORT"))
                cfg.kiss_amq_port = _line.section("=", 1, 1);
            else if(_line.startsWith("KISS_AMQ_USERNAME"))
                cfg.kiss_amq_user = _line.section("=", 1, 1);
            else if(_line.startsWith("KISS_AMQ_PASSWD"))
                cfg.kiss_amq_passwd = _line.section("=", 1, 1);
            else if(_line.startsWith("KISS_AMQ_TOPIC"))
                cfg.kiss_amq_topic = _line.section("=", 1, 1);

            else if(_line.startsWith("MPSS_AMQ_IP"))
                cfg.mpss_amq_ip = _line.section("=", 1, 1);
            else if(_line.startsWith("MPSS_AMQ_PORT"))
                cfg.mpss_amq_port = _line.section("=", 1, 1);
            else if(_line.startsWith("MPSS_AMQ_USERNAME"))
                cfg.mpss_amq_user = _line.section("=", 1, 1);
            else if(_line.startsWith("MPSS_AMQ_PASSWD"))
                cfg.mpss_amq_passwd = _line.section("=", 1, 1);
            else if(_line.startsWith("MPSS_AMQ_TOPIC"))
                cfg.mpss_amq_topic = _line.section("=", 1, 1);

            else if(_line.startsWith("EEW_AMQ_IP"))
                cfg.eew_amq_ip = _line.section("=", 1, 1);
            else if(_line.startsWith("EEW_AMQ_PORT"))
                cfg.eew_amq_port = _line.section("=", 1, 1);
            else if(_line.startsWith("EEW_AMQ_USERNAME"))
                cfg.eew_amq_user = _line.section("=", 1, 1);
            else if(_line.startsWith("EEW_AMQ_PASSWD"))
                cfg.eew_amq_passwd = _line.section("=", 1, 1);
            else if(_line.startsWith("EEW_AMQ_TOPIC"))
                cfg.eew_amq_topic = _line.section("=", 1, 1);

            else if(_line.startsWith("CHANNEL"))
            {
                if(_line.section("=", 1, 1).left(1).startsWith("Z")) cfg.monChanID = 0;
                else if(_line.section("=", 1, 1).startsWith("N")) cfg.monChanID = 1;
                else if(_line.section("=", 1, 1).startsWith("E")) cfg.monChanID = 2;
                else if(_line.section("=", 1, 1).startsWith("H")) cfg.monChanID = 3;
                else if(_line.section("=", 1, 1).startsWith("T")) cfg.monChanID = 4;
            }

            else if(_line.startsWith("MAP_TYPE"))
            {
                if(_line.section("=", 1, 1).startsWith("satellite")) cfg.mapType = "/.RTICOM2/map_data/mapbox-satellite";
                else if(_line.section("=", 1, 1).startsWith("outdoors")) cfg.mapType = "/.RTICOM2/map_data/mapbox-outdoors";
                else if(_line.section("=", 1, 1).startsWith("light")) cfg.mapType = "/.RTICOM2/map_data/mapbox-light";
                else if(_line.section("=", 1, 1).startsWith("street")) cfg.mapType = "/.RTICOM2/map_data/osm_street";
            }
        }
        file.close();
    }
}

void MainWindow::openDB()
{
    qDebug() << "Connect to the Database (" + mydb.hostName() + " (" + mydb.databaseName() + "))";
    log->write(cfg.logDir, "Connect to the Database (" + mydb.hostName() + " (" + mydb.databaseName() + "))");
    if(!mydb.open())
    {
        log->write(cfg.logDir, "Error connecting to DB: " + mydb.lastError().text());
    }
}

void MainWindow::readStationListfromDB(bool isUpdate)
{
    staListVT.clear();
    staVTForGetIndex.clear();

    QString query;
    query = "SELECT * FROM NETWORK";
    openDB();
    networkModel->setQuery(query);

    qDebug() << "Loading new stations list from the database.";
    log->write(cfg.logDir, "Loading new stations list from the database.");

    int count = 0;

    QString temp;
    if(isUpdate) temp = codec->toUnicode("관측소 정보를 갱신 중입니다.");
    else temp = codec->toUnicode("시작 준비중. 관측소 정보를 담는 중입니다.");

    QProgressDialog progress(temp, codec->toUnicode("취소"), 0, networkModel->rowCount(), this);
    progress.setWindowTitle(codec->toUnicode("RTICOM2 - 관측소 관리자"));
    progress.setMinimumWidth(700);
    progress.setWindowModality(Qt::WindowModal);

    if(networkModel->rowCount() > 0)
    {
        for(int i=0;i<networkModel->rowCount();i++)
        {
            progress.setValue(i);
            if(progress.wasCanceled())
            {
                exit(1);
            }
            QString network = networkModel->record(i).value("net").toString();
            query = "SELECT * FROM AFFILIATION where net='" + network + "'";
            affiliationModel->setQuery(query);

            for(int j=0;j<affiliationModel->rowCount();j++)
            {
                QString affiliation = affiliationModel->record(j).value("aff").toString();
                QString affiliationName = affiliationModel->record(j).value("affname").toString();
                double lat = affiliationModel->record(j).value("lat").toDouble();
                double lon = affiliationModel->record(j).value("lon").toDouble();
                double elev = affiliationModel->record(j).value("elev").toDouble();
                query = "SELECT * FROM SITE where aff='" + affiliation + "'";
                siteModel->setQuery(query);

                for(int k=0;k<siteModel->rowCount();k++)
                {
                    if(siteModel->record(k).value("statype").toString().startsWith("G"))
                    {
                        _STATION sta;
                        sta.index = count;
                        sta.sta = siteModel->record(k).value("sta").toString();
                        staVTForGetIndex.push_back(sta.sta);
                        sta.comment = affiliationName;
                        sta.lat = lat;
                        sta.lon = lon;
                        sta.elev = elev;
                        sta.staType = siteModel->record(k).value("statype").toString();
                        sta.inUse = siteModel->record(k).value("inuse").toInt();
                        sta.lastPGA = 0;
                        sta.lastPGATime = 0;
                        sta.maxPGA = 0;
                        sta.maxPGATime = 0;

                        staListVT.push_back(sta);
                        count++;
                    }
                }
            }
        }
        progress.setValue(networkModel->rowCount());
    }

    log->write(cfg.logDir, "Read Station List (Ground) from DB. Number of Station : " + QString::number(staListVT.count()));
}

void MainWindow::readCriteriafromDB()
{
    QString query;
    query = "SELECT * FROM CRITERIA";
    openDB();
    criteriaModel->setQuery(query);

    qDebug() << "Loading a Criteria for RTICOM2 from the database.";
    log->write(cfg.logDir, "Loading a Criteria for RTICOM2 from the database.");

    cfg.inSeconds = criteriaModel->record(0).value("inseconds").toInt();
    cfg.numSta = criteriaModel->record(0).value("numsta").toInt();
    cfg.thresholdG = criteriaModel->record(0).value("thresholdG").toDouble();
    cfg.distance = criteriaModel->record(0).value("distance").toInt();
    cfg.thresholdM = criteriaModel->record(0).value("thresholdM").toDouble();

    log->write(cfg.logDir, "Read Criteria List from DB.");
    log->write(cfg.logDir, "In seconds : " + QString::number(cfg.inSeconds));
    log->write(cfg.logDir, "Number of stations : " + QString::number(cfg.numSta));
    log->write(cfg.logDir, "Threshold(gal) : " + QString::number(cfg.thresholdG, 'f', 1));
    log->write(cfg.logDir, "Distance for each stations : " + QString::number(cfg.distance));
    log->write(cfg.logDir, "Threshold(eew magnitude) : " + QString::number(cfg.thresholdM, 'f', 1));
}

void MainWindow::createStaCircleOnMap()
{
    // create staCircle
    QMetaObject::invokeMethod(this->realTimeObj, "clearMap", Q_RETURN_ARG(QVariant, returnedValue));
    QMetaObject::invokeMethod(this->stackedObj, "clearMap", Q_RETURN_ARG(QVariant, returnedValue));

    for(int i=0;i<staListVT.count();i++)
    {
        if(staListVT.at(i).inUse == 1)
        {
            QMetaObject::invokeMethod(this->realTimeObj, "createStaCircle", Q_RETURN_ARG(QVariant, returnedValue),
                                      Q_ARG(QVariant, staListVT.at(i).index), Q_ARG(QVariant, staListVT.at(i).lat),
                                      Q_ARG(QVariant, staListVT.at(i).lon), Q_ARG(QVariant, 10), Q_ARG(QVariant, "white"),
                                      Q_ARG(QVariant, staListVT.at(i).sta.left(2) + "/" + staListVT.at(i).sta.mid(2)),
                                      Q_ARG(QVariant, 1));

            QMetaObject::invokeMethod(this->stackedObj, "createStaCircle", Q_RETURN_ARG(QVariant, returnedValue),
                                      Q_ARG(QVariant, staListVT.at(i).index), Q_ARG(QVariant, staListVT.at(i).lat),
                                      Q_ARG(QVariant, staListVT.at(i).lon), Q_ARG(QVariant, 10), Q_ARG(QVariant, "white"),
                                      Q_ARG(QVariant, staListVT.at(i).sta.left(2) + "/" + staListVT.at(i).sta.mid(2)),
                                      Q_ARG(QVariant, 0));
        }
    }
}

void MainWindow::changeCircleOnMap(QString mapName, int staIndex, int width, QColor colorName, int opacity)
{
    if(mapName.startsWith("realTimeMap"))
    {
        QMetaObject::invokeMethod(this->realTimeObj, "changeSizeAndColorForStaCircle", Q_RETURN_ARG(QVariant, returnedValue),
                                  Q_ARG(QVariant, staIndex), Q_ARG(QVariant, width),
                                  Q_ARG(QVariant, colorName), Q_ARG(QVariant, opacity));

    }
    else if(mapName.startsWith("eventMap"))
    {
        QMetaObject::invokeMethod(this->stackedObj, "changeSizeAndColorForStaCircle", Q_RETURN_ARG(QVariant, returnedValue),
                                  Q_ARG(QVariant, staIndex), Q_ARG(QVariant, width),
                                  Q_ARG(QVariant, colorName), Q_ARG(QVariant, opacity));
    }
}

void MainWindow::resetStaCircleOnRealTimeMap()
{
    for(int i=0;i<staListVT.count();i++)
    {
        if(staListVT.at(i).inUse == 1)
        {
            changeCircleOnMap("realTimeMap", staListVT.at(i).index, 10, "white", 1);
        }
    }
}

void MainWindow::doRepeatWork()
{
    QDateTime systemTime = QDateTime::currentDateTimeUtc();
    QDateTime systemTimeKST; systemTimeKST = systemTime;
    systemTimeKST = convertKST(systemTimeKST);

    ui->sysTimeLCD->display(systemTimeKST.toString("hh:mm:ss"));

    // read multimap   systemtime - cfg.dataTimeDiff
    QDateTime dataTime = systemTime.addSecs(cfg.timeDiffForMon); // GMT
    QDateTime dataTimeKST; dataTimeKST = dataTime;
    dataTimeKST = convertKST(dataTimeKST);

    mutex.lock();
    QList<_QSCD_FOR_MULTIMAP> pgaList = dataHouse.values(dataTime.toTime_t());
    mutex.unlock();

    int realstanum = 0;

    // to do
    resetStaCircleOnRealTimeMap();

    if(pgaList.size() == 0)
    {
        titleTimer->stop();
        QPixmap titlePX("/.RTICOM2/images/RTICOM2Logo_R.png");
        ui->titleLB->setPixmap(titlePX.scaled(ui->titleLB->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    }
    else if(pgaList.size() != 0)
    {
        if(!titleTimer->isActive())
            titleTimer->start();
        ui->dataTimeLCD->display(dataTimeKST.toString("hh:mm:ss"));

        // display
        int staIndex, regendIndex, maxRegendIndex, repeatIndex;
        QVector<int> indexVector;

        for(int i=0;i<pgaList.size();i++)
        {
            staIndex = staVTForGetIndex.indexOf(pgaList.at(i).sta);

            repeatIndex = indexVector.indexOf(staIndex);

            if(staIndex != -1 && staListVT.at(staIndex).inUse == 1 && repeatIndex == -1)
            {
                realstanum++;
                indexVector.push_back(staIndex);

                _STATION _sta;
                _sta = staListVT.at(staIndex);

                /*
                double revisionValue = 0;
                revisionValue = abs(_sta.lastPGA - pgaList.at(i).pga[cfg.monChanID]);
                */

                QFuture<int> future = QtConcurrent::run(getRegendIndex, pgaList.at(i).pga[cfg.monChanID]);
                //QFuture<int> future = QtConcurrent::run(getRegendIndex, revisionValue);
                future.waitForFinished();
                regendIndex = future.result();

                changeCircleOnMap("realTimeMap", staIndex, CIRCLE_WIDTH, pgaColor[regendIndex], 1);

                _sta.lastPGATime = dataTime.toTime_t();
                _sta.lastPGA = pgaList.at(i).pga[cfg.monChanID];

                if(isEvent && dataTime.toTime_t() >= publicEEWInfo.origin_time + EVENT_SECONDS_FOR_START)
                {
                    if(_sta.maxPGA < pgaList.at(i).pga[cfg.monChanID])
                    {
                        _sta.maxPGA = pgaList.at(i).pga[cfg.monChanID];
                        _sta.maxPGATime = dataTime.toTime_t();

                        QFuture<int> future = QtConcurrent::run(getRegendIndex, _sta.maxPGA);
                        future.waitForFinished();
                        maxRegendIndex = future.result();

                        changeCircleOnMap("eventMap", staIndex, CIRCLE_WIDTH, pgaColor[maxRegendIndex], 1);
                    }
                }

                makeEvent(_sta);
                staListVT.replace(staIndex, _sta);
            }
        }

        if(isEvent && dataTime.toTime_t() >= publicEEWInfo.origin_time + EVENT_SECONDS_FOR_START)
        {
            // circle animation
            if(publicEEWInfo.eew_evid != 0)
            {
                int diffTime = dataTime.toTime_t() - publicEEWInfo.origin_time;
                if(diffTime > 0)
                {
                    double radiusP = diffTime * P_VEL * 1000;
                    double radiusS = diffTime * S_VEL * 1000;

                    QMetaObject::invokeMethod(this->stackedObj, "showCircleForAnimation",
                                              Q_RETURN_ARG(QVariant, returnedValue),
                                              Q_ARG(QVariant, publicEEWInfo.latitude), Q_ARG(QVariant, publicEEWInfo.longitude),
                                              Q_ARG(QVariant, radiusP), Q_ARG(QVariant, radiusS));
                }
            }
            findMaxStations();
        }
        else if(!isEvent) checkEvent(dataTime);
    }

    if(isEvent)
    {
        int timeDiff = dataTime.toTime_t() - eventTime;

        // end event
        if(timeDiff >= EVENT_DURATION - EVENT_SECONDS_FOR_START)
        {
            log->write(cfg.logDir, "==================== END OF THIS EVENT =====================");
            resetEvent();
        }
    }

    soh->updateInfo(staListVT, dataTime.toTime_t());

    ui->staNumLB->setText(QString::number(realstanum));
    log->write(cfg.logDir, "TIME=" + QString::number(dataTime.toTime_t()) + "  NUM=" + QString::number(realstanum) + " of " + QString::number(pgaList.size()));

    // remove mmap
    if(!dataHouse.isEmpty())
    {
        mutex.lock();
        QMultiMap<int, _QSCD_FOR_MULTIMAP>::iterator iter;
        QMultiMap<int, _QSCD_FOR_MULTIMAP>::iterator untilIter;
        untilIter = dataHouse.lowerBound(dataTime.toTime_t() + (cfg.timeDiffForMon - 5));

        for(iter = dataHouse.begin() ; untilIter != iter;)
        {
            QMultiMap<int, _QSCD_FOR_MULTIMAP>::iterator thisIter;
            thisIter = iter;
            iter++;
            dataHouse.erase(thisIter);
        }
        mutex.unlock();
    }
}

void MainWindow::findMaxStations()
{
    ui->eventMaxPGATW->setRowCount(0);
    ui->eventMaxPGATW->setSortingEnabled(false);

    for(int i=0;i<staListVT.count();i++)
    {
        if(staListVT.at(i).maxPGA == 0 || staListVT.at(i).maxPGATime <= eventTime)
        {
                continue;
        }

        QDateTime t;
        t.setTime_t(staListVT.at(i).maxPGATime);
        t = convertKST(t);

        ui->eventMaxPGATW->setRowCount(ui->eventMaxPGATW->rowCount()+1);
        ui->eventMaxPGATW->setItem(ui->eventMaxPGATW->rowCount()-1, 0, new QTableWidgetItem(staListVT.at(i).sta.left(2)));
        ui->eventMaxPGATW->setItem(ui->eventMaxPGATW->rowCount()-1, 1, new QTableWidgetItem(staListVT.at(i).sta.mid(2)));
        ui->eventMaxPGATW->setItem(ui->eventMaxPGATW->rowCount()-1, 2, new QTableWidgetItem(staListVT.at(i).comment));
        ui->eventMaxPGATW->setItem(ui->eventMaxPGATW->rowCount()-1, 3, new QTableWidgetItem(t.toString("hh:mm:ss")));

        QTableWidgetItem *pgaItem = new QTableWidgetItem;
        pgaItem->setData(Qt::EditRole, staListVT.at(i).maxPGA);
        ui->eventMaxPGATW->setItem(ui->eventMaxPGATW->rowCount()-1, 4, pgaItem);

        ui->eventMaxPGATW->item(ui->eventMaxPGATW->rowCount()-1, 0)->setTextAlignment(Qt::AlignCenter);
        ui->eventMaxPGATW->item(ui->eventMaxPGATW->rowCount()-1, 1)->setTextAlignment(Qt::AlignCenter);
        ui->eventMaxPGATW->item(ui->eventMaxPGATW->rowCount()-1, 2)->setTextAlignment(Qt::AlignCenter);
        ui->eventMaxPGATW->item(ui->eventMaxPGATW->rowCount()-1, 3)->setTextAlignment(Qt::AlignCenter);
        ui->eventMaxPGATW->item(ui->eventMaxPGATW->rowCount()-1, 4)->setTextAlignment(Qt::AlignCenter);
    }

    ui->eventMaxPGATW->setSortingEnabled(true);
    ui->eventMaxPGATW->sortByColumn(4, Qt::DescendingOrder);
}

void MainWindow::resetEventGUI()
{
    QMetaObject::invokeMethod(this->stackedObj, "clearMap", Q_RETURN_ARG(QVariant, returnedValue));

    for(int i=0;i<staListVT.count();i++)
    {
        if(staListVT.at(i).inUse == 1)
        {
            changeCircleOnMap("eventMap", staListVT.at(i).index, 10, "white", 0);

            _STATION sta;
            sta = staListVT.at(i);
            sta.maxPGA = 0;
            sta.maxPGATime = 0;

            staListVT.replace(i, sta);
        }
    }

    ui->eventTimeLB->clear();

    for(int i=0;i<MAX_NUM_EVENTINITSTA;i++)
    {
        eventNetLB[i]->clear();
        eventStaLB[i]->clear();
        eventComLB[i]->clear();
        eventTimeLB[i]->clear();
        eventPGALB[i]->clear();
    }

    ui->eventMaxPGATW->setRowCount(0);

    ui->stackedLB->clear();
    ui->eqTimeLB->clear();
    ui->eqLocLB->clear();
    ui->eqLoc2LB->clear();
    ui->eqMagLB->clear();

    ui->eventInitFR->show();
}

void MainWindow::resetEvent()
{
    isEvent = false;
    eventTime = 0;
    eventV.clear();

    publicEEWInfo.eew_evid = 0;

    for(int i=0;i<staListVT.count();i++)
    {
        if(staListVT.at(i).inUse == 1)
        {
            _STATION sta;
            sta = staListVT.at(i);
            sta.maxPGA = 0;
            sta.maxPGATime = 0;

            staListVT.replace(i, sta);
        }
    }
}

void MainWindow::_qmlSignalfromRealTimeMap(QString indexS)
{
    QString msg, msg2;
    int index = indexS.toInt();
    msg = staListVT.at(index).sta.left(2) + "/" + staListVT.at(index).sta.mid(2) + "/" +
          staListVT.at(index).comment;

    if(staListVT.at(index).lastPGATime != 0)
    {
        QDateTime dt;
        dt.setTime_t(staListVT.at(index).lastPGATime);
        dt = convertKST(dt);

        msg2 = "     Last PGA Time : " + dt.toString("yyyy/MM/dd hh:mm:ss") + "     Last PGA : " +
                QString::number(staListVT.at(index).lastPGA, 'f', 6) + " gal";

        msg = msg + msg2;
    }
    ui->realTimeLB->setText( msg );
}

void MainWindow::_qmlSignalfromStackedMap(QString indexS)
{
    QString msg, msg2;
    int index = indexS.toInt();
    msg = staListVT.at(index).sta.left(2) + "/" + staListVT.at(index).sta.mid(2) + "/" +
          staListVT.at(index).comment;

    if(staListVT.at(index).maxPGATime != 0)
    {
        QDateTime dt;
        dt.setTime_t(staListVT.at(index).maxPGATime);
        dt = convertKST(dt);

        msg2 = "     Maximum PGA Time:" + dt.toString("yyyy/MM/dd hh:mm:ss") + "     Maximum PGA:" +
                QString::number(staListVT.at(index).maxPGA, 'f', 6) + " gal";

        msg = msg + msg2;
    }

    ui->stackedLB->setText( msg );
}

void MainWindow::makeEvent(_STATION tempSta)
{
    // make a EVENT VECTOR (eventV)
    if(tempSta.lastPGA >= cfg.thresholdG && !isEvent)
    {
        if(eventV.isEmpty())
        {
            _EVENT ev;
            ev.numOverDist = 0;
            ev.triggerTime = tempSta.lastPGATime;
            ev.originTime = 0;
            ev.staIndexV.push_back(tempSta.index);
            ev.timeV.push_back(tempSta.lastPGATime);
            ev.pgaV.push_back(tempSta.lastPGA);
            eventV.push_back(ev);
        }
        else
        {
            for(int k=0;k<eventV.count();k++)
            {
                int timeDiff = tempSta.lastPGATime - eventV.at(k).triggerTime;

                if(timeDiff <= cfg.inSeconds)
                {
                    _EVENT ev = eventV.at(k);

                    int isExist = ev.staIndexV.indexOf(tempSta.index);

                    if(isExist == -1)
                    {
                        ev.staIndexV.push_back(tempSta.index);
                        ev.timeV.push_back(tempSta.lastPGATime);
                        ev.pgaV.push_back(tempSta.lastPGA);

                        // get number over distance
                        for(int xx = 0; xx < ev.staIndexV.count() - 1 ; xx++)
                        {
                            for(int yy = xx + 1 ; yy < ev.staIndexV.count() ; yy++)
                            {
                                double dist = getDistance(staListVT.at(ev.staIndexV.at(xx)).lat,
                                                          staListVT.at(ev.staIndexV.at(xx)).lon,
                                                          staListVT.at(ev.staIndexV.at(yy)).lat,
                                                          staListVT.at(ev.staIndexV.at(yy)).lon);

                                if(dist >= cfg.distance)
                                    ev.numOverDist++;
                            }
                        }

                        eventV.replace(k, ev);
                    }
                }
            }

            _EVENT ev;
            ev.numOverDist = 0;
            ev.triggerTime = tempSta.lastPGATime;
            ev.originTime = 0;
            ev.staIndexV.push_back(tempSta.index);
            ev.timeV.push_back(tempSta.lastPGATime);
            ev.pgaV.push_back(tempSta.lastPGA);
            eventV.push_back(ev);
        }
    }
}

void MainWindow::checkEvent(QDateTime dataTime)
{
    // check event
    if(!eventV.isEmpty() && !isEvent)
    {
        for(int k=0;k<eventV.count();k++)
        {
            int timeDiff = dataTime.toTime_t() - eventV.at(k).triggerTime;
            if(timeDiff <= cfg.inSeconds && eventV.at(k).staIndexV.count() >= cfg.numSta &&
                    //(eventV.at(k).staIndexV.count() - eventV.at(k).numOverDist) >= eventV.at(k).staIndexV.count())
                    (eventV.at(k).staIndexV.count() - eventV.at(k).numOverDist) >= cfg.numSta)
            {
                _EVENT ev = eventV.at(k);
                ev.originTime = dataTime.toTime_t(); // epoch (GMT)
                eventV.replace(k, ev);
                isEvent = true;
                resetEventGUI();
                eventTime = ev.originTime;

                int maxRegendIndex;
                for(int q=0;q<staListVT.count();q++)
                {
                    // init sta.maxPGA
                    _STATION sta = staListVT.at(q);

                    if(sta.lastPGATime >= eventTime)
                    {
                        sta.maxPGATime = sta.lastPGATime;
                        sta.maxPGA = sta.lastPGA;

                        staListVT.replace(q, sta);

                        QFuture<int> future = QtConcurrent::run(getRegendIndex, sta.maxPGA);
                        future.waitForFinished();
                        maxRegendIndex = future.result();

                        if(staListVT.at(q).inUse == 1 && staListVT.at(q).maxPGA != 0)
                        {
                            changeCircleOnMap("eventMap", staListVT.at(q).index, CIRCLE_WIDTH, pgaColor[maxRegendIndex], 1);
                        }
                    }
                }

                // to do for event
                QDateTime et, etKST; et.setTime_t(ev.originTime);
                etKST = convertKST(et);

                ui->eventTimeLB->setText(etKST.toString("yyyy/MM/dd hh:mm:ss"));

                // logging
                log->write(cfg.logDir, "==================== DETECTED NEW EVENT =====================");
                log->write(cfg.logDir, "EVENT TIME:" + et.toString("yyyy/MM/dd hh:mm:ss"));
                log->write(cfg.logDir, "EVENT_CONDITION=" + QString::number(cfg.inSeconds) + ":" + QString::number(cfg.numSta) + ":"
                           + QString::number(cfg.thresholdG, 'f', 2) + ":" + ui->monChanLB->text());

                for(int j=0;j<ev.staIndexV.count();j++)
                {
                    QDateTime t, tKST; t.setTime_t(ev.timeV.at(j));
                    tKST = convertKST(t);

                    eventNetLB[j]->setText(staListVT.at(ev.staIndexV.at(j)).sta.left(2));
                    eventStaLB[j]->setText(staListVT.at(ev.staIndexV.at(j)).sta);
                    eventComLB[j]->setText(staListVT.at(ev.staIndexV.at(j)).comment);

                    eventTimeLB[j]->setText(tKST.toString("hh:mm:ss"));
                    eventPGALB[j]->setText(QString::number(ev.pgaV.at(j), 'f', 4));

                    log->write(cfg.logDir, "FIRST_EVENT_INFO=" + QString::number(ev.timeV.at(j)) + ":" + staListVT.at(ev.staIndexV.at(j)).sta.left(2) + ":" +
                               staListVT.at(ev.staIndexV.at(j)).sta
                               + ":" + staListVT.at(ev.staIndexV.at(j)).comment + ":" + QString::number(ev.pgaV.at(j), 'f', 6));
                }

                ui->eventInitFR->show();

                log->write(cfg.logDir, "=============================================================");

                eventV.clear();
                break;
            }
            else
            {
                eventV.remove(k);
            }
        }
    }
}

void MainWindow::rvEEWInfo(_EEWInfo eewInfo)
{
    if(eewInfo.magnitude < cfg.thresholdM)
        return;

    if(!isEvent)
    {
        isEvent = true;
        resetEventGUI();

        ui->eventInitFR->hide();

        eventTime = eewInfo.origin_time;
        publicEEWInfo = eewInfo;

        int maxRegendIndex;
        for(int q=0;q<staListVT.count();q++)
        {
            // init sta.maxPGA
            _STATION sta = staListVT.at(q);

            if(sta.lastPGATime >= eventTime)
            {
                sta.maxPGATime = sta.lastPGATime;
                sta.maxPGA = sta.lastPGA;
                staListVT.replace(q, sta);

                QFuture<int> future = QtConcurrent::run(getRegendIndex, sta.maxPGA);
                future.waitForFinished();
                maxRegendIndex = future.result();

                if(staListVT.at(q).inUse == 1 && staListVT.at(q).maxPGA != 0)
                {
                    changeCircleOnMap("eventMap", staListVT.at(q).index, CIRCLE_WIDTH, pgaColor[maxRegendIndex], 1);
                }
            }
        }

        // to do for event
        QDateTime et, etKST; et.setTime_t(publicEEWInfo.origin_time);
        etKST = convertKST(et);

        ui->eventTimeLB->setText(etKST.toString("yyyy/MM/dd hh:mm:ss"));
        ui->eqTimeLB->setText(etKST.toString("yyyy/MM/dd hh:mm:ss"));

        // logging
        log->write(cfg.logDir, "==================== DETECTED NEW EVENT BY EEW =====================");
        log->write(cfg.logDir, "EVENT TIME:" + et.toString("yyyy/MM/dd hh:mm:ss"));
        log->write(cfg.logDir, "EEW_INFO:" + QString::number(publicEEWInfo.eew_evid) + "/" + QString::number(publicEEWInfo.latitude, 'f', 4) + "/" + QString::number(publicEEWInfo.longitude, 'f', 4)
                   + "/" + QString::number(publicEEWInfo.magnitude, 'f', 2));
        log->write(cfg.logDir, "=============================================================");

        QProcess process;
        QString cmd = cfg.homeDir + "/bin/" + find_loc_program + " " + QString::number(publicEEWInfo.latitude, 'f', 4) + " " + QString::number(publicEEWInfo.longitude, 'f', 4);
        process.start(cmd);
        process.waitForFinished(-1); // will wait forever until finished

        QString stdout = process.readAllStandardOutput();
        int leng = stdout.length();

        ui->eqLocLB->setText(stdout.left(leng-1));
        ui->eqLoc2LB->setText("LAT:"+QString::number(publicEEWInfo.latitude, 'f', 4) + "    LON:"+QString::number(publicEEWInfo.longitude, 'f', 4));
        ui->eqMagLB->setText(QString::number(publicEEWInfo.magnitude, 'f', 1));

        //show eqStaMarker on a map
        QString tempMag = QString::number(publicEEWInfo.magnitude, 'f', 1);
        QMetaObject::invokeMethod(this->stackedObj, "showEEWStarMarker",
                                  Q_RETURN_ARG(QVariant, returnedValue),
                                  Q_ARG(QVariant, publicEEWInfo.latitude), Q_ARG(QVariant, publicEEWInfo.longitude),
                                  Q_ARG(QVariant, tempMag));

        eventV.clear();
    }
    else
    {
        if(eewInfo.message_type == UPDATE)
        {
            eventTime = eewInfo.origin_time;
            publicEEWInfo = eewInfo;

            QDateTime et, etKST; et.setTime_t(publicEEWInfo.origin_time);
            etKST = convertKST(et);

            ui->eventTimeLB->setText(etKST.toString("yyyy/MM/dd hh:mm:ss"));
            ui->eqTimeLB->setText(etKST.toString("yyyy/MM/dd hh:mm:ss"));

            // logging
            log->write(cfg.logDir, "==================== DETECTED UPDATE EVENT BY EEW =====================");
            log->write(cfg.logDir, "EVENT TIME:" + et.toString("yyyy/MM/dd hh:mm:ss"));
            log->write(cfg.logDir, "EEW_INFO:" + QString::number(publicEEWInfo.eew_evid) + "/" + QString::number(publicEEWInfo.latitude, 'f', 4) + "/" + QString::number(publicEEWInfo.longitude, 'f', 4)
                       + "/" + QString::number(publicEEWInfo.magnitude, 'f', 2));
            log->write(cfg.logDir, "=============================================================");

            QProcess process;
            QString cmd = cfg.homeDir + "/bin/" + find_loc_program + " " + QString::number(publicEEWInfo.latitude, 'f', 4) + " " + QString::number(publicEEWInfo.longitude, 'f', 4);
            process.start(cmd);
            process.waitForFinished(-1); // will wait forever until finished

            QString stdout = process.readAllStandardOutput();
            int leng = stdout.length();

            ui->eqLocLB->setText(stdout.left(leng-1));
            ui->eqLoc2LB->setText("LAT:"+QString::number(publicEEWInfo.latitude, 'f', 4) + "    LON:"+QString::number(publicEEWInfo.longitude, 'f', 4));
            ui->eqMagLB->setText(QString::number(publicEEWInfo.magnitude, 'f', 1));

            //show eqStaMarker on a map
            QString tempMag = QString::number(publicEEWInfo.magnitude, 'f', 1);
            QMetaObject::invokeMethod(this->stackedObj, "showEEWStarMarker",
                                      Q_RETURN_ARG(QVariant, returnedValue),
                                      Q_ARG(QVariant, publicEEWInfo.latitude), Q_ARG(QVariant, publicEEWInfo.longitude),
                                      Q_ARG(QVariant, tempMag));
        }
    }
}
