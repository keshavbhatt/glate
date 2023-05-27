#ifndef SHARE_H
#define SHARE_H

#include "translationdownloader.h"

#include <QDateTime>
#include <QDesktopServices>
#include <QFile>
#include <QFileDialog>
#include <QGraphicsOpacityEffect>
#include <QMessageBox>
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

  QString getFileNameFromString(QString string);

  void showStatus(QString message);
  void pastebin_it_finished(int k);
  void on_pastebin_clicked();
  bool saveFile(QString filename);
  void pastebin_it_facebook_finished(int k);
  void on_download_clicked();

  void ffmpeg_finished(int k);
  void concat(QString currentDownloadDir);

private:
  Ui::Share *ui;
  QSettings settings;
  QProcess *ffmpeg = nullptr;
  QProcess *pastebin_it = nullptr;
  QProcess *pastebin_it_facebook = nullptr;
  QString translationUUID;
  TranslationDownloader *td = nullptr;
  QString cacheDirToDelete;
};

#endif // SHARE_H
