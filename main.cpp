#include "mainwindow.h"
#include "connection.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFontDatabase>
#include <QIODevice>
#include <QTextStream>
#include <QMessageBox>
#include <QStringList>

namespace {
void loadEnvFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QTextStream stream(&file);
    while (!stream.atEnd()) {
        const QString rawLine = stream.readLine().trimmed();
        if (rawLine.isEmpty() || rawLine.startsWith('#')) {
            continue;
        }

        const int separatorIndex = rawLine.indexOf('=');
        if (separatorIndex <= 0) {
            continue;
        }

        const QString key = rawLine.left(separatorIndex).trimmed();
        QString value = rawLine.mid(separatorIndex + 1).trimmed();

        if ((value.startsWith('"') && value.endsWith('"')) || (value.startsWith('\'') && value.endsWith('\''))) {
            value = value.mid(1, value.size() - 2);
        }

        if (!key.isEmpty()) {
            const QByteArray keyBytes = key.toUtf8();
            const QByteArray valueBytes = value.toUtf8();
            qputenv(keyBytes.constData(), valueBytes);
        }
    }
}

void loadLocalEnvironmentFrom(const QString &startDir)
{
    QDir dir(startDir);
    for (int depth = 0; depth < 6 && dir.exists(); ++depth) {
        const QString candidate = dir.filePath(QStringLiteral(".env"));
        if (QFile::exists(candidate)) {
            loadEnvFile(candidate);
            return;
        }

        if (!dir.cdUp()) {
            break;
        }
    }
}

void loadLocalEnvironment()
{
    loadLocalEnvironmentFrom(QDir::currentPath());
    loadLocalEnvironmentFrom(QCoreApplication::applicationDirPath());
}
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    loadLocalEnvironment();

    const int fontId = QFontDatabase::addApplicationFont(QStringLiteral(":/assets/CalSans-Regular.ttf"));
    if (fontId != -1) {
        const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
        if (!families.isEmpty()) {
            const QString calSansFamily = families.first();
            QString style = a.styleSheet();
            style.append(QStringLiteral("\n* { font-family: \"%1\"; }").arg(calSansFamily));
            a.setStyleSheet(style);
        }
    }




    Connection& c = Connection::createInstance();

    bool test = c.createConnection();

    if(test){
        QMessageBox::information(nullptr, QObject::tr("database is open"),
                                 QObject::tr("connection successful.\n"
                                             "Click Cancel to exit."), QMessageBox::Cancel);
    }
    else{
        QMessageBox::critical(nullptr, QObject::tr("database is not open"),
                              QObject::tr("connection failed.\n"
                                          "Click Cancel to exit."), QMessageBox::Cancel);

    }





    MainWindow w;
    w.show();
    return a.exec();
}
