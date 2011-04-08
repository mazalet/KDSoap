#include "wsdl_mywsdl_rpc.h"
#include "httpserver_p.h"
#include <QtTest/QtTest>
#include <QEventLoop>
#include <QDebug>

using namespace KDSoapUnitTestHelpers;

static const char* xmlEnvBegin =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<soap:Envelope"
        " xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\""
        " xmlns:soap-enc=\"http://schemas.xmlsoap.org/soap/encoding/\""
        " xmlns:xsd=\"http://www.w3.org/1999/XMLSchema\""
        " xmlns:xsi=\"http://www.w3.org/1999/XMLSchema-instance\""
        " soap:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"";
static const char* xmlEnvEnd = "</soap:Envelope>";

class WsdlRPCTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    // Using wsdl-generated code, make a call, and check the xml that was sent,
    // and check that the server's response was correctly parsed.
    void testRequest()
    {
        // Prepare response
        QByteArray responseData = QByteArray(xmlEnvBegin) + "><soap:Body>"
                                  "<kdab:addEmployeeResponse xmlns:kdab=\"http://www.kdab.com/xml/MyWsdl/\"><kdab:bStrReturn>Foo</kdab:bStrReturn></kdab:addEmployeeResponse>"
                                  " </soap:Body>" + xmlEnvEnd;
        HttpServerThread server(responseData, HttpServerThread::Public);

        // For testing the http server with telnet or wget:
        //httpGet(server.endPoint());
        //QEventLoop testLoop;
        //testLoop.exec();

        MyWsdl service;
        service.setEndPoint(server.endPoint());
        KDAB__EmployeeAchievements achievements;
        QList<KDAB__EmployeeAchievement> lst;
        KDAB__EmployeeAchievement achievement1;
        achievement1.setType(QString::fromLatin1("Project"));
        achievement1.setLabel(QString::fromLatin1("Management"));
        lst.append(achievement1);
        KDAB__EmployeeAchievement achievement2;
        achievement2.setType(QString::fromLatin1("Development"));
        achievement2.setLabel(QString::fromLatin1("C++"));
        lst.append(achievement2);
        achievements.setItems(lst);
        KDAB__EmployeeType employeeType;
        employeeType.setType(KDAB__EmployeeTypeEnum::Developer);
        employeeType.setOtherRoles(QList<KDAB__EmployeeTypeEnum>() << KDAB__EmployeeTypeEnum::TeamLeader);
        KDAB__EmployeeTypeEnumList otherRoles;
        otherRoles.setEntries(QList<KDAB__EmployeeTypeEnum>() << KDAB__EmployeeTypeEnum::TeamLeader << KDAB__EmployeeTypeEnum::Developer);
        employeeType.setOtherRolesAsList(otherRoles);
        KDAB__LottoNumbers lottoNumbers;
        lottoNumbers.setEntries(QList<int>() << 7 << 21 << 30 << 42);
        employeeType.setLottoNumbers(lottoNumbers);
        employeeType.setTeam(QString::fromLatin1("Minitel"));

        KDAB__LoginElement login;
        login.setUser(QLatin1String("foo"));
        login.setPass(QLatin1String("bar"));
        KDAB__SessionElement session;
        session.setSessionId(QLatin1String("id"));

        service.setLoginHeader(login);
        service.setSessionHeader(session);

        QString ret = service.addEmployee(employeeType,
                                          QString::fromLatin1("David Faure"),
                                          QString::fromLatin1("France"),
                                          achievements);
        if (!service.lastError().isEmpty())
            qDebug() << service.lastError();
        QVERIFY(service.lastError().isEmpty());
        QCOMPARE(ret, QString::fromLatin1("Foo"));
        const QByteArray expectedHeader =
                "<soap:Header>"
                "<n1:LoginHeader xsi:type=\"n1:LoginElement\">"
                "<n1:user xsi:type=\"xsd:string\">foo</n1:user>"
                "<n1:pass xsi:type=\"xsd:string\">bar</n1:pass>"
                "</n1:LoginHeader>"
                "<n1:SessionHeader xsi:type=\"n1:SessionElement\">"
                "<n1:sessionId xsi:type=\"xsd:string\">id</n1:sessionId>"
                "</n1:SessionHeader>"
                "</soap:Header>";
        // Check what we sent
        QByteArray requestXmlTemplate =
            QByteArray(xmlEnvBegin) +
            " xmlns:n1=\"http://www.kdab.com/xml/MyWsdl/\">%1"
            "<soap:Body>"
            "<n1:addEmployee>"
            + serializedEmployee() +
            "</n1:addEmployee>"
            "</soap:Body>" + xmlEnvEnd
            + '\n'; // added by QXmlStreamWriter::writeEndDocument
        {
            QByteArray expectedRequestXml = requestXmlTemplate;
            expectedRequestXml.replace("%1", expectedHeader);
            QVERIFY(xmlBufferCompare(server.receivedData(), expectedRequestXml));
            QCOMPARE(QString::fromUtf8(server.receivedData().constData()), QString::fromUtf8(expectedRequestXml.constData()));
            QVERIFY(server.receivedHeaders().contains("SoapAction: http://www.kdab.com/AddEmployee"));
        }

        // Test utf8
        {
            // This second call also tests that persistent headers are indeed persistent.
            server.resetReceivedBuffers();
            requestXmlTemplate.replace("David Faure", "Hervé");
            requestXmlTemplate.replace("France", "фгн7");
            QByteArray expectedRequestXml = requestXmlTemplate;
            expectedRequestXml.replace("%1", expectedHeader);
            ret = service.addEmployee(employeeType,
                                      QString::fromUtf8("Hervé"),
                                      QString::fromUtf8("фгн7"), // random russian letters
                                      achievements);
            QVERIFY(service.lastError().isEmpty());
            QCOMPARE(ret, QString::fromLatin1("Foo"));
            QVERIFY(xmlBufferCompare(server.receivedData(), expectedRequestXml));
        }

        // Test removing headers
        {
            server.resetReceivedBuffers();
            service.clearLoginHeader();
            service.clearSessionHeader();
            ret = service.addEmployee(employeeType,
                                      QString::fromUtf8("Hervé"),
                                      QString::fromUtf8("фгн7"), // random russian letters
                                      achievements);
            QByteArray expectedRequestXml = requestXmlTemplate;
            expectedRequestXml.replace("%1", "<soap:Header/>");
            QVERIFY(xmlBufferCompare(server.receivedData(), expectedRequestXml));
        }
    }

    void testResponse()
    {
        // Prepare response
        QByteArray responseData = QByteArray(xmlEnvBegin) + "><soap:Body>"
                                  "<n1:getEmployeeResponse xmlns:n1=\"http://www.kdab.com/xml/MyWsdl/\">"
                                  "<n1:employee>"
                                   + serializedEmployee() +
                                  "</n1:employee>"
                                  "</n1:getEmployeeResponse>"
                                  " </soap:Body>" + xmlEnvEnd;
        HttpServerThread server(responseData, HttpServerThread::Public);
        MyWsdl service;
        service.setEndPoint(server.endPoint());
        const KDAB__Employee employee = service.getEmployee(QString::fromLatin1("David Faure"));
        const KDAB__EmployeeType employeeType = employee.employeeType();
        QCOMPARE(employeeType.team().value().value(), QString::fromLatin1("Minitel"));
        QCOMPARE(employeeType.type().type(), KDAB__EmployeeTypeEnum::Developer);
        QCOMPARE(employeeType.otherRoles(), QList<KDAB__EmployeeTypeEnum>() << KDAB__EmployeeTypeEnum::TeamLeader);
        QCOMPARE(employeeType.otherRolesAsList().entries(), QList<KDAB__EmployeeTypeEnum>() << KDAB__EmployeeTypeEnum::TeamLeader << KDAB__EmployeeTypeEnum::Developer);
        QCOMPARE(employeeType.lottoNumbers().entries(), QList<int>() << 7 << 21 << 30 << 42);
        QCOMPARE(employee.employeeName().value().value(), QString::fromLatin1("David Faure"));
        QCOMPARE(employee.employeeCountry().value(), QString::fromLatin1("France"));
    }

    // Test enum deserialization
    void testEnums()
    {
        // Prepare response
        QByteArray responseData = QByteArray(xmlEnvBegin) + "><soap:Body>"
                                  "<kdab:getEmployeeTypeResponse xmlns:kdab=\"http://www.kdab.com/xml/MyWsdl/\">"
                                    "<kdab:employeeType kdab:type=\"Developer\">"
                                      "<kdab:team>Minitel</kdab:team>"
                                      "<kdab:otherRoles>TeamLeader</kdab:otherRoles>"
                                    "</kdab:employeeType>"
                                  "</kdab:getEmployeeTypeResponse>"
                                  "</soap:Body>" + xmlEnvEnd;
        HttpServerThread server(responseData, HttpServerThread::Public);
        MyWsdl service;
        service.setEndPoint(server.endPoint());

        KDAB__EmployeeType employeeType = service.getEmployeeType(KDAB__EmployeeName(QLatin1String("Joe")));
        if (!service.lastError().isEmpty())
            qDebug() << service.lastError();
        QVERIFY(service.lastError().isEmpty());
        QCOMPARE(employeeType.team().value().value(), QLatin1String("Minitel"));
        QCOMPARE(employeeType.otherRoles().count(), 1);
        QCOMPARE(employeeType.otherRoles().at(0).type(), KDAB__EmployeeTypeEnum::TeamLeader);
        QCOMPARE((int)employeeType.type().type(), (int)KDAB__EmployeeTypeEnum::Developer);
    }

