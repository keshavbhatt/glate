#include "history.h"
#include "ui_history.h"
#include "utils.h"
#include <QDateTime>
#include <QDir>

History::History(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::History)
{
    ui->setupUi(this);
}

History::~History()
{
    delete ui;
}

void History::on_clearall_clicked()
{

}

void History::insertItem(QStringList meta,bool fromHistory){
    QWidget *histWid = new QWidget();
    if(meta.count()>=5){
        QString from, to, src1, src2, date;
        from    = meta.at(0);
        to      = meta.at(1);
        src1    = meta.at(2);
        src2    = meta.at(3);
        date    = meta.at(4);
        history_item_ui.setupUi(histWid);
        history_item_ui.time->setText(date);
        history_item_ui.from->setText(from);
        history_item_ui.to->setText(to);
        history_item_ui.src1->setText(src1.left(100)+"....");
        history_item_ui.src2->setText(src2.left(100)+"....");
        QListWidgetItem* item;
        item = new QListWidgetItem(ui->historyList);
        ui->historyList->setItemWidget(item,histWid);
        item->setSizeHint(histWid->minimumSizeHint());
        this->setMinimumWidth(histWid->width()+60);
        ui->historyList->addItem(item);
        if(!fromHistory){
            save(meta);
        }
    }
}

void History::save(QStringList meta){
    if(meta.count()>=5){
        QString from, to, src1, src2, date;
        from    = meta.at(0);
        to      = meta.at(1);
        src1    = meta.at(2);
        src2    = meta.at(3);
        date    = meta.at(4);
        QString history_file_path = utils::returnPath("history")+"/"+utils::generateRandomId(15)+".json";
        QVariantMap map;
        map.insert("date",date);
        map.insert("from", from);
        map.insert("to", to);
        map.insert("src1",src1);
        map.insert("src2", src2);
        QJsonObject object = QJsonObject::fromVariantMap(map);
        QJsonDocument document;
        document.setObject(object);
        saveJson(document,history_file_path);
    }
}


QJsonDocument History::loadJson(QString fileName) {
    QFile jsonFile(fileName);
    jsonFile.open(QFile::ReadOnly);
    QByteArray data = jsonFile.readAll();
    jsonFile.close();
    return QJsonDocument().fromJson(data);
}

void History::saveJson(QJsonDocument document, QString fileName) {
    QFile jsonFile(fileName);
    jsonFile.open(QFile::WriteOnly);
    jsonFile.write(document.toJson());
    jsonFile.close();
}

void History::loadHistory()
{
    ui->historyList->clear();
    QString history_path = utils::returnPath("history");
    QDir dir(history_path);
    QStringList filter;
    filter<< +"*.json";
    QFileInfoList files = dir.entryInfoList(filter);
    foreach (QFileInfo fileInfo, files)
    {
        //open file , read it to stringlist pass it to insertItem
        QJsonDocument doc = loadJson(fileInfo.filePath());
        QJsonObject jsonObject = doc.object();
        QString from, to, src1, src2,date;
        from = jsonObject.value("from").toString();
        to   = jsonObject.value("to").toString();
        src1 = jsonObject.value("src1").toString();
        src2 = jsonObject.value("src2").toString();
        date = jsonObject.value("date").toString();
        insertItem(QStringList()<<from<<to<<src1.left(100)<<src2.left(100)<<date,true);
    }
}
