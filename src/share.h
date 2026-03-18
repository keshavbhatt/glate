#ifndef SHARE_H
#define SHARE_H

#include "translationdownloader.h"

#include <QDateTime>
#include <QDesktopServices>
#include <QFile>
#include <QFileDialog>
#include <QGraphicsOpacityEffect>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProcess>
#include <QPropertyAnimation>
#include <QSettings>
#include <QStandardPaths>
#include <QTextStream>
#include <QTimer>
#include <QUrlQuery>
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
  void on_twitter_clicked();
  void on_facebook_clicked();
  void on_text_clicked();
  void on_email_clicked();
  void on_pastebin_clicked();
  void on_download_clicked();
  void pastebin_network_finished(QNetworkReply *reply);
  void ffmpeg_finished(int k);

  QString getFileNameFromString(QString string);
  void showStatus(QString message);
  bool saveFile(QString filename);
  void concat(QString currentDownloadDir);

private:
  Ui::Share *ui;
  QSettings settings;
  QProcess *ffmpeg = nullptr;
  QNetworkAccessManager *networkManager = nullptr;
  QString translationUUID;
  TranslationDownloader *td = nullptr;
  QString cacheDirToDelete;
};

#endif // SHARE_H
