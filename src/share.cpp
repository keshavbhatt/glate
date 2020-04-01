#include "share.h"
#include "ui_share.h"
#include <QDesktopServices>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QDateTime>
#include <QTimer>

#include "utils.h"

Share::Share(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Share)
{
    ui->setupUi(this);
    pastebin_it=new QProcess(this);
    pastebin_it_facebook=new QProcess(this);
    connect(this->pastebin_it,SIGNAL(finished(int)),this,SLOT(pastebin_it_finished(int)));
    connect(this->pastebin_it_facebook,SIGNAL(finished(int)),this,SLOT(pastebin_it_facebook_finished(int)));

}

void Share::setTranslation(QString translation){
    ui->translation->setPlainText(translation);
}

Share::~Share()
{
    delete ui;
}

void Share::on_twitter_clicked()
{
    showStatus("<span style='color:green'>Share: </span>opening web-browser...");
    bool opened = QDesktopServices::openUrl(QUrl(QUrl("https://twitter.com/intent/tweet?text="+ui->translation->toPlainText())).toString(QUrl::FullyEncoded));
    if(!opened){
        showStatus("<span style='color:red'>Share: </span>unable to open a web-browser...");
    }
}

void Share::on_facebook_clicked()
{
    QString encoded = QUrl(ui->translation->toPlainText()).toString(QUrl::FullyEncoded);
    QString o = "wget -O - --post-data=\"""sprunge="+encoded+"\" http://sprunge.us";
    pastebin_it_facebook->start("bash", QStringList()<<"-c"<< o);
    ui->facebook->setDisabled(true);
    if(pastebin_it_facebook->waitForStarted(1000)){
        showStatus("<span style='color:green'>Share: </span>Prepearing facebook share please wait...");
    }else{
        showStatus("<span style='color:red'>Share: </span>Failure.<br>");
    }
}

QString Share::getFileNameFromString(QString string){
    QString filename ;
    if(string.trimmed().length()>10){
        filename = string.trimmed().left(10).trimmed();
    }else{
        filename = utils::generateRandomId(10);
    }

    return filename.remove("/").remove(".");
}

void Share::on_text_clicked()
{
    QString translation = ui->translation->toPlainText();
    QString path = settings.value("sharePath",QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Text File"), path+"/"+getFileNameFromString(translation)+".txt", tr("Text Files (*.txt)"));
    if (!fileName.isEmpty())
    {
        QFile file(QFileInfo(fileName).absoluteFilePath());
        if (!file.open(QIODevice::WriteOnly))
        {
            showStatus("<span style='color:red'>Share: </span>error failed to save file.");
            return; //aborted
        }
        //save data
        QString text = translation;
        QTextStream out(&file);
        out << text;
        //save last used location
        settings.setValue("sharePath",QFileInfo(file).path());
        file.close();
        //inform user
        showStatus("<span style='color:green'>Share: </span>file saved successfully.");
    }else {
        showStatus("<span style='color:red'>Share: </span>file save operation cancelled.");
    }
}

void Share::on_email_clicked()
{
    showStatus("<span style='color:green'>Share: </span>opening mail client...");
    bool opened = QDesktopServices::openUrl(QUrl("mailto:?body="+ui->translation->toPlainText()));
    if(!opened){
        showStatus("<span style='color:red'>Share: </span>unable to open a mail client...");
    }
}

void Share::on_pastebin_clicked()
{
    QString encoded = QUrl(ui->translation->toPlainText()).toString(QUrl::FullyEncoded);
    QString o = "wget -O - --post-data=\"""sprunge="+encoded+"\" http://sprunge.us";
    pastebin_it->start("bash", QStringList()<<"-c"<< o);
    showStatus("<span style='color:red'>Share: </span>Prepearing facebook share please wait...");
    ui->pastebin->setDisabled(true);
    if(pastebin_it->waitForStarted(1000)){
        showStatus("<span style='color:green'>Share: </span>Uploading paste please wait...<br>");
    }else{
        showStatus("<span style='color:red'>Share: </span>Failure.<br>");
    }
}

//not being used as we are not saving file now to share.
bool Share::saveFile(QString filename)
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QFile file(path+"/"+filename);
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

void Share::pastebin_it_finished(int k)
{
    if(k==0){
        QString url = pastebin_it->readAll().trimmed().simplified();
        showStatus("<span style='color:green'>Share: </span>Pastebin link - <a target='_blank' href='"+url+"'>"+url+"</a>");
        ui->pastebin->setDisabled(false);
    }
    else if(k==1){
         showStatus("<span style='color:red'>"+pastebin_it->readAllStandardError()+"</span><br>");
         ui->pastebin->setDisabled(false);
    }
}

void Share::pastebin_it_facebook_finished(int k)
{
    if(k==0){
        QString url = pastebin_it_facebook->readAll();
        showStatus("<span style='color:green'>Share: </span>opening web-browser with url - https://www.facebook.com/sharer/sharer.php?u="+url);
        ui->facebook->setDisabled(false);
        bool opened = QDesktopServices::openUrl(QUrl("https://www.facebook.com/sharer/sharer.php?u="+url));
        if(!opened){
            showStatus("<span style='color:red'>Share: </span>unable to open a web-browser...");
        }
    }
    else if(k==1){
         showStatus("<span style='color:red'>"+pastebin_it_facebook->readAllStandardError()+"</span><br>");
         ui->facebook->setDisabled(false);
    }
}

void Share::showStatus(QString message)
{
    QGraphicsOpacityEffect *eff = new QGraphicsOpacityEffect(this);
    ui->status->setGraphicsEffect(eff);
    QPropertyAnimation *a = new QPropertyAnimation(eff,"opacity");
    a->setDuration(500);
    a->setStartValue(0);
    a->setEndValue(1);
    a->setEasingCurve(QEasingCurve::InCurve);
    a->start(QPropertyAnimation::DeleteWhenStopped);
    ui->status->setText(message);
    ui->status->show();
}

