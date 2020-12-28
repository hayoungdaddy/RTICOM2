#include "recvmessage.h"

#include "QtConcurrent/qtconcurrentrun.h"

RecvMessage::RecvMessage(QWidget *parent)
{
}

RecvMessage::~RecvMessage()
{
}

void RecvMessage::setup(QString qBrokerURI, QString qID, QString qPW, QString qTopic, bool useTopic, bool clientAck)
{
    auto_ptr<ConnectionFactory> connectionFactory(ConnectionFactory::createCMSConnectionFactory( qBrokerURI.toStdString() ) );

    if(qID == "" || qPW == "")
    {
        connection = connectionFactory->createConnection();
    }
    else
    {
        connection = connectionFactory->createConnection(qID.toStdString(), qPW.toStdString());
    }

    connection->start();

    if(qTopic.section(".", 1, 1).startsWith("KISS"))
        isKISS = true;
    else
        isKISS = false;

    // Create a Session
    session = connection->createSession( Session::AUTO_ACKNOWLEDGE );

    useTopic = true;

    // Create the destination (Topic or Queue)
    if( useTopic ) {
        destination = session->createTopic( qTopic.toStdString() );
    } else {
        destination = session->createQueue( qTopic.toStdString() );
    }
    consumer = session->createConsumer (destination);
}

void RecvMessage::run()
{ 
    while (this->isRunning())
    {
        if(this->isInterruptionRequested())
            this->quit();

        auto_ptr<Message> message( consumer->receive() );
        //auto_ptr<Message> message( consumer->receiveNoWait() );

        if( message.get() != NULL )
        {
            const BytesMessage* bytes_message = dynamic_cast< const BytesMessage* >( message.get() );
            char *msg = (char *)bytes_message->getBodyBytes();
            QFuture<QMultiMap<int, _QSCD_FOR_MULTIMAP>> future = QtConcurrent::run(convertMMap, msg, isKISS);
            future.waitForFinished();
            emit sendQSCDtoMain(future.result());

            free(msg);
        }
    }
}

void RecvMessage::cleanUp()
{
    if (connection != nullptr)
        connection->close();

    delete destination;
    destination = nullptr;
    delete consumer;
    consumer = nullptr;
    delete session;
    session = nullptr;
    delete connection;
}


RecvEEWMessage::RecvEEWMessage(QWidget *parent)
{
}

RecvEEWMessage::~RecvEEWMessage()
{
    cleanUp();
}

void RecvEEWMessage::setup(QString qBrokerURI, QString qID, QString qPW, QString qTopic, bool useTopic, bool clientAck)
{
    auto_ptr<ConnectionFactory> connectionFactory(ConnectionFactory::createCMSConnectionFactory( qBrokerURI.toStdString() ));

    if(qID == "" || qPW == "")
        connection = connectionFactory->createConnection();
    else
        connection = connectionFactory->createConnection(qID.toStdString(), qPW.toStdString());

    connection->start();

    // Create a Session
    if( clientAck ) {
        session = connection->createSession( Session::CLIENT_ACKNOWLEDGE );
    } else {
        session = connection->createSession( Session::AUTO_ACKNOWLEDGE );
    }

    useTopic = true;

    // Create the destination (Topic or Queue)
    if( useTopic ) {
        destination = session->createTopic( qTopic.toStdString() );
    } else {
        destination = session->createQueue( qTopic.toStdString() );
    }

    consumer = session->createConsumer (destination);
}

