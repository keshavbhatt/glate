#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QKeyEvent>
#include <QMainWindow>
#include <QAudioOutput>
#include <QMediaPlayer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QScrollBar>
#include <QTemporaryDir>
#include <QTextEdit>

#include "controlbutton.h"
#include "error.h"
#include "history.h"
#include "linebyline.h"
#include "request.h"
#include "settings.h"
#include "share.h"
#include "slider.h"
#include "systemtraymanager.h"
#include "ui_textoptionform.h"
#include "waitingspinnerwidget.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

protected slots:
  void changeEvent(QEvent *e) override;
  void resizeEvent(QResizeEvent *event) override;
  void closeEvent(QCloseEvent *event) override;
  bool eventFilter(QObject *o, QEvent *e) override;
private slots:

  void on_clear_clicked();
  void on_paste_clicked();
  void on_switch_lang_clicked();
  void on_lang1_currentIndexChanged(int index);
  void on_lang2_currentIndexChanged(int index);
  void on_play2_clicked();
  void on_play1_clicked();
  void on_src1_textChanged();
  void on_copy_clicked();
  void on_src2_textChanged();
  void on_share_clicked();
  void on_settings_clicked();
  void on_history_clicked();
  void on_lineByline_clicked();
  void on_help_clicked();

  void readLangCode();
  void resizeFix();
  void translate_clicked();
  void init_history();
  QString getTransLang();
  QString getSourceLang();
  void init_settings();
  static void setStyle(const QString &fname);
  void showError(const QString &message);
  void textSelectionChanged(QTextEdit *editor);
  QString getLangName(const QString &langCode);
  QString getLangCode(const QString &langName);
  static void saveByTransId(const QString &translationId, const QString &reply);
  void processTranslation(const QString &reply);
  void startTtsSession(const QString &playerName, const QString &text,
                       const QString &ttsLang, const QString &voiceGender);
  void downloadCurrentTtsChunk();
  void playCurrentTtsChunk();
  void stopTtsSession(bool stopPlayer);
  void resetPlayIcons();
  void setPlayButtonState(const QString &buttonName, const QIcon &icon,
                          const QString &toolTip);
  QString activePlayButtonName() const;
  bool retryCurrentTtsChunkWithFallback();
  bool resolveTtsVoiceConfig(const QString &baseLang, QString &ttsLang,
                             QString &ttsCheckLang,
                             QString &voiceGender) const;
  void toggleOrStartTts(const QString &buttonName, const QString &baseLang,
                        const QString &displayLang, const QString &text);

private:
  Ui::MainWindow *ui;
  Ui::textOptionForm textOptionForm;

  controlButton *m_translate = nullptr;
  WaitingSpinnerWidget *m_loader = nullptr;
  Error *m_error = nullptr;
  LineByLine *m_lineByLine = nullptr;
  Share *m_share = nullptr;
  Request *m_request = nullptr;
  QMediaPlayer *m_player = nullptr;
  QClipboard *m_clipboard = nullptr;
  Settings *m_settingsWidget = nullptr;
  History *m_historyWidget = nullptr;
  QHotkey *m_nativeHotkey = nullptr;
  QWidget *m_textOptionWidget = nullptr;
  Slider *m_slider = nullptr;
  SystemTrayManager *m_trayManager = nullptr;

  QString m_selectedText;
  bool m_playSelected = false;
  bool m_forceQuit = false;
  QString m_translationId;
  QStringList m_langName, m_langCode, m_supportedTts;
  QSettings m_settings;
  QList<QStringList> m_currentTranslationData;

  QNetworkAccessManager *m_ttsNetwork = nullptr;
  QNetworkReply *m_ttsReply = nullptr;
  QTemporaryDir *m_ttsTempDir = nullptr;
  QStringList m_ttsChunks;
  QStringList m_ttsLocalFiles;
  QString m_ttsLang;
  QString m_ttsGender;
  int m_ttsCurrentIndex = -1;
  bool m_ttsSessionActive = false;
};

#endif // MAINWINDOW_H
