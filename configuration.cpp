#include "configuration.h"
#include "ui_configuration.h"

Configuration::Configuration(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Configuration)
{
    ui->setupUi(this);

    connect(ui->savePB, SIGNAL(clicked(bool)), this, SLOT(saveConfigure()));
}

Configuration::~Configuration()
{
    delete ui;
}

void Configuration::setup(_CONFIGURE _cfg)
{
    con = _cfg;
    ui->channelCB->setCurrentIndex(con.monChanID);
    ui->insecLB->setText(QString::number(con.inSeconds));
    ui->numstaLB->setText(QString::number(con.numSta));
    ui->thresholdgLB->setText(QString::number(con.thresholdG, 'f', 1));
    ui->distanceLB->setText(QString::number(con.distance));
    ui->thresholdmLB->setText(QString::number(con.thresholdM, 'f', 1));
}

void Configuration::saveConfigure()
{
    QString temp = ui->channelCB->currentText();

    if(temp.left(1) == "U")
        con.monChanID = 0;
    else if(temp.left(1) == "N")
        con.monChanID = 1;
    else if(temp.left(1) == "E")
        con.monChanID = 2;
    else if(temp.left(1) == "H")
        con.monChanID = 3;
    else if(temp.left(1) == "T")
        con.monChanID = 4;

    emit sendCFGtoMain(con);
    accept();
}
