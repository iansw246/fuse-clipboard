#pragma once

#include "clipboardData.hpp"
#include "qtClipboardDataBase.hpp"

class QtClipboardData : public ClipboardData
{
  public:
    void run();
    void quit();

    // Lock object so data won't change while using it.
    // Must call before using any function below (any function not run() or and quit())
    std::lock_guard<std::mutex> getLock(Mode mode = Mode::Clipboard);

    bool hasData(Mode mode = Mode::Clipboard);

    const std::vector<std::string> mainMimeTypes(Mode mode = Mode::Clipboard);
    size_t mainMimeTypesCount(Mode mode);

    const std::vector<std::string> mimeSubTypes(const std::string& mainMimeType, Mode mode = Mode::Clipboard);
    size_t mimeSubTypesCount(const std::string& mainMimeType, Mode mode);

    bool hasMainMimeType(const std::string& mainMimeType, Mode mode = Mode::Clipboard);

    bool hasMimeSubType(const std::string& mimeSubType, Mode mode = Mode::Clipboard);

    bool hasFullMimeType(const std::string& fullMimeType, Mode mode = Mode::Clipboard);

    // Optional has no data if no mimetype found
    std::optional<size_t> dataSize(const std::string& fullMimeType, Mode mode = Mode::Clipboard);
    std::optional<const std::vector<uint8_t>> mimeData(const std::string& fullMimeType, Mode mode = Mode::Clipboard);

    QtClipboardData(int& argc, char** argv);
    ~QtClipboardData();

  private:
    QGuiApplication m_qtApp;
    QtClipboardDataBase m_clipboardData;
    QtClipboardDataBase m_selectionData;

    QtClipboardDataBase& dataObjectForMode(ClipboardData::Mode mode);

};
