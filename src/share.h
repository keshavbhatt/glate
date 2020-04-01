#ifndef SHARE_H
#define SHARE_H

#include <QWidget>
#include <QSettings>
#include <QProcess>

namespace Ui {
class Share;
}

class Share : public QWidget
{
    Q_OBJECT

public:
    explicit Share(QWidget *parent = nullptr);
    ~Share();

public slots:
    void setTranslation(QString translation);
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
private:
    Ui::Share *ui;
    QSettings settings;
    QProcess * pastebin_it = nullptr;
    QProcess * pastebin_it_facebook = nullptr;
};

#endif // SHARE_H
