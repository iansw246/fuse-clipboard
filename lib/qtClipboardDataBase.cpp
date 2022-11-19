#include "qtClipboardDataBase.hpp"

#include <QMimeData>

namespace
{
bool fullMimeTypeHasMainMimeType(const QString& fullMimeType, const QString& mainMimeType)
{
    // +2 to make sure there is room for a slash and at least one character after the slash
    return fullMimeType.startsWith(mainMimeType) && fullMimeType.size() >= mainMimeType.size() + 2 &&
           fullMimeType[mainMimeType.size()] == '/';
}
} // namespace

QtClipboardDataBase::QtClipboardDataBase(QClipboard::Mode mode)
{
    const QClipboard* clipboard = QGuiApplication::clipboard();

    switch (mode)
    {
    case QClipboard::Mode::Clipboard:
        QObject::connect(clipboard, &QClipboard::dataChanged, std::bind(&QtClipboardDataBase::onClipboardChanged, this));
        break;
    case QClipboard::Mode::Selection:
        QObject::connect(clipboard, &QClipboard::selectionChanged, std::bind(&QtClipboardDataBase::onClipboardChanged, this));
        break;
    default:
        throw;
    }
    onClipboardChanged();
}

std::lock_guard<std::mutex> QtClipboardDataBase::getLock()
{
    return std::lock_guard(m_mutex);
}

bool QtClipboardDataBase::hasData()
{
    return m_fullMimeTypeToDataMap.size() != 0;
}

std::optional<size_t> QtClipboardDataBase::dataSize(const QString& fullMimeType)
{
    auto it = m_fullMimeTypeToDataMap.constFind(fullMimeType);
    if (it != m_fullMimeTypeToDataMap.cend())
    {
        return (*it).size();
    }
    else
    {
        return {};
    }
}

const QHash<QString, QByteArray>& QtClipboardDataBase::fullMimeTypeToDataMap()
{
    return m_fullMimeTypeToDataMap;
}

const QSet<QString>& QtClipboardDataBase::mainMimeTypes()
{
    return m_mainMimeTypes;
}

#if 0
bool QtClipboardDataBase::hasMimeSubType(const QString& mimeSubType)
{
    for (const QString& fullMimeType : std::as_const(m_fullMimeTypeToDataMap))
    {
        if (fullMimeType.endsWith(mimeSubType) && fullMimeType.size() >= mimeSubType.size() + 2 && fullMimeType[fullMimeType.size() - 1 - mimeSubType.size()] == '/')
        {
            return true;
        }
    }
    return false;
}

bool QtClipboardDataBase::hasFullMimeType(const QString& fullMimeType)
{
    return m_fullMimeTypeToDataMap.contains(fullMimeType);
}
#endif
const QByteArray* QtClipboardDataBase::mimeData(const QString& fullMimeType)
{
    auto it = m_fullMimeTypeToDataMap.constFind(fullMimeType);
    if (it != m_fullMimeTypeToDataMap.constEnd())
    {
        return &(*it);
    }
    return nullptr;
}

void QtClipboardDataBase::onClipboardChanged()
{
    std::lock_guard lock(m_mutex);
    const QClipboard* clipboard = QGuiApplication::clipboard();
    const QMimeData* mimeData = clipboard->mimeData();
    const QStringList formats = mimeData->formats();
    m_fullMimeTypeToDataMap.clear();
    m_fullMimeTypeToDataMap.reserve(formats.size());
    m_mainMimeTypes.clear();

    for (auto it = formats.constBegin(); it != formats.constEnd(); ++it)
    {
        const QString& fullMimeType = *it;
        auto slashIndex = fullMimeType.indexOf('/');
        if (slashIndex == -1)
        {
            continue;
        }

        {
            QString mainMimeType = fullMimeType.first(slashIndex);
            m_mainMimeTypes.insert(std::move(mainMimeType));
            QByteArray data = mimeData->data(fullMimeType);
            m_fullMimeTypeToDataMap.insert(fullMimeType, std::move(data));
        }
    }
}


