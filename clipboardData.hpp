#pragma once

#include <mutex>
#include <vector>
#include <optional>

class ClipboardData
{
public:
    enum class Mode
    {
        Clipboard, // Ctrl+c, Ctrl+v clipbard
        Selection, // X11 selection clipboard that contains mouse selection. Pasted via middle click
    };

    virtual void run() = 0;
    virtual void quit() = 0;

    // Lock object so data won't change while using it.
    // Must call before using any function below (any function not init(), run(), and quit())
    virtual std::lock_guard<std::mutex> getLock(Mode mode = Mode::Clipboard) = 0;

    virtual bool hasData(Mode mode = Mode::Clipboard) = 0;

    virtual const std::vector<std::string> mainMimeTypes(Mode mode = Mode::Clipboard) = 0;
    virtual size_t mainMimeTypesCount(Mode mode = Mode::Clipboard) = 0;

    virtual const std::vector<std::string> mimeSubTypes(const std::string& mimeMainType, Mode mode = Mode::Clipboard) = 0;
    virtual size_t mimeSubTypesCount(const std::string& mimeMainType, Mode mode = Mode::Clipboard) = 0;

    virtual bool hasMainMimeType(const std::string& mimeMainType, Mode mode = Mode::Clipboard) = 0;

    virtual bool hasMimeSubType(const std::string& mimeSubType, Mode mode = Mode::Clipboard) = 0;

    virtual bool hasFullMimeType(const std::string& fullMimeType, Mode mode = Mode::Clipboard) = 0;

    virtual std::optional<size_t> dataSize(const std::string& fullMimeType, Mode mode = Mode::Clipboard) = 0;
    virtual std::optional<const std::vector<uint8_t>> mimeData(const std::string& fullMimeType, Mode mode = Mode::Clipboard) = 0;

    virtual ~ClipboardData() = default;
};
