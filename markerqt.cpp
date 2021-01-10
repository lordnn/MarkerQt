/**
 * The MIT License (MIT)
 * Copyright (c) 2017-2018 Kirill Lebedev
**/

#include "mainwindow.h"

#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QGuiApplication::setApplicationDisplayName(MainWindow::tr("MarkerQt"));
    QCommandLineParser commandLineParser;
    commandLineParser.addHelpOption();
    commandLineParser.addPositionalArgument(MainWindow::tr("[file]"), MainWindow::tr("Image file to open."));
    commandLineParser.process(QCoreApplication::arguments());
    MainWindow w;
    if (!commandLineParser.positionalArguments().isEmpty())
        w.loadFile(commandLineParser.positionalArguments().front());
    w.showMaximized();
#ifdef Q_OS_SYMBIAN
    app.setNavigationMode(Qt::NavigationModeCursorAuto);
#endif
    return app.exec();
}
