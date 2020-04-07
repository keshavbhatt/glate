#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFile>
#include <QDebug>
#include <QPushButton>
#include <QSettings>
#include <QMediaPlayer>
#include <QClipboard>
#include <QHotkey>
#include <QTextEdit>

#include "share.h"
#include "waitingspinnerwidget.h"
#include "request.h"
#include "error.h"
#include "settings.h"
#include "history.h"
#include "ui_textoptionform.h"
#include "controlbutton.h"
#include "utils.h"
#include "linebyline.h"
#include "slider.h"


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected slots:
    void resizeEvent(QResizeEvent *event);
    void closeEvent(QCloseEvent *event);
    bool eventFilter(QObject *o, QEvent *e);
private slots:
    void setStyle(QString fname);
    void readLangCode();
    void resizeFix();
    void translate_clicked();
    void showError(QString message);
    void on_clear_clicked();

    void on_paste_clicked();

    void on_switch_lang_clicked();

    void on_lang1_currentIndexChanged(int index);

    void on_lang2_currentIndexChanged(int index);

    QString getTransLang();

    QString getSourceLang();

    void processTranslation(QString reply);

    void on_play2_clicked();

    void on_play1_clicked();

    void on_src1_textChanged();

    void on_copy_clicked();

    void on_src2_textChanged();

    void on_share_clicked();

    void init_settings();

    void on_settings_clicked();

    void init_textOptionWidget();

    void textSelectionChanged(QTextEdit *editor);
    void on_history_clicked();

    void init_history();
    void on_lineByline_clicked();

    QString getLangName(QString langCode);
    QString getLangCode(QString langName);
    void saveByTransId(QString translationId, QString reply);
    void on_help_clicked();

private:
    Ui::MainWindow *ui;
    Ui::textOptionForm textOptionForm;
    QStringList _langName,_langCode,_supportedTts;
    controlButton *_translate = nullptr;
    WaitingSpinnerWidget *_loader = nullptr;
    QSettings settings;
    Request *_request = nullptr;
    Share *_share = nullptr;
    LineByLine *_lineByLine = nullptr;

    Error * _error = nullptr;
    QList<QStringList> currentTranslationData;
    QMediaPlayer *_player = nullptr;
    QClipboard *clipboard = nullptr;

    Settings *settingsWidget = nullptr;
    History *historyWidget = nullptr;

    QHotkey *nativeHotkey = nullptr;

    QWidget *textOptionWidget = nullptr;
    QString selectedText;
    bool playSelected = false;
    QString translationId;

    Slider * slider = nullptr;


};

#endif // MAINWINDOW_H
