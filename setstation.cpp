#include "setstation.h"
#include "ui_setstation.h"

SetStation::SetStation(QVector<_STATION> initStaVT, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SetStation)
{
    ui->setupUi(this);

    codec = QTextCodec::codecForName("utf-8");
    QFont lbFont("Cantarell", 12, QFont::Normal);

    for(int i=0;i<initStaVT.count();i++)
            networkList << initStaVT.at(i).sta.left(2);

    int numRemoved = networkList.removeDuplicates();
    numNetwork = initStaVT.count() - numRemoved;

    ui->tNumStaLB->setText(QString::number(initStaVT.count()));

    ui->netTitleLB->setText(codec->toUnicode("네트워크별 관측소 리스트 (총 ") +
                            QString::number(numNetwork) +
                            codec->toUnicode("개 네트워크)"));

    for(int i=0;i<numNetwork;i++)
    {
        QHBoxLayout *titleLO = new QHBoxLayout();
        ui->sAreaLO->addLayout(titleLO);
        netLB[i] = new QLabel();
        titleLO->addWidget(netLB[i]);

        netLB[i]->setFont(lbFont);
        //netLB[i]->setFixedSize(530, 30);
        netLB[i]->setFixedHeight(30);

        netTW[i] = new QTableWidget();
        netTW[i]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        ui->sAreaLO->addWidget(netTW[i]);

        netTW[i]->setRowCount(0);
        netTW[i]->setColumnCount(7);
        netTW[i]->setAlternatingRowColors(true);
        netTW[i]->setEditTriggers(QAbstractItemView::NoEditTriggers);
        netTW[i]->setSelectionBehavior(QAbstractItemView::SelectRows);
        netTW[i]->setHorizontalHeaderItem(0, new QTableWidgetItem("Index"));
        netTW[i]->setHorizontalHeaderItem(1, new QTableWidgetItem("Station"));
        netTW[i]->setHorizontalHeaderItem(2, new QTableWidgetItem("Comment"));
        netTW[i]->setHorizontalHeaderItem(3, new QTableWidgetItem("Lat."));
        netTW[i]->setHorizontalHeaderItem(4, new QTableWidgetItem("Lon."));
        netTW[i]->setHorizontalHeaderItem(5, new QTableWidgetItem("Elev."));
        netTW[i]->setHorizontalHeaderItem(6, new QTableWidgetItem("In Use"));
        netTW[i]->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        netTW[i]->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        for(int j=0;j<initStaVT.count();j++)
        {
            if(networkList.at(i).startsWith(initStaVT.at(j).sta.left(2)))
            {
                netTW[i]->setRowCount(netTW[i]->rowCount()+1);
                netTW[i]->setItem(netTW[i]->rowCount()-1, 0, new QTableWidgetItem(QString::number(initStaVT.at(j).index)));
                netTW[i]->setItem(netTW[i]->rowCount()-1, 1, new QTableWidgetItem(initStaVT.at(j).sta));
                netTW[i]->setItem(netTW[i]->rowCount()-1, 2, new QTableWidgetItem(initStaVT.at(j).comment));
                netTW[i]->setItem(netTW[i]->rowCount()-1, 3, new QTableWidgetItem(QString::number(initStaVT.at(j).lat, 'f', 4)));
                netTW[i]->setItem(netTW[i]->rowCount()-1, 4, new QTableWidgetItem(QString::number(initStaVT.at(j).lon, 'f', 4)));
                netTW[i]->setItem(netTW[i]->rowCount()-1, 5, new QTableWidgetItem(QString::number(initStaVT.at(j).elev, 'f', 4)));
                netTW[i]->setItem(netTW[i]->rowCount()-1, 6, new QTableWidgetItem("-"));
            }
        }

        netTW[i]->horizontalHeader()->setStretchLastSection(true);
        netTW[i]->verticalHeader()->setStretchLastSection(true);
        netTW[i]->verticalHeader()->setDefaultSectionSize(25);

        netTW[i]->setFixedHeight(netTW[i]->rowCount() * 25 + 25);

        //table width : 640px
        netTW[i]->setColumnWidth(0, 40);
        netTW[i]->setColumnWidth(1, 60);
        netTW[i]->setColumnWidth(2, 240);
        netTW[i]->setColumnWidth(3, 80);
        netTW[i]->setColumnWidth(4, 80);
        netTW[i]->setColumnWidth(5, 80);
        netTW[i]->setColumnWidth(6, 70);

        netLB[i]->setText("- 네트워크 : " + networkList.at(i) + codec->toUnicode(" (총 ") +
                          QString::number(netTW[i]->rowCount()) + codec->toUnicode("개 관측소)"));

        for(int j=0;j<netTW[i]->rowCount();j++)
        {
            for(int k=0;k<netTW[i]->columnCount();k++)
            {
                netTW[i]->item(j, k)->setTextAlignment(Qt::AlignCenter);
            }
        }

        QSpacerItem *spacer = new QSpacerItem(10,20, QSizePolicy::Fixed, QSizePolicy::Fixed);
        ui->sAreaLO->addSpacerItem(spacer);

        netBoldStatus << "0";
        netBoldPB[i] = new QPushButton();
        netBoldPB[i]->setObjectName(networkList.at(i) + "_" + QString::number(i));
        connect(netBoldPB[i], SIGNAL(clicked(bool)), this, SLOT(netBoldPBClicked(bool)));
        netBoldPB[i]->setStyleSheet("background-color: rgb(255,255,255)");
        netBoldPB[i]->setText(networkList.at(i));
        netBoldPB[i]->setFixedHeight(25);
        int locationRow, locationCol;
        if(i<10) locationRow = 0;
        else locationRow = i / 10;
        locationCol = i % 10;
        ui->netPBLO->addWidget(netBoldPB[i], locationRow, locationCol);
    }

    QQuickView *mapView = new QQuickView();
    mapContainer = QWidget::createWindowContainer(mapView, this);
    mapView->setResizeMode(QQuickView::SizeRootObjectToView);
    mapContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mapContainer->setFocusPolicy(Qt::TabFocus);
    mapView->setSource(QUrl(QStringLiteral("qrc:/SetSta.qml")));
    ui->mapLO->addWidget(mapContainer);
    obj = mapView->rootObject();

    QMetaObject::invokeMethod(this->obj, "clearMap", Q_RETURN_ARG(QVariant, returnedValue));

    // create staMarker
    for(int i=0;i<initStaVT.count();i++)
    {
        QMetaObject::invokeMethod(this->obj, "createStaMarker", Q_RETURN_ARG(QVariant, returnedValue),
                                  Q_ARG(QVariant, initStaVT.at(i).index), Q_ARG(QVariant, initStaVT.at(i).sta.left(2)), Q_ARG(QVariant, initStaVT.at(i).sta.mid(2)),
                                  Q_ARG(QVariant, initStaVT.at(i).lat), Q_ARG(QVariant, initStaVT.at(i).lon), Q_ARG(QVariant, initStaVT.at(i).inUse));
    }
}

