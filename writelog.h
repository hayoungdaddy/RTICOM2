#ifndef WRITELOG_H
#define WRITELOG_H

#include <QObject>

#include <QFile>
#include <QTextStream>

class WriteLog : public QObject
{
    Q_OBJECT
public:
    WriteLog();
    ~WriteLog();

    void write(QString, QString);
};

#endif // WRITELOG_H
