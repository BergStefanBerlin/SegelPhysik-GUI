#include "main_window.hpp"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QSurfaceFormat>
#include <QVTKOpenGLNativeWidget.h>
#include <vtkOutputWindow.h>

int main(int argc, char* argv[])
{
    vtkOutputWindow::GetInstance()->SetDisplayModeToAlwaysStdErr();
    QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());
    QApplication application(argc, argv);
    application.setApplicationName(QStringLiteral("SegelPhysik"));
    application.setOrganizationName(QStringLiteral("SegelPhysik"));

    MainWindow window;
    window.show();
    const auto arguments = QApplication::arguments();
    const QString argument = arguments.size() > 1 ? arguments.at(1) : QString();
    const QFileInfo info(argument);
    const QString remembered = QSettings().value(QStringLiteral("modellordner")).toString();
    if (argument.isEmpty() || !info.exists()) {
        window.setModelFolder(remembered);
        window.selectInitialModel({});
    } else if (info.isDir() && !QFileInfo::exists(QDir(argument).filePath(QStringLiteral("modell.json")))) {
        // Katalogordner
        window.setModelFolder(argument);
        window.selectInitialModel({});
    } else {
        // Modellordner oder modell.json: Katalog ist der ?bergeordnete Ordner;
        // die Vorauswahl folgt nach dem Katalogabruf.
        const QString modelDir = info.isDir() ? info.absoluteFilePath() : info.absolutePath();
        window.setModelFolder(QFileInfo(modelDir).absolutePath());
        window.selectInitialModel(info.absoluteFilePath());
    }
    return application.exec();
}