void RecvEEWMessage::run()
{
    while (this->isRunning())
    {
        if(this->isInterruptionRequested())
            this->quit();

        auto_ptr<Message> message (consumer->receive() );
        if( message.get() != nullptr )
        {
            const TextMessage *m = dynamic_cast< const TextMessage* >( message.get() );
            QString xmlbody = QString::fromStdString(m->getText());

            _EEWInfo _eewinfo;

            xmlbody = xmlbody.simplified();
            xmlbody.replace(QString("\n"), QString(""));

            QXmlStreamReader xml(xmlbody);
            while(!xml.atEnd() && !xml.hasError())
            {
                xml.readNext();
                if(xml.name() == "event_message")
                {
                    QXmlStreamAttributes attributes = xml.attributes();
                    if(attributes.value("category").toString().startsWith("live"))
                        _eewinfo.message_category = LIVE;
                    else
                        _eewinfo.message_category = TEST;

                    if(attributes.value("message_type").toString().startsWith("new"))
                        _eewinfo.message_type = NEW;
                    else if(attributes.value("message_type").toString().startsWith("update"))
                        _eewinfo.message_type = UPDATE;
                    else
                        _eewinfo.message_type = DELETE;

                    _eewinfo.version = attributes.value("version").toInt();
                }
                else if(xml.name() == "core_info")
                {
                    QXmlStreamAttributes attributes = xml.attributes();
                    if(attributes.hasAttribute("id"))
                        _eewinfo.eew_evid = attributes.value("id").toInt();
                }
                else if(xml.name() == "mag")
                {
                    xml.readNext();
                    QString temp = xml.text().toString();
                    if(!temp.startsWith(" "))
                        _eewinfo.magnitude = temp.toDouble();
                }
                else if(xml.name() == "mag_uncer")
                {
                    xml.readNext();
                    QString temp = xml.text().toString();
                    if(!temp.startsWith(" "))
                        _eewinfo.magnitude_uncertainty = temp.toDouble();
                }
                else if(xml.name() == "lat")
                {
                    xml.readNext();
                    QString temp = xml.text().toString();
                    if(!temp.startsWith(" "))
                        _eewinfo.latitude = temp.toDouble();
                }
                else if(xml.name() == "lat_uncer")
                {
                    xml.readNext();
                    QString temp = xml.text().toString();
                    if(!temp.startsWith(" "))
                        _eewinfo.latitude_uncertainty = temp.toDouble();
                }
                else if(xml.name() == "lon")
                {
                    xml.readNext();
                    QString temp = xml.text().toString();
                    if(!temp.startsWith(" "))
                        _eewinfo.longitude = temp.toDouble();
                }
                else if(xml.name() == "lon_uncer")
                {
                    xml.readNext();
                    QString temp = xml.text().toString();
                    if(!temp.startsWith(" "))
                        _eewinfo.longitude_uncertainty = temp.toDouble();
                }
                else if(xml.name() == "depth")
                {
                    xml.readNext();
                    QString temp = xml.text().toString();
                    if(!temp.startsWith(" "))
                        _eewinfo.depth = temp.toDouble();
                }
                else if(xml.name() == "depth_uncer")
                {
                    xml.readNext();
                    QString temp = xml.text().toString();
                    if(!temp.startsWith(" "))
                        _eewinfo.depth_uncertainty = temp.toDouble();
                }
                else if(xml.name() == "orig_time")
                {
                    xml.readNext();
                    QString temp = xml.text().toString();
                    if(!temp.startsWith(" "))
                    {
                        QDateTime dt = QDateTime::fromString(temp, "yyyy-MM-ddThh:mm:ss.zzzZ");
                        dt.setTimeSpec(Qt::UTC);
                        _eewinfo.origin_time = dt.toTime_t();
                        QString msec = "0." + dt.toString("zzz");
                        _eewinfo.origin_time = _eewinfo.origin_time + msec.toDouble();
                    }
                }
                else if(xml.name() == "orig_time_uncer")
                {
                    xml.readNext();
                    QString temp = xml.text().toString();
                    if(!temp.startsWith(" "))
                        _eewinfo.origin_time_uncertainty = temp.toDouble();
                }
                else if(xml.name() == "num_stations")
                {
                    xml.readNext();
                    QString temp = xml.text().toString();
                    if(!temp.startsWith(" "))
                        _eewinfo.number_stations = temp.toDouble();
                }
                else if(xml.name() == "num_triggers")
                {
                    xml.readNext();
                    QString temp = xml.text().toString();
                    if(!temp.startsWith(" "))
                        _eewinfo.number_triggers = temp.toDouble();
                }
                else if(xml.name() == "sent_flag")
                {
                    xml.readNext();
                    QString temp = xml.text().toString();
                    //qDebug() << temp;
                    if(!temp.startsWith(" "))
                    {
                        if(temp.startsWith("0"))
                            _eewinfo.sent_flag = true;
                        else
                            _eewinfo.sent_flag = false;
                    }
                }
                else if(xml.name() == "sent_time")
                {
                    xml.readNext();
                    QString temp = xml.text().toString();
                    if(!temp.startsWith(" "))
                        _eewinfo.sent_time = temp.toDouble();
                }
            }

            emit _rvEEWInfo(_eewinfo);
        }
    }
}

void RecvEEWMessage::cleanUp()
{
    if (connection != nullptr)
        connection->close();

    delete destination;
    destination = nullptr;
    delete consumer;
    consumer = nullptr;
    delete session;
    session = nullptr;
    delete connection;
}



RecvUpdateMessage::RecvUpdateMessage(QWidget *parent)
{
}

RecvUpdateMessage::~RecvUpdateMessage()
{
    //cleanUp();
}

void RecvUpdateMessage::setup(QString qBrokerURI, QString qID, QString qPW, QString qTopic, bool useTopic, bool clientAck)
{
    auto_ptr<ConnectionFactory> connectionFactory(ConnectionFactory::createCMSConnectionFactory( qBrokerURI.toStdString() ) );

    if(qID == "" || qPW == "")
    {
        connection = connectionFactory->createConnection();
    }
    else
    {
        connection = connectionFactory->createConnection(qID.toStdString(), qPW.toStdString());
    }

    connection->start();

    // Create a Session
    session = connection->createSession( Session::AUTO_ACKNOWLEDGE );

    useTopic = true;

    // Create the destination (Topic or Queue)
    if( useTopic ) {
        destination = session->createTopic( qTopic.toStdString() );
    } else {
        destination = session->createQueue( qTopic.toStdString() );
    }
    consumer = session->createConsumer (destination);
}

void RecvUpdateMessage::run()
{
    while (this->isRunning())
    {
        if(this->isInterruptionRequested())
            this->quit();

        auto_ptr<Message> message( consumer->receive() );

        if( message.get() != NULL )
        {
            const BytesMessage* bytes_message = dynamic_cast< const BytesMessage* >( message.get() );
            char *msg = (char *)bytes_message->getBodyBytes();

            _UPDATE_MESSAGE message;
            memcpy(&message, msg, sizeof(_UPDATE_MESSAGE));

            emit _rvUpdateInfo(message);

            free(msg);
        }
    }
}

void RecvUpdateMessage::cleanUp()
{
    if (connection != nullptr)
        connection->close();

    delete destination;
    destination = nullptr;
    delete consumer;
    consumer = nullptr;
    delete session;
    session = nullptr;
    delete connection;
}
