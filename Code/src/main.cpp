#include "main_window.hpp"

#include <QApplication>
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
    if (!argument.isEmpty() && QFileInfo(argument).isDir()) {
        window.setModelFolder(argument);
        window.selectInitialModel({});
    } else {
        window.setModelFolder(QSettings().value(QStringLiteral("modellordner"))
                                  .toString());
        window.selectInitialModel(argument);
    }
    return application.exec();
}
