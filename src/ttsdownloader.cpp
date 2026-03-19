#include "ttsdownloader.h"

#include <QFile>
#include <QNetworkRequest>
#include <QUrlQuery>

#include "utils.h"

TtsDownloader::TtsDownloader(QObject *parent) : QObject(parent) {
  m_network = new QNetworkAccessManager(this);
}

TtsDownloader::~TtsDownloader() {
  clearTempStorage();
}

void TtsDownloader::clearTempStorage() {
  if (m_tempDir != nullptr) {
    delete m_tempDir;
    m_tempDir = nullptr;
  }
}

void TtsDownloader::start(const QString &text, const QString &ttsLang,
                          const QString &voiceGender) {
  cancel();

  clearTempStorage();
  m_tempDir = new QTemporaryDir();

  if (!utils::splitString(text, kMaxChunkLength, m_chunks) || m_chunks.isEmpty()) {
    m_chunks = QStringList{text};
  }

  m_ttsLang = ttsLang;
  m_voiceGender = voiceGender;
  m_currentIndex = 0;
  m_active = true;

  if (!m_tempDir->isValid()) {
    emit failed("Unable to create temporary directory for TTS audio.");
    clearTempStorage();
    resetSession();
    return;
  }

  emit progress(QString("Preparing audio... 1/%1").arg(m_chunks.size()));
  downloadCurrentChunk();
}

void TtsDownloader::cancel() {
  if (m_reply != nullptr) {
    m_reply->abort();
    m_reply->deleteLater();
    m_reply = nullptr;
  }
  clearTempStorage();
  resetSession();
}

void TtsDownloader::resetSession() {
  m_active = false;
  m_chunks.clear();
  m_audioFiles.clear();
  m_ttsLang.clear();
  m_voiceGender.clear();
  m_currentIndex = -1;
}

bool TtsDownloader::retryCurrentChunkWithFallback() {
  if (!m_active || m_currentIndex < 0 || m_currentIndex >= m_chunks.size()) {
    return false;
  }

  const QString currentChunk = m_chunks.at(m_currentIndex);
  if (currentChunk.size() <= kFallbackChunkLength) {
    return false;
  }

  QStringList fallbackChunks;
  if (!utils::splitString(currentChunk, kFallbackChunkLength, fallbackChunks) ||
      fallbackChunks.isEmpty()) {
    return false;
  }

  m_chunks.removeAt(m_currentIndex);
  for (int i = fallbackChunks.size() - 1; i >= 0; --i) {
    m_chunks.insert(m_currentIndex, fallbackChunks.at(i));
  }

  return true;
}

void TtsDownloader::downloadCurrentChunk() {
  if (!m_active || m_currentIndex < 0 || m_currentIndex >= m_chunks.size()) {
    const QStringList files = m_audioFiles;
    resetSession();
    if (files.isEmpty()) {
      emit failed("TTS service returned no audio data.");
    } else {
      emit finished(files);
    }
    return;
  }

  emit progress(QString("Preparing audio... %1/%2")
                    .arg(m_currentIndex + 1)
                    .arg(m_chunks.size()));

  QUrl url("https://www.google.com/speech-api/v1/synthesize");
  QUrlQuery params;
  params.addQueryItem("ie", "UTF-8");
  params.addQueryItem("lang", m_ttsLang);
  params.addQueryItem("gender", m_voiceGender);
  params.addQueryItem("text", m_chunks.at(m_currentIndex));
  url.setQuery(params);

  QNetworkRequest request(url);
  request.setHeader(QNetworkRequest::UserAgentHeader,
                    "Mozilla/5.0 (X11; Linux x86_64) Glate/4.0");

  QNetworkReply *reply = m_network->get(request);
  m_reply = reply;

  connect(reply, &QNetworkReply::finished, this, [this, reply]() {
    if (reply != m_reply) {
      reply->deleteLater();
      return;
    }

    m_reply = nullptr;

    if (!m_active) {
      reply->deleteLater();
      return;
    }

    const int statusCode =
        reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const bool hasError = reply->error() != QNetworkReply::NoError ||
                          statusCode < 200 || statusCode >= 300;

    if (hasError) {
      const QString err = reply->errorString().isEmpty()
                              ? QString("HTTP %1").arg(statusCode)
                              : reply->errorString();
      reply->deleteLater();

      if (retryCurrentChunkWithFallback()) {
        downloadCurrentChunk();
        return;
      }

      emit failed("Unable to download TTS audio: " + err);
      resetSession();
      return;
    }

    const QByteArray data = reply->readAll();
    reply->deleteLater();

    if (data.isEmpty()) {
      if (retryCurrentChunkWithFallback()) {
        downloadCurrentChunk();
        return;
      }

      emit failed("TTS service returned empty audio data.");
      resetSession();
      return;
    }

    const QString filePath =
        m_tempDir->filePath(QString("tts_%1.mp3").arg(m_currentIndex, 4, 10, QChar('0')));
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
      emit failed("Unable to store downloaded TTS audio chunk.");
      resetSession();
      return;
    }
    file.write(data);
    file.close();

    while (m_audioFiles.size() <= m_currentIndex) {
      m_audioFiles.append(QString());
    }
    m_audioFiles[m_currentIndex] = filePath;

    ++m_currentIndex;
    downloadCurrentChunk();
  });
}

