#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include "common.h"

#include <QDialog>
#include <QFile>

namespace Ui {
class Configuration;
}

class Configuration : public QDialog
{
    Q_OBJECT

public:
    explicit Configuration(QWidget *parent = 0);
    ~Configuration();

    void setup(_CONFIGURE);

private:
    Ui::Configuration *ui;
    _CONFIGURE con;

private slots:
    void saveConfigure();

signals:
    void sendCFGtoMain(_CONFIGURE);
};

#endif // CONFIGURATION_H
