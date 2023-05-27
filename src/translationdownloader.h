#ifndef TRANSLATIONDOWNLOADER_H
#define TRANSLATIONDOWNLOADER_H

#include <QObject>
#include <QtNetwork>

class TranslationDownloader : public QObject {
  Q_OBJECT
public:
  explicit TranslationDownloader(QObject *parent = nullptr,
                                 QList<QUrl> urls = {},
                                 QString translationUUID = "");
  QString currentDownloadDir;
signals:
  void downloadStarted(QString status);
  void downloadFinished(QString status);
  void downloadError(QString status);
  void downloadComplete();

public slots:
  void start();
private slots:
  void startDownload(QUrl url);

private:
  QString _cache_path;
  int total = 0;
  int current = 0;
  QList<QUrl> urls;
};

#endif // TRANSLATIONDOWNLOADER_H
