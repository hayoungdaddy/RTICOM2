#ifndef RecvMessage_H
#define RecvMessage_H

#include "common.h"

#include <QThread>

#include <decaf/lang/Thread.h>
#include <decaf/lang/Runnable.h>
#include <decaf/util/concurrent/CountDownLatch.h>
#include <activemq/core/ActiveMQConnectionFactory.h>
#include <activemq/core/ActiveMQConnection.h>
#include <activemq/transport/DefaultTransportListener.h>
#include <activemq/library/ActiveMQCPP.h>
#include <decaf/lang/Integer.h>
#include <activemq/util/Config.h>
#include <decaf/util/Date.h>
#include <cms/Connection.h>
#include <cms/Session.h>
#include <cms/TextMessage.h>
#include <cms/BytesMessage.h>
#include <cms/MapMessage.h>
#include <cms/ExceptionListener.h>
#include <cms/MessageListener.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include <QXmlStreamReader>
#include <QXmlStreamAttributes>


using namespace activemq;
using namespace activemq::core;
using namespace activemq::transport;
using namespace decaf::lang;
using namespace decaf::util;
using namespace decaf::util::concurrent;
using namespace cms;
using namespace std;

class RecvMessage : public QThread
{
    Q_OBJECT
public:
    RecvMessage(QWidget *parent = nullptr);
    ~RecvMessage();

public:
    void setup(QString, QString, QString, QString, bool, bool);

private:
    Connection *connection;
    Session *session;
    Destination *destination;
    MessageConsumer *consumer;
    bool isKISS;

    void cleanUp();

    int count = 0;

protected:
    void run();

signals:
    void sendQSCDtoMain(QMultiMap<int, _QSCD_FOR_MULTIMAP>);
};

class RecvEEWMessage : public QThread
{
    Q_OBJECT
public:
    RecvEEWMessage(QWidget *parent = 0);
    ~RecvEEWMessage();

public:
    Connection *connection;
    Session *session;
    Destination *destination;
    MessageConsumer *consumer;
    QString ip;

    void setup(QString, QString, QString, QString, bool, bool);
    void cleanUp();

protected:
    void run();

signals:
    void _rvEEWInfo(_EEWInfo);
};

class RecvUpdateMessage : public QThread
{
    Q_OBJECT
public:
    RecvUpdateMessage(QWidget *parent = 0);
    ~RecvUpdateMessage();

public:
    Connection *connection;
    Session *session;
    Destination *destination;
    MessageConsumer *consumer;
    QString ip;

    void setup(QString, QString, QString, QString, bool, bool);
    void cleanUp();

protected:
    void run();

signals:
    void _rvUpdateInfo(_UPDATE_MESSAGE);
};

#endif // RecvMessage_H