private:
    static QByteArray serializedEmployeeType() {
        return QByteArray(
                "<n1:employeeType xsi:type=\"n1:EmployeeType\" n1:type=\"Developer\">"
                "<n1:team xsi:type=\"n1:TeamName\">Minitel</n1:team>"
                "<n1:otherRoles xsi:type=\"n1:EmployeeTypeEnum\">TeamLeader</n1:otherRoles>"
                "<n1:otherRolesAsList xsi:type=\"n1:EmployeeTypeEnumList\">TeamLeader Developer</n1:otherRolesAsList>"
                "<n1:lottoNumbers xsi:type=\"n1:LottoNumbers\">7 21 30 42</n1:lottoNumbers>"
                "</n1:employeeType>");
    }
    static QByteArray serializedEmployee() {
        return serializedEmployeeType() +
                "<n1:employeeName xsi:type=\"n1:EmployeeName\">David Faure</n1:employeeName>"
                "<n1:employeeCountry xsi:type=\"n1:LimitedString\">France</n1:employeeCountry>"
                "<n1:employeeAchievements xsi:type=\"n1:EmployeeAchievements\" soap-enc:arrayType=\"n1:EmployeeAchievement[2]\">"
                "<n1:item xsi:type=\"n1:EmployeeAchievement\">"
                "<n1:type xsi:type=\"xsd:string\">Project</n1:type>"
                "<n1:label xsi:type=\"xsd:string\">Management</n1:label>"
                "</n1:item>"
                "<n1:item xsi:type=\"n1:EmployeeAchievement\">"
                "<n1:type xsi:type=\"xsd:string\">Development</n1:type>"
                "<n1:label xsi:type=\"xsd:string\">C++</n1:label>"
                "</n1:item>"
                "</n1:employeeAchievements>";
    }
};


QTEST_MAIN(WsdlRPCTest)

#include "test_wsdl_rpc.moc"