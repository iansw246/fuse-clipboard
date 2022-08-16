#include "qtClipboardData.hpp"

#include <functional> // std::bind
#include <utility>    // std::move
#include <cassert>

#include <QClipboard>
#include <QMimeData>
#include <QStringList>

namespace
{
bool fullMimeTypeHasSubType(const QString& fullMimeType, const QString& mimeSubType)
{
    return fullMimeType.endsWith(mimeSubType) && fullMimeType.size() >= mimeSubType.size() + 2 &&
           fullMimeType[fullMimeType.size() - 1 - mimeSubType.size()] == '/' && fullMimeType[fullMimeType.size() - 1 - mimeSubType.size() - 1] != '/';
}

bool fullMimeTypeHasMainType(const QString& fullMimeType, const QString& mainMimeType)
{
    return fullMimeType.startsWith(mainMimeType) && fullMimeType.size() > mainMimeType.size() + 1 &&
           fullMimeType[mainMimeType.size()] == '/' && fullMimeType[mainMimeType.size() + 1] != '/';
}
}

QtClipboardData::QtClipboardData(int& argc, char** argv) : m_qtApp(argc, argv), m_clipboardData(QClipboard::Mode::Clipboard), m_selectionData(QClipboard::Mode::Selection)
{

}

QtClipboardData::~QtClipboardData()
{
    m_qtApp.quit();
}

void QtClipboardData::run()
{
    m_qtApp.exec();
}

void QtClipboardData::quit()
{
    QGuiApplication::quit();
}

std::lock_guard<std::mutex> QtClipboardData::getLock(ClipboardData::Mode mode)
{
    switch (mode)
    {
    case ClipboardData::Mode::Clipboard:
        return m_clipboardData.getLock();
    case ClipboardData::Mode::Selection:
        return m_selectionData.getLock();
    default:
        throw;
    }
}

bool QtClipboardData::hasData(Mode mode)
{
    switch (mode)
    {
    case QtClipboardData::Mode::Clipboard:
        return m_clipboardData.hasData();
    case QtClipboardData::Mode::Selection:
        return m_selectionData.hasData();
    default:
        throw;
    }
}

const std::vector<std::string> QtClipboardData::mainMimeTypes(Mode mode)
{
    QtClipboardDataBase& dataObject = dataObjectForMode(mode);
    const auto hash = dataObject.mainMimeTypes();

    std::vector<std::string> result;
    result.reserve(hash.size());
    for (const QString& mainMimeType : hash)
    {
        result.emplace_back(mainMimeType.toStdString());
    }
    return result;
}

size_t QtClipboardData::mainMimeTypesCount(Mode mode)
{
    QtClipboardDataBase& dataObject = dataObjectForMode(mode);
    return dataObject.mainMimeTypes().size();
}

const std::vector<std::string> QtClipboardData::mimeSubTypes(const std::string& mainMimeType, Mode mode)
{
    QtClipboardDataBase& dataObject = dataObjectForMode(mode);
    std::vector<std::string> result;
    const QString mainMimeTypeQString = QString::fromStdString(mainMimeType);
    const auto& hash = dataObject.fullMimeTypeToDataMap();
    for (auto it = hash.keyBegin(), itEnd = hash.keyEnd(); it != itEnd; ++it)
    {
        const QString& fullMimeType = *it;
        if (fullMimeTypeHasMainType(fullMimeType, mainMimeTypeQString))
        {
            // -1 to skip slash
            result.emplace_back(fullMimeType.right(fullMimeType.size() - mainMimeTypeQString.size() - 1).toStdString());
        }
    }
    return result;
}

size_t QtClipboardData::mimeSubTypesCount(const std::string& mainMimeType, Mode mode)
{
    size_t count = 0;
    QtClipboardDataBase& dataObject = dataObjectForMode(mode);
    const QString mainMimeTypeQString = QString::fromStdString(mainMimeType);
    const auto& hash = dataObject.fullMimeTypeToDataMap();
    // Use std::count instead
    for (auto it = hash.keyBegin(), itEnd = hash.keyEnd(); it != itEnd; ++it)
    {
        const QString& fullMimeType = *it;
        if (fullMimeTypeHasMainType(fullMimeType, mainMimeTypeQString))
        {
            ++count;
        }
    }
    return count;
}

bool QtClipboardData::hasMainMimeType(const std::string& mainMimeType, Mode mode)
{
    QtClipboardDataBase& dataObject = dataObjectForMode(mode);

    const auto& mainTypes = dataObject.mainMimeTypes();
    return mainTypes.contains(QString::fromStdString(mainMimeType));
//    return std::any_of(hash.keyBegin(), hash.keyEnd(), [&mainMimeType](const QString& fullMimeType)
//        {
//            return fullMimeTypeHasMainType(fullMimeType, mainMimeType);
//        });
}

bool QtClipboardData::hasMimeSubType(const std::string& mimeSubType, Mode mode)
{
    QtClipboardDataBase& dataObject = dataObjectForMode(mode);

    const QString mimeSubTypeQString = QString::fromStdString(mimeSubType);

    const auto& hash = dataObject.fullMimeTypeToDataMap();
    return std::any_of(hash.keyBegin(), hash.keyEnd(), [&mimeSubTypeQString](const QString& fullMimeType)
        {
            return fullMimeTypeHasSubType(fullMimeType, mimeSubTypeQString);
        });
}

bool QtClipboardData::hasFullMimeType(const std::string& fullMimeType, Mode mode)
{
    QtClipboardDataBase& dataObject = dataObjectForMode(mode);
    return dataObject.fullMimeTypeToDataMap().contains(QString::fromStdString(fullMimeType));
}

std::optional<size_t> QtClipboardData::dataSize(const std::string& fullMimeType, Mode mode)
{
    QtClipboardDataBase& dataObject = dataObjectForMode(mode);
    const auto& hash = dataObject.fullMimeTypeToDataMap();
    auto it = hash.find(QString::fromStdString(fullMimeType));
    if (it == hash.constEnd())
    {
        return {};
    }
    return (*it).size();
}

std::optional<const std::vector<uint8_t>> QtClipboardData::mimeData(const std::string& fullMimeType, Mode mode)
{
    QtClipboardDataBase& dataObject = dataObjectForMode(mode);
    const auto& hash = dataObject.fullMimeTypeToDataMap();
    auto it = hash.find(QString::fromStdString(fullMimeType));
    if (it == hash.constEnd())
    {
        return {};
    }
    std::vector<uint8_t> result;
    result.assign(it->cbegin(), it->cend());
    return result;
}

QtClipboardDataBase& QtClipboardData::dataObjectForMode(ClipboardData::Mode mode)
{
    switch(mode)
    {
    case ClipboardData::Mode::Clipboard:
        return m_clipboardData;
    case ClipboardData::Mode::Selection:
        return m_selectionData;
    }
    assert(false);
}
