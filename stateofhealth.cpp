#include "stateofhealth.h"
#include "ui_stateofhealth.h"

StateOfHealth::StateOfHealth(QVector<_STATION> initStaVT, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::StateOfHealth)
{
    ui->setupUi(this);

    codec = QTextCodec::codecForName("utf-8");
    QFont lbFont("Cantarell", 12, QFont::Normal);

    ui->staInfoTW->setColumnWidth(0, 80);
    ui->staInfoTW->setColumnWidth(1, 100);
    ui->staInfoTW->setColumnWidth(2, 100);
    ui->staInfoTW->setColumnWidth(3, 300);
    ui->staInfoTW->setColumnWidth(4, 120);
    ui->staInfoTW->setColumnWidth(5, 120);
    ui->staInfoTW->setColumnWidth(6, 120);
    ui->staInfoTW->setColumnWidth(7, 230);
    ui->staInfoTW->setColumnWidth(8, 150);

    for(int i=0;i<initStaVT.count();i++)
            networkList << initStaVT.at(i).sta.left(2);

    int numRemoved = networkList.removeDuplicates();
    numNetwork = initStaVT.count() - numRemoved;
    ui->numNetLB->setText(QString::number(numNetwork));
    ui->numStaLB->setText(QString::number(initStaVT.count()));

    myIndex = 0;

    QSpacerItem *spacer = new QSpacerItem(10,20, QSizePolicy::Fixed, QSizePolicy::Fixed);
    ui->sAreaLO->addSpacerItem(spacer);

    for(int i=0;i<numNetwork;i++)
    {
        netLB[i] = new QLabel();
        netLB[i]->setFont(lbFont);
        netLB[i]->setFixedHeight(30);
        ui->sAreaLO->addWidget(netLB[i]);
        QGridLayout *gLO = new QGridLayout();
        gLO->setSpacing(0);
        ui->sAreaLO->addLayout(gLO);

        int numInNetwork = 0;

        for(int j=0;j<initStaVT.count();j++)
        {
            if(networkList.at(i).startsWith(initStaVT.at(j).sta.left(2)))
            {
                staPB[j] = new QPushButton();
                staPB[j]->setObjectName(QString::number(j));
                connect(staPB[j], SIGNAL(clicked(bool)), this, SLOT(staPBClicked(bool)));
                staPB[j]->setText(initStaVT.at(j).sta);
                staPB[j]->setFixedSize(144, 25);
                staPB[j]->setStyleSheet("background-color: rgb(159, 255, 111);");
                int locationRow, locationCol;
                if(numInNetwork<10) locationRow = 0;
                else locationRow = numInNetwork / 10;
                locationCol = numInNetwork % 10;
                gLO->addWidget(staPB[j], locationRow, locationCol);
                numInNetwork++;
            }
        }

        if(numInNetwork<10)
        {
            QLabel *dummy[10-numInNetwork];
            for(int k=0;k<10-numInNetwork;k++)
            {
                dummy[k] = new QLabel();
                gLO->addWidget(dummy[k], 0, 10-k);
            }
        }

        QSpacerItem *spacer = new QSpacerItem(10,20, QSizePolicy::Fixed, QSizePolicy::Fixed);
        ui->sAreaLO->addSpacerItem(spacer);

        _NETWORK net;
        net.netName = networkList.at(i);
        net.numSta = numInNetwork;
        net.numInUse = 0;
        net.numNoUse = 0;
        net.numNormal = 0;
        net.numBad = 0;
        netVT.push_back(net);
    }
}

StateOfHealth::~StateOfHealth()
{
    delete ui;
}

void StateOfHealth::resetInUseInfo(QVector<_STATION> _staVT)
{
    staVT = _staVT;

    int numInUse = 0, numNoUse = 0;
    for(int i=0;i<staVT.count();i++)
    {
            if(staVT.at(i).inUse == 1)
                numInUse++;
            else
                numNoUse++;
    }

    ui->numInUse->setText(QString::number(numInUse));
    ui->numNoUse->setText(QString::number(numNoUse));

    for(int i=0;i<netVT.count();i++)
    {
        _NETWORK net;
        net = netVT.at(i);
        net.numInUse = 0;
        net.numNoUse = 0;
        mutex.lock();
        netVT.replace(i, net);
        mutex.unlock();
    }

    for(int i=0;i<staVT.count();i++)
    {
        int inUse = 1;
        if(staVT.at(i).inUse == 1)
        {
            staPB[i]->setStyleSheet("background-color: rgb(159, 255, 111); color: rgb(0, 0, 0);");
        }
        else
        {
            staPB[i]->setStyleSheet("background-color: rgb(210, 210, 210); color: rgb(0, 0, 0);");
            inUse = 0;
        }

        for(int j=0;j<netVT.count();j++)
        {
            if(staVT.at(i).sta.left(2).startsWith(netVT.at(j).netName))
            {
                _NETWORK net;
                net = netVT.at(j);
                if(inUse)
                    net.numInUse++;
                else
                    net.numNoUse++;
                mutex.lock();
                netVT.replace(j, net);
                mutex.unlock();
                break;
            }
        }
    }
}

