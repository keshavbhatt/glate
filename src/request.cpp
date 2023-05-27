#include "request.h"

Request::Request(QObject *parent) : QObject(parent) {
  _cache_path = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
}

Request::~Request() {}

void Request::get(const QUrl &url) {
  QNetworkAccessManager *m_netwManager = new QNetworkAccessManager(this);
  QNetworkDiskCache *diskCache = new QNetworkDiskCache(this);
  diskCache->setCacheDirectory(_cache_path);
  m_netwManager->setCache(diskCache);
  connect(m_netwManager, &QNetworkAccessManager::finished, this,
          [=](QNetworkReply *rep) {
            if (rep->error() == QNetworkReply::NoError) {
              QString repStr = rep->readAll();
              emit requestFinished(repStr);
            } else {
              emit downloadError(rep->errorString());
            }
            rep->deleteLater();
            m_netwManager->deleteLater();
          });

  QNetworkRequest request(url);
  m_netwManager->get(request);
  emit requestStarted();
}

void Request::downloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
  double progress = (double)bytesReceived / bytesTotal * 100.0;
  int intProgress = static_cast<int>(progress);
  emit _downloadProgress(intProgress);
}