SetStation::~SetStation()
{
    delete ui;
}

void SetStation::setup(QVector<_STATION> _staVT)
{
    staVT = _staVT;

    for(int i=0;i<numNetwork;i++)
    {
        setCheckBoxInTable(i);
    }
}

void SetStation::netBoldPBClicked(bool)
{
    QObject* pObject = sender();
    int netIndex = pObject->objectName().section("_", 1, 1).toInt();
    QString netName = pObject->objectName().section("_", 0, 0);

    if(netBoldStatus.at(netIndex).startsWith("1"))
    {
        netBoldStatus.replace(netIndex, "0");
        netBoldPB[netIndex]->setStyleSheet("background-color: rgb(255,255,255)");

        for(int i=0;i<staVT.count();i++)
        {
            if(netName.startsWith(staVT.at(i).sta.left(2)))
            {
                QMetaObject::invokeMethod(this->obj, "normalStaMarker", Q_RETURN_ARG(QVariant, returnedValue),
                                          Q_ARG(QVariant, staVT.at(i).index), Q_ARG(QVariant, staVT.at(i).inUse));
            }
        }
    }
    else
    {
        netBoldStatus.replace(netIndex, "1");
        netBoldPB[netIndex]->setStyleSheet("background-color: rgb(236, 239, 14)");

        for(int i=0;i<staVT.count();i++)
        {
            if(netName.startsWith(staVT.at(i).sta.left(2)))
            {
                QMetaObject::invokeMethod(this->obj, "boldStaMarker", Q_RETURN_ARG(QVariant, returnedValue), Q_ARG(QVariant, staVT.at(i).index));
            }
        }
    }

}

void SetStation::setCheckBoxInTable(int i)
{
    int indexInNetwork = 0;
    for(int j=0;j<staVT.count();j++)
    {
        if(networkList.at(i).startsWith(staVT.at(j).sta.left(2)))
        {
            QWidget *checkBoxWidget = new QWidget();
            QCheckBox *staCB = new QCheckBox();
            staCB->setEnabled(false);
            QHBoxLayout *layoutCheckBox = new QHBoxLayout(checkBoxWidget);
            layoutCheckBox->addWidget(staCB);
            layoutCheckBox->setAlignment(Qt::AlignCenter);
            layoutCheckBox->setContentsMargins(0,0,0,0);
            checkBoxWidget->setLayout(layoutCheckBox);

            if(staVT.at(j).inUse == 1)
            {
                staCB->setChecked(true);
            }
            else
            {
                staCB->setChecked(false);
            }

            netTW[i]->setCellWidget(indexInNetwork, 6, checkBoxWidget);
            indexInNetwork++;
        }
    }
    getNumInUseStation();
}

void SetStation::getNumInUseStation()
{
    int numInUseSta = 0;
    for(int i=0;i<staVT.count();i++)
    {
        if(staVT.at(i).inUse == 1)
            numInUseSta++;
    }
    ui->cNumStaLB->setText(QString::number(numInUseSta));
}