void StateOfHealth::updateInfo(QVector<_STATION> _staVT, int dataTime)
{
    staVT = _staVT;
    datatime = dataTime;
    QDateTime t; t.setTime_t(datatime);
    t = convertKST(t);
    ui->dataTimeLCD->display(t.toString("hh:mm:ss"));
    doRepeatWork();
}

void StateOfHealth::doRepeatWork()
{
    int numTotNormal = 0;
    int numTotBad = 0;

    for(int i=0;i<netVT.count();i++)
    {
        _NETWORK net;
        net = netVT.at(i);
        net.numNormal = 0;
        net.numBad = 0;
        mutex.lock();
        netVT.replace(i, net);
        mutex.unlock();
    }

    for(int i=0;i<staVT.count();i++)
    {
        if(staVT.at(i).inUse == 1)
        {
            int normal = 1;
            if(staVT.at(i).lastPGATime >= datatime)
            {
                staPB[i]->setStyleSheet("background-color: rgb(159, 255, 111); color: rgb(0, 0, 0);");
                numTotNormal++;
            }
            else
            {
                staPB[i]->setStyleSheet("background-color: rgb(239, 41, 41); color: rgb(0, 0, 0);");
                normal = 0;
                numTotBad++;
            }

            for(int j=0;j<netVT.count();j++)
            {
                if(staVT.at(i).sta.left(2).startsWith(netVT.at(j).netName))
                {
                    _NETWORK net;
                    net = netVT.at(j);
                    if(normal)
                        net.numNormal++;
                    else
                        net.numBad++;
                    mutex.lock();
                    netVT.replace(j, net);
                    mutex.unlock();
                    break;
                }
            }
        }
    }

    for(int i=0;i<netVT.count();i++)
    {
        netLB[i]->setText("- 네트워크 : " + networkList.at(i) + codec->toUnicode(" (총 ") +
                          QString::number(netVT.at(i).numSta) + codec->toUnicode("개 관측소 - ") +
                          codec->toUnicode("사용중:") + QString::number(netVT.at(i).numInUse) +
                          codec->toUnicode("  사용안함:") + QString::number(netVT.at(i).numNoUse) +
                          codec->toUnicode("  수신중:") + QString::number(netVT.at(i).numNormal) +
                          codec->toUnicode("  수신불가:") + QString::number(netVT.at(i).numBad) +")");
    }

    ui->numNormal->setText(QString::number(numTotNormal));
    ui->numBad->setText(QString::number(numTotBad));

    updateTable(myIndex);
}

void StateOfHealth::staPBClicked(bool)
{
    QObject* pObject = sender();
    myIndex = pObject->objectName().toInt();
}

void StateOfHealth::updateTable(int index)
{
    QDateTime lastPGATimeKST, maxPGATimeKST;
    ui->staInfoTW->setRowCount(1);
    ui->staInfoTW->setItem(0, 0, new QTableWidgetItem(QString::number(staVT.at(index).index)));

    if(staVT.at(index).lastPGATime != 0)
    {
        lastPGATimeKST.setTime_t(staVT.at(index).lastPGATime);
        lastPGATimeKST = convertKST(lastPGATimeKST);
        ui->staInfoTW->setItem(0, 7, new QTableWidgetItem(lastPGATimeKST.toString("yyyy/MM/dd hh:mm:ss")));
        ui->staInfoTW->setItem(0, 8, new QTableWidgetItem(QString::number(staVT.at(index).lastPGA, 'f', 4)));
    }
    else
    {
        ui->staInfoTW->setItem(0, 7, new QTableWidgetItem("-"));
        ui->staInfoTW->setItem(0, 8, new QTableWidgetItem("-"));
    }

    if(staVT.at(index).inUse == 1)
        ui->staInfoTW->setItem(0, 9, new QTableWidgetItem("YES"));
    else
        ui->staInfoTW->setItem(0, 9, new QTableWidgetItem("NO"));

    ui->staInfoTW->setItem(0, 1, new QTableWidgetItem(staVT.at(index).sta.left(2)));
    ui->staInfoTW->setItem(0, 2, new QTableWidgetItem(staVT.at(index).sta));
    ui->staInfoTW->setItem(0, 3, new QTableWidgetItem(staVT.at(index).comment));
    ui->staInfoTW->setItem(0, 4, new QTableWidgetItem(QString::number(staVT.at(index).lat, 'f', 4)));
    ui->staInfoTW->setItem(0, 5, new QTableWidgetItem(QString::number(staVT.at(index).lon, 'f', 4)));
    ui->staInfoTW->setItem(0, 6, new QTableWidgetItem(QString::number(staVT.at(index).elev, 'f', 4)));


    for(int j=0;j<ui->staInfoTW->rowCount();j++)
    {
        for(int k=0;k<ui->staInfoTW->columnCount();k++)
        {
            ui->staInfoTW->item(j, k)->setTextAlignment(Qt::AlignCenter);
        }
    }
}
