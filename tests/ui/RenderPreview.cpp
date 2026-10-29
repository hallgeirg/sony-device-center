// Render the actual QML pages for visual review. Run with a separate simulated
// daemon and private XDG_RUNTIME_DIR; this helper never sends setting changes.
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QQuickItem>
#include <QTimer>
#include <QDir>
#include <QImage>
#include <QTest>
#include "DeviceCenterController.h"

static QQuickItem* findVisualItem(QQuickItem* parent, const QString& name) {
    if (parent->objectName() == name) return parent;
    for (auto* child : parent->childItems()) {
        if (auto* found = findVisualItem(child, name)) return found;
    }
    return nullptr;
}

int main(int argc, char** argv) {
    QGuiApplication app(argc, argv);
    if (argc != 2) return 2;
    const QDir output(QString::fromLocal8Bit(argv[1]));
    if (!QDir().mkpath(output.path())) return 2;
    QQuickStyle::setStyle("Basic");
    sony::devicecenter::DeviceCenterController controller;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("controller", &controller);
    engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));
    if (engine.rootObjects().isEmpty()) return 3;
    auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
    if (!window) return 3;
    const QStringList names{"overview", "noise-control", "equalizer", "audio-features", "devices", "settings",
                            "compact-overview", "compact-noise-control", "compact-equalizer",
                            "compact-audio-features", "compact-devices", "compact-settings"};
    QTimer timer;
    int page = -1;
    int attempts = 0;
    QObject::connect(&timer, &QTimer::timeout, &app, [&] {
        if (!controller.isConnected() || controller.busy()) {
            if (++attempts > 30) app.exit(4);
            return;
        }
        if (page >= 0) {
            const auto image = window->grabWindow();
            if (image.isNull() || !image.save(output.filePath(names[page] + ".png"))) {
                app.exit(5); return;
            }
        }
        if (++page == names.size()) { app.quit(); return; }
        if (page == 6) window->resize(980, 660);
        const int navigationIndex = page % 6;
        auto* navigation = findVisualItem(window->contentItem(), "navigation-" + QString::number(navigationIndex));
        if (!navigation) { app.exit(6); return; }
        navigation->forceActiveFocus();
        QTest::keyClick(window, Qt::Key_Return);
        if (window->property("navIndex").toInt() != navigationIndex) { app.exit(7); return; }
    });
    timer.start(500);
    return app.exec();
}
