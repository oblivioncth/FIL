#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSet>
#include <QFile>
#include <QtXml>
#include <assert.h>
#include <QFileInfo>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    // Setup
    ui->setupUi(this);

    QSet<QString> gameIDs;
    QSet<QString> additionalAppIDs;

    int additionalAppsStartLine = -1;

    const QString fullXMLPath = QFileInfo("..\\OFILb\\Authorware.xml").canonicalFilePath();

    QFile platformXML(fullXMLPath);
    if(platformXML.open(QFile::ReadOnly | QFile::Text))
    {
        QXmlStreamReader xmlReader;
        xmlReader.setDevice(&platformXML);

        if (xmlReader.readNextStartElement())
        {
            if (xmlReader.name() == "LaunchBox")
            {
                // Loop over entire XML document
                while(xmlReader.readNextStartElement())
                {
                    if(xmlReader.name() == "AdditionalApplication" && additionalAppsStartLine == -1)
                        additionalAppsStartLine = xmlReader.lineNumber();
                    else if(xmlReader.name() == "ID")
                        gameIDs.insert(xmlReader.readElementText());
                    else if(xmlReader.name() == "Id")
                        additionalAppIDs.insert(xmlReader.readElementText());
                    else
                        xmlReader.skipCurrentElement();
                }
            }
            else
                assert("nope");
        }
        else
            assert("nope");
    }
    else
        assert("no open");

    platformXML.close();
}

MainWindow::~MainWindow()
{
    delete ui;
}

