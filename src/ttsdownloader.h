#ifndef TTSDOWNLOADER_H
#define TTSDOWNLOADER_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QTemporaryDir>

class TtsDownloader : public QObject {
  Q_OBJECT

public:
  explicit TtsDownloader(QObject *parent = nullptr);
  ~TtsDownloader() override;

  void start(const QString &text, const QString &ttsLang,
             const QString &voiceGender);
  void cancel();

signals:
  void progress(const QString &message);
  void finished(const QStringList &audioFiles);
  void failed(const QString &message);

private:
  bool retryCurrentChunkWithFallback();
  void downloadCurrentChunk();
  void clearTempStorage();
  void resetSession();

  QNetworkAccessManager *m_network = nullptr;
  QNetworkReply *m_reply = nullptr;
  QTemporaryDir *m_tempDir = nullptr;
  QStringList m_chunks;
  QStringList m_audioFiles;
  QString m_ttsLang;
  QString m_voiceGender;
  int m_currentIndex = -1;
  bool m_active = false;

  static constexpr int kMaxChunkLength = 1400;
  static constexpr int kFallbackChunkLength = 700;
};

#endif // TTSDOWNLOADER_H

