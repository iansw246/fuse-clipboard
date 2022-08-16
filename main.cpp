#include "fuse.hpp"
#include "clipboardData.hpp"
#include "qtClipboardData.hpp"

#include <thread>
#include <future>
#include <string>
#include <mutex> // std::lock_guard
#include <chrono> // std::chrono::milliseconds
#include <memory>
#include <iostream>

std::unique_ptr<ClipboardData> createClipboardData(int& argc, char* argv[])
{
    // For now, always use Qt
    return std::make_unique<QtClipboardData>(argc, argv);
}

#if 0
void onClipboardChanged()
{
    using namespace FuseImplementation;
    std::lock_guard clipboardLock(clipboardDataMapMutex);
    const QClipboard* clipboard = QGuiApplication::clipboard();
    const QMimeData* mimeData = clipboard->mimeData();
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
        QByteArray data = mimeData->data(format);
        clipboardDataMap.emplace(std::piecewise_construct,
                                 std::forward_as_tuple(format.toStdString()),
                                 std::forward_as_tuple(data.begin(), data.end()));
    }
}
#endif

//int fuseMainThread(int argc, char* argv[], const fuse_operations* op, void* privateData = NULL)
int fuseMainThread(int argc, char* argv[], const fuse_operations* op, void* privateData = nullptr)
{
    fuse_args args = FUSE_ARGS_INIT(argc, argv);
    if (fuse_opt_parse(&args, nullptr, nullptr, nullptr) == -1)
    {
        return 1;
    }
    // Force foreground so main thread doesn't exit, destroying the QApplication object
    fuse_opt_add_arg(&args, "-f");

    //int ret = fuse_main(args.argc, args.argv, &FuseImplementation::operations, NULL);
    int ret = fuse_main(args.argc, args.argv, op, privateData);
    fuse_opt_free_args(&args);
    if (privateData)
    {
        reinterpret_cast<FuseImplementation::FuseInitData*>(privateData)->clipboardData->quit();
    }
    return ret;
}

int main(int argc, char* argv[])
{
    std::unique_ptr<ClipboardData> clipboardData = createClipboardData(argc, argv);
    FuseImplementation::FuseInitData privateData{clipboardData.get()};
    auto future = std::async(fuseMainThread, argc, argv, &FuseImplementation::operations, &privateData);

    // Check early in case fuse exits early (for incorrect option or another reason)
    // If fuse exits very quickly and calls QGuiApplication::quit(), maybe before app.exec() runs, the program never exits (not sure what circumstnaces causes the issues)
    // This is an attempt to return if Fuse exits early and never run app.exec()
    std::chrono::milliseconds waitSpan(50);
    if (future.wait_for(waitSpan) == std::future_status::ready)
    {
        return future.get();
    }

    clipboardData->run();

    return future.get();
}
