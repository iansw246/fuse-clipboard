#pragma once

#include <QByteArray>
#include <QClipboard>
#include <QGuiApplication>
#include <QHash>
#include <QObject>
#include <QSet>
#include <QString>

#include <functional> // std::bind
#include <mutex>
#include <optional>

class QtClipboardDataBase
{
  public:
    QtClipboardDataBase(QClipboard::Mode mode);

    // Consider QReadWriteLock
    std::lock_guard<std::mutex> getLock();

    bool hasData();
    std::optional<size_t> dataSize(const QString& fullMimeType);

    const QHash<QString, QByteArray>& fullMimeTypeToDataMap();
    const QSet<QString>& mainMimeTypes();

    // Pointer only valid while lock is held
    // Null pointer indicates no data
    const QByteArray* mimeData(const QString& fullMimeType);

  private:
    std::mutex m_mutex;
    QHash<QString, QByteArray> m_fullMimeTypeToDataMap;
    QSet<QString> m_mainMimeTypes;

    void onClipboardChanged();
};
