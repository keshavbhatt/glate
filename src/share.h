#ifndef SHARE_H
#define SHARE_H

#include "translationdownloader.h"

#include <QDesktopServices>
#include <QNetworkAccessManager>
#include <QProcess>
#include <QSettings>
#include <QWidget>

namespace Ui {
class Share;
}

class Share : public QWidget {
  Q_OBJECT

public:
  explicit Share(QWidget *parent = nullptr);
  ~Share();

public slots:
  void setTranslation(const QString &translation, const QString &uuid,
                      const QString &langCode = QString());
private slots:
  void on_text_clicked();
  void on_email_clicked();
  void on_pastebin_clicked();
  void on_download_clicked();
  void ffmpeg_finished(int k);

  static QString getFileNameFromString(const QString &string);
  void showStatus(const QString &message);
  bool saveFile(const QString &filename);
  void concat(const QString &currentDownloadDir);

private:
  Ui::Share *ui;
  QSettings settings;
  QProcess *ffmpeg = nullptr;
  QNetworkAccessManager *m_networkManager = nullptr;
  QString translationUUID;
  QString translationLangCode;
  TranslationDownloader *td = nullptr;
  QString cacheDirToDelete;
};

#endif // SHARE_H
