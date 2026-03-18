#include "share.h"
#include "ui_share.h"

#include "utils.h"

#include <QFileDialog>
#include <QGraphicsOpacityEffect>
#include <QMessageBox>

Share::Share(QWidget *parent) : QWidget(parent), ui(new Ui::Share) {
  ui->setupUi(this);
  ffmpeg = new QProcess(this);
  m_networkManager = new QNetworkAccessManager(this);
  ui->voiceGender->setCurrentIndex(
      settings.value("shareAudioVoiceGender", 0).toInt());
  connect(ui->voiceGender, &QComboBox::currentIndexChanged, this,
          [=](int index) {
            settings.setValue("shareAudioVoiceGender", index);
          });
  connect(this->ffmpeg, SIGNAL(finished(int)), this,
          SLOT(ffmpeg_finished(int)));
}

void Share::setTranslation(QString translation, QString uuid) {
  ui->translation->setPlainText(translation);
  ui->voiceGender->setCurrentIndex(
      settings.value("shareAudioVoiceGender", 0).toInt());
  translationUUID = uuid;
}

Share::~Share() { delete ui; }


QString Share::getFileNameFromString(QString string) {
  QString filename;
  if (string.trimmed().length() > 10) {
    filename = string.trimmed().left(10).trimmed();
  } else {
    filename = utils::generateRandomId(10);
  }

  return filename.remove("/").remove(".");
}

void Share::on_text_clicked() {
  QString translation = ui->translation->toPlainText();
  QString path = settings
                     .value("sharePath", QStandardPaths::writableLocation(
                                             QStandardPaths::DocumentsLocation))
                     .toString();
  QString fileName = QFileDialog::getSaveFileName(
      this, tr("Save Text File"),
      path + "/" + getFileNameFromString(translation) + ".txt",
      tr("Text Files (*.txt)"));
  if (!fileName.isEmpty()) {
    QFile file(QFileInfo(fileName).absoluteFilePath());
    if (!file.open(QIODevice::WriteOnly)) {
      showStatus(
          "<span style='color:red'>Share: </span>error failed to save file.");
      return; // aborted
    }
    // save data
    QString text = translation;
    QTextStream out(&file);
    out << text;
    // save last used location
    settings.setValue("sharePath", QFileInfo(file).path());
    file.close();
    // inform user
    showStatus(
        "<span style='color:green'>Share: </span>file saved successfully.");
  } else {
    showStatus(
        "<span style='color:red'>Share: </span>file save operation cancelled.");
  }
}

void Share::on_email_clicked() {
  showStatus("<span style='color:green'>Share: </span>opening mail client...");
  bool opened = QDesktopServices::openUrl(
      QUrl("mailto:?body=" + ui->translation->toPlainText()));
  if (!opened) {
    showStatus("<span style='color:red'>Share: </span>unable to open a mail "
               "client...");
  }
}

void Share::on_pastebin_clicked() {
  const QString text = ui->translation->toPlainText();
  if (text.trimmed().isEmpty()) {
    showStatus("<span style='color:red'>Share: </span>No text to share.");
    return;
  }

  ui->pastebin->setDisabled(true);
  showStatus("<span style='color:green'>Share: </span>Uploading paste please wait...");

  QNetworkRequest request(QUrl("https://paste.rs/"));
  request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                       QNetworkRequest::NoLessSafeRedirectPolicy);
  request.setHeader(QNetworkRequest::ContentTypeHeader,
                    "text/plain");
  request.setRawHeader("User-Agent", "glate/1.0");
  request.setTransferTimeout(15000);

  QByteArray payload = text.toUtf8();
  if (!payload.endsWith('\n'))
    payload.append('\n');

  QNetworkReply *reply = m_networkManager->post(request, payload);
  connect(reply, &QNetworkReply::finished, this, [this, reply]() {
    const int httpStatus =
        reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QString body = QString::fromUtf8(reply->readAll()).trimmed();
    const bool hasShareUrl = body.startsWith("http://") || body.startsWith("https://");

    if (httpStatus == 201 || httpStatus == 206) {
      if (hasShareUrl) {
        const QString prefix =
            httpStatus == 206
                ? "<span style='color:orange'>Share: </span>Paste uploaded partially (size limit). Link - "
                : "<span style='color:green'>Share: </span>Share link - ";
        showStatus(prefix + "<a target='_blank' href='" + body + "'>" + body + "</a>");
      } else {
        showStatus("<span style='color:red'>Share: </span>Unexpected response for HTTP " +
                   QString::number(httpStatus) + ": " + body.toHtmlEscaped());
      }
    } else {
      const QString statusText =
          httpStatus > 0 ? QString::number(httpStatus) : QString("network");
      const QString err = reply->error() == QNetworkReply::NoError
                              ? body
                              : reply->errorString() +
                                    (body.isEmpty() ? "" : " | " + body);
      showStatus("<span style='color:red'>Share: </span>Upload failed (" +
                 statusText + "): " + err.toHtmlEscaped());
    }
    ui->pastebin->setDisabled(false);
    reply->deleteLater();
  });
}


