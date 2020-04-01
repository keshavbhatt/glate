#include "settings.h"
#include "ui_settings.h"
#include <QDesktopServices>

Settings::Settings(QWidget *parent, QHotkey *hotKey) :
    QWidget(parent),
    ui(new Ui::Settings)
{
    ui->setupUi(this);
    this->nativeHotkey = hotKey;
    readSettings();
    connect(this->nativeHotkey, &QHotkey::activated,
            this, &Settings::get_selected_word_fromX11);

    connect(ui->quickResultCheckBox, &QCheckBox::toggled,
            this->nativeHotkey, &QHotkey::setRegistered);

    connect(ui->keySequenceEdit, &QKeySequenceEdit::keySequenceChanged,
            this, &Settings::setShortcut);

    ui->dark->setChecked(settings.value("theme","dark").toString()=="dark");
    ui->light->setChecked(settings.value("theme").toString()=="light");

}

Settings::~Settings()
{
    delete ui;
}

void Settings::setShortcut(const QKeySequence &sequence){
    this->nativeHotkey->setShortcut(sequence,ui->quickResultCheckBox->isChecked());
    //save quick trans shortcut instanty
    settings.setValue("quicktrans_shortcut",sequence);
}

void Settings::readSettings(){

    //quick trans settings
    if(settings.value("quicktrans").isValid()){
        ui->quickResultCheckBox->setChecked(settings.value("quicktrans").toBool());
    }else if(settings.value("quicktrans").isValid()){
        ui->quickResultCheckBox->setChecked(true);
    }else{
        ui->quickResultCheckBox->setChecked(false);
    }
    //quick trans key sequence
    if(settings.value("quicktrans_shortcut").isValid()){
        QKeySequence k = QKeySequence(settings.value("quicktrans_shortcut").toString());
        ui->keySequenceEdit->setKeySequence(k);
        this->nativeHotkey->setShortcut(k.toString(),settings.value("quicktrans").toBool());
    }
    else{
        //default value if value not founds
        QKeySequence k = QKeySequence::fromString(tr("Ctrl+Shift+Space"));
        ui->keySequenceEdit->setKeySequence(k);
        this->nativeHotkey->setShortcut(k.toString(),true);
        ui->quickResultCheckBox->setChecked(true);
    }
}

void Settings::get_selected_word_fromX11(){
    qDebug()<<"Called";
    QProcess *xsel = new QProcess(this);
    xsel->setObjectName("xclip");
    xsel->start("xclip",QStringList()<<"-o"<<"-sel");
    connect(xsel,SIGNAL(finished(int)),this,SLOT(set_x11_selection()));
}

void Settings::set_x11_selection(){
    QObject *xselection = this->findChild<QObject*>("xclip");
    x11_selected = ((QProcess*)(xselection))->readAllStandardOutput();
    if(!x11_selected.trimmed().isEmpty())
      show_requested_text();
    ((QProcess*)(xselection))->deleteLater();
    xselection->deleteLater();
}

void Settings::show_requested_text(){
    emit translationRequest(x11_selected);
}

bool Settings::quickResultCheckBoxChecked(){
    return ui->quickResultCheckBox->isChecked();
}

QKeySequence Settings::keySequenceEditKeySequence(){
    return ui->keySequenceEdit->keySequence();
}


void Settings::on_github_clicked()
{
    QDesktopServices::openUrl(QUrl("https://github.com/keshavbhatt/glate"));
}

void Settings::on_rate_clicked()
{
    QDesktopServices::openUrl(QUrl("snap://glate"));
}

void Settings::on_donate_clicked()
{
    QDesktopServices::openUrl(QUrl("https://paypal.me/keshavnrj/5"));
}

void Settings::on_dark_toggled(bool checked)
{
    settings.setValue("theme",checked ? "dark": "light");
    emit themeToggled();
}

void Settings::on_light_toggled(bool checked)
{
    settings.setValue("theme",checked ? "light": "dark");
    emit themeToggled();
}
