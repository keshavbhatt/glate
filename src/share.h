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
  void setTranslation(QString translation, QString uuid);
private slots:
  void on_text_clicked();
  void on_email_clicked();
  void on_pastebin_clicked();
  void on_download_clicked();
  void ffmpeg_finished(int k);

  QString getFileNameFromString(QString string);
  void showStatus(QString message);
  bool saveFile(QString filename);
  void concat(QString currentDownloadDir);

private:
  Ui::Share *ui;
  QSettings settings;
  QProcess *ffmpeg = nullptr;
  QNetworkAccessManager *m_networkManager = nullptr;
  QString translationUUID;
  TranslationDownloader *td = nullptr;
  QString cacheDirToDelete;
};

#endif // SHARE_H
