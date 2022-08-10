#include <QGuiApplication>
#include <QDebug>
#include <QClipboard>
#include <QMimeData>
#include <QString>

#define FUSE_USE_VERSION 31
#include <fuse.h>
#include <thread>
#include <future>
#include <string>
#include <utility> // std::piecewise_construct, std::forward_as_tuple
#include <mutex> // std::lock_guard
#include <chrono> // std::chrono::milliseconds

#include "fuse.hpp"

void onClipboardChanged()
{
    using namespace FuseImplementation;
    std::lock_guard clipboardLock(clipboardDataMapMutex);
    const QClipboard* clipboard = QGuiApplication::clipboard();
    const QMimeData* mimeData = clipboard->mimeData();
    qDebug() << "Clipboard changed";
    const QStringList formats = mimeData->formats();
    clipboardDataMap.clear();
    clipboardDataMap.reserve(formats.size());
    clipboardMimeDirs.clear();
    for (auto it = formats.constBegin(); it != formats.constEnd(); ++it)
    {
        const QString format = *it;
        auto slashIndex = format.indexOf('/');
        if (slashIndex == -1)
        {
            continue;
        }
        std::string folder = format.first(slashIndex).toStdString();
        if (clipboardMimeDirs.find(folder) == clipboardMimeDirs.cend())
        {
            clipboardMimeDirs.emplace(std::move(folder));

        }
        qDebug() << "Format" << *it;
        QByteArray data = mimeData->data(format);
        clipboardDataMap.emplace(std::piecewise_construct,
                                 std::forward_as_tuple(format.toStdString()),
                                 std::forward_as_tuple(data.begin(), data.end()));
    }
}

//int fuseMainThread(int argc, char* argv[], const fuse_operations* op, void* privateData = NULL)
int fuseMainThread(int argc, char* argv[], const fuse_operations* op, void* privateData = nullptr)
{
    fuse_args args = FUSE_ARGS_INIT(argc, argv);
    if (fuse_opt_parse(&args, nullptr, nullptr, nullptr) == -1)
    {
        return 1;
    }

    //int ret = fuse_main(args.argc, args.argv, &FuseImplementation::operations, NULL);
    int ret = fuse_main(args.argc, args.argv, op, privateData);
    // Quit() is threadsafe, but exit() is not
    fuse_opt_free_args(&args);
    QGuiApplication::quit();
    return ret;
}

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);

    {
        const QClipboard* clipboard = QGuiApplication::clipboard();
        const QMimeData* mimeData = clipboard->mimeData();
        qDebug() << (mimeData->hasImage() ? "Has image" : "No image");
    }

    //FuseImplementation::FuseInitData privateData{QGuiApplication::clipboard()};
    auto future = std::async(fuseMainThread, argc, argv, &FuseImplementation::operations, nullptr);
    const QClipboard* clipboard = QGuiApplication::clipboard();
    QObject::connect(clipboard, &QClipboard::dataChanged, onClipboardChanged);

    // Set initial clipboard data
    onClipboardChanged();

    // Check early in case fuse exits early (for incorrect option or another reason)
    // If fuse exits very quickly and calls QGuiApplication::quit(), maybe before app.exec() runs, the program never exits (not sure what circumstnaces causes the issues)
    // This is an attempt to return if Fuse exits early and never run app.exec()
    std::chrono::milliseconds waitSpan(50);
    if (future.wait_for(waitSpan) == std::future_status::ready)
    {
        return future.get();
    }
    app.exec();

    return future.get();
}