// not being used as we are not saving file now to share.
bool Share::saveFile(QString filename) {
  QString path = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
  QFile file(path + "/" + filename);
  if (!file.open(QFile::WriteOnly | QFile::Text)) {
    QMessageBox::warning(this, tr("Application"),
                         tr("Cannot write file %1:\n%2.")
                             .arg(QDir::toNativeSeparators(file.fileName()),
                                  file.errorString()));
    return false;
  }
  QTextStream out(&file);
#ifndef QT_NO_CURSOR
  QApplication::setOverrideCursor(Qt::WaitCursor);
#endif
  out << ui->translation->toPlainText();
#ifndef QT_NO_CURSOR
  QApplication::restoreOverrideCursor();
#endif
  file.close();
  return true;
}


void Share::showStatus(QString message) {
  auto eff = new QGraphicsOpacityEffect(this);
  ui->status->setGraphicsEffect(eff);
  auto a = new QPropertyAnimation(eff, "opacity");
  a->setDuration(500);
  a->setStartValue(0);
  a->setEndValue(1);
  a->setEasingCurve(QEasingCurve::InCurve);
  a->start(QPropertyAnimation::DeleteWhenStopped);
  ui->status->setText(message);
  ui->status->show();
}

void Share::on_download_clicked() {

  // create a dir with uuid to track process,
  // start donwload manager with tasks,
  // wait for tasks to finish,
  // concat downloaded results,
  // ask user to save file,
  // destroy uuid and file on close.

  QString text = ui->translation->toPlainText();
  const QString selectedGender =
      settings.value("shareAudioVoiceGender", ui->voiceGender->currentIndex())
                  .toInt() == 1
          ? "male"
          : "female";
  showStatus("<span style='color:green'>Share: </span>Preparing " +
             selectedGender + " voice download...");
  QStringList src1Parts;
  QList<QUrl> urls;
  if (utils::splitString(text, 1400, src1Parts)) {
    qDebug() << src1Parts;
    for (int i = 0; i < src1Parts.count(); i++) {
      QUrl url("https://www.google.com/speech-api/v1/synthesize");
      QUrlQuery params;
      params.addQueryItem("ie", "UTF-8");
      params.addQueryItem("lang", "hi");
      params.addQueryItem("gender", selectedGender);
      params.addQueryItem("text",
                          QUrl::toPercentEncoding(src1Parts.at(i).toUtf8()));
      url.setQuery(params);
      urls.append(url);
    }
  }

  if (td != nullptr) {
    td->disconnect();
    td->deleteLater();
    td = nullptr;
  } else {
    td = new TranslationDownloader(this, urls, translationUUID);
    ui->download->setEnabled(false);
    connect(td, &TranslationDownloader::downloadComplete, this, [=]() {
      qDebug() << "DOWNLOAD FINISHED";
      showStatus("Voice download finished...");
      showStatus("Prepearing output file...");
      concat(td->currentDownloadDir);
      td->deleteLater();
      td = nullptr;
    });
    connect(td, &TranslationDownloader::downloadStarted, this,
            [=](QString status) { showStatus(status); });
    connect(td, &TranslationDownloader::downloadError, this,
            [=](QString status) {
              showStatus(status);
              ui->download->setEnabled(true);
            });
    connect(td, &TranslationDownloader::downloadFinished, this,
            [=](QString status) { showStatus(status); });
    td->start();
  }
}

void Share::concat(QString currentDownloadDir) {
  // get path
  QString translation = ui->translation->toPlainText();
  QString path =
      settings
          .value("sharePathAudio", QStandardPaths::writableLocation(
                                       QStandardPaths::DocumentsLocation))
          .toString();
  QString fileName = QFileDialog::getSaveFileName(
      this, tr("Save Audio File"),
      path + "/" + getFileNameFromString(translation) + ".mp3",
      tr("Audio Files (*.mp3)"));
  if (!fileName.isEmpty()) {

    QStringList args;
    QString complex;
    args << "-v"
         << "quiet"
         << "-stats";
    QDir transDir(currentDownloadDir);
    cacheDirToDelete =
        currentDownloadDir; // save audio cahce dir for deletion after concat
    transDir.setFilter(QDir::Files);
    transDir.setSorting(QDir::Name);
    QFileInfoList infoList = transDir.entryInfoList();
    for (int i = 0; i < infoList.count(); i++) {
      args << "-i" << infoList.at(i).filePath();
      complex += "[" + QString::number(i) + ":0]";
    }
    complex +=
        "concat=n=" + QString::number(infoList.count()) + ":v=0:a=1[out]";
    args << "-filter_complex" << complex << "-map"
         << "[out]" << fileName << "-y";
    // ffmpeg -v quiet -stats inputs -filter_complex
    // '[0:0][1:0]concat=n=2:v=0:a=1[out]' -map '[out]' /tmp/output.mp3
    ffmpeg->start("ffmpeg", args);

    ui->download->setEnabled(!ffmpeg->waitForStarted());
    // save last used location
    showStatus("<span style='color:green'>Share: </span>concating parts...");
    settings.setValue("sharePathAudio", QFileInfo(fileName).path());
  } else {
    showStatus(
        "<span style='color:red'>Share: </span>file save operation cancelled.");
    ui->download->setEnabled(true);
  }
}

void Share::ffmpeg_finished(int k) {
  if (k == 0) {
    QString output = ffmpeg->readAll();
    showStatus("<span style='color:green'>Share: </span>export finished.\n" +
               output);
    QDir d(cacheDirToDelete);
    d.removeRecursively();
  } else {
    showStatus("<span style='color:red'>" + ffmpeg->readAllStandardError() +
               "</span><br>");
  }
  ui->download->setDisabled(false);
}
