#include "share.h"
#include "ui_share.h"

#include "utils.h"

#include <QFileDialog>
#include <QGraphicsOpacityEffect>
#include <QMessageBox>
#include <QPropertyAnimation>

Share::Share(QWidget *parent) : QWidget(parent), ui(new Ui::Share) {
  ui->setupUi(this);
  ffmpeg = new QProcess(this);
  m_networkManager = new QNetworkAccessManager(this);
  m_ttsDownloader = new TtsDownloader(this);

  // Populate voice combo from central voice list
  ui->voiceGender->clear();
  for (const VoiceOption &v : utils::availableVoices())
    ui->voiceGender->addItem(v.displayName);

  ui->voiceGender->setCurrentIndex(
      settings.value("shareAudioVoiceGender", 0).toInt());
  connect(ui->voiceGender, &QComboBox::currentIndexChanged, this,
          [=](int index) {
            settings.setValue("shareAudioVoiceGender", index);
          });
  connect(this->ffmpeg, SIGNAL(finished(int)), this,
          SLOT(ffmpeg_finished(int)));

  connect(m_ttsDownloader, &TtsDownloader::progress, this,
          [=](const QString &status) {
            showStatus("<span style='color:green'>Share: </span>" +
                       status.toHtmlEscaped());
          });
  connect(m_ttsDownloader, &TtsDownloader::finished, this,
          [=](const QStringList &audioFiles) {
            m_downloadedAudioFiles = audioFiles;
            if (m_downloadedAudioFiles.isEmpty()) {
              showStatus("<span style='color:red'>Share: </span>No audio chunks downloaded.");
              ui->download->setEnabled(true);
              return;
            }
            showStatus("<span style='color:green'>Share: </span>Voice download finished...");
            concat(m_downloadedAudioFiles);
          });
  connect(m_ttsDownloader, &TtsDownloader::failed, this,
          [=](const QString &error) {
            showStatus("<span style='color:red'>Share: </span>" +
                       error.toHtmlEscaped());
            ui->download->setEnabled(true);
          });
}

void Share::setTranslation(const QString &translation, const QString &uuid, const QString &langCode) {
  ui->translation->setPlainText(translation);
  ui->voiceGender->setCurrentIndex(
      settings.value("shareAudioVoiceGender", 0).toInt());
  translationUUID = uuid;
  translationLangCode = langCode;
}

Share::~Share() { delete ui; }


QString Share::getFileNameFromString(const QString &string) {
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
bool Share::saveFile(const QString &filename) {
  QString path = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
  QFile file(path + QDir::separator() + filename);
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


void Share::showStatus(const QString &message) {
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
  const int voiceIdx = ui->voiceGender->currentIndex();
  const auto voices = utils::availableVoices();
  const VoiceOption &voiceConf =
      voiceIdx < voices.size() ? voices.at(voiceIdx) : voices.first();
  const QString selectedGender = voiceConf.gender;
  const QString ttsLang =
      voiceConf.langOverride.isEmpty()
          ? (translationLangCode.isEmpty() ? QString("en") : translationLangCode)
          : voiceConf.langOverride;
  showStatus("<span style='color:green'>Share: </span>Preparing " +
             voiceConf.displayName + " voice download...");
  ui->download->setEnabled(false);
  m_downloadedAudioFiles.clear();
  m_ttsDownloader->start(text, ttsLang, selectedGender);
}

void Share::concat(const QStringList &audioFiles) {
  // get path
  QString translation = ui->translation->toPlainText();
  QString path =
      settings
          .value("sharePathAudio", QStandardPaths::writableLocation(
                                       QStandardPaths::DocumentsLocation))
          .toString();
  QString fileName = QFileDialog::getSaveFileName(
      this, tr("Save Audio File"),
      path + QDir::separator() + getFileNameFromString(translation) + ".mp3",
      tr("Audio Files (*.mp3)"));
  if (!fileName.isEmpty()) {

    QStringList args;
    QString complex;
    args << "-v"
         << "quiet"
         << "-stats";
    for (int i = 0; i < audioFiles.count(); i++) {
      args << "-i" << audioFiles.at(i);
      complex += "[" + QString::number(i) + ":0]";
    }
    complex +=
        "concat=n=" + QString::number(audioFiles.count()) + ":v=0:a=1[out]";
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
    const QString output = QString::fromUtf8(ffmpeg->readAll()).trimmed();
    const QString message = output.isEmpty()
                                ? "<span style='color:green'>Share: </span>Export finished."
                                : "<span style='color:green'>Share: </span>Export finished.<br>" +
                                      output.toHtmlEscaped();
    showStatus(message);
  } else {
    const QString err =
        QString::fromUtf8(ffmpeg->readAllStandardError()).trimmed();
    showStatus(err.isEmpty()
                   ? "<span style='color:red'>Share: </span>Export failed."
                   : "<span style='color:red'>Share: </span>" +
                         err.toHtmlEscaped());
  }
  ui->download->setDisabled(false);
}
