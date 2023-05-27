#include "history.h"
#include "ui_history.h"
#include "utils.h"
#include <QDateTime>
#include <QDir>

History::History(QWidget *parent) : QWidget(parent), ui(new Ui::History) {
  ui->setupUi(this);
  ui->historyList->setSelectionRectVisible(true);
  ui->historyList->setAlternatingRowColors(true);
  ui->historyList->setDragEnabled(false);
  ui->historyList->setDropIndicatorShown(false);
  ui->historyList->setDragDropMode(QAbstractItemView::NoDragDrop);
}

History::~History() { delete ui; }

void History::on_clearall_clicked() {
  QDir dir(utils::returnPath("history"));
  if (dir.removeRecursively()) {
    ui->historyList->clear();
    ui->clearall->setEnabled(false);
  }
}

void History::insertItem(QStringList meta, bool fromHistory,
                         QString translationId) {
  QWidget *histWid = new QWidget();
  if (meta.count() >= 5) {
    QString from, to, src1, src2, date;
    from = meta.at(0);
    to = meta.at(1);
    src1 = meta.at(2);
    src2 = meta.at(3);
    date = meta.at(4);
    history_item_ui.setupUi(histWid);
    histWid->setObjectName("widget_" + translationId);
    histWid->setStyleSheet("QWidget#widget_" + translationId +
                           "{background: transparent;}");
    history_item_ui.remove->setObjectName("remove_" + translationId);
    history_item_ui.load->setObjectName("load_" + translationId);
    connect(history_item_ui.remove, &QPushButton::clicked, [=]() {
      QFile file(utils::returnPath("history") + "/" + translationId + ".json");
      if (file.remove()) {
        loadHistory();
      }
    });

    connect(history_item_ui.load, &QPushButton::clicked, [=]() {
      emit setTranslationId(translationId);
      loadHistoryItem(utils::returnPath("history") + "/" + translationId +
                      ".json");
    });
    history_item_ui.time->setText(date);
    history_item_ui.from->setText(from);
    history_item_ui.to->setText(to);
    history_item_ui.src1->setText(src1.left(100) + "....");
    history_item_ui.src2->setText(src2.left(100) + "....");
    QListWidgetItem *item;
    item = new QListWidgetItem(ui->historyList);
    ui->historyList->setItemWidget(item, histWid);
    item->setSizeHint(histWid->minimumSizeHint());
    this->setMinimumWidth(histWid->width() + 60);
    ui->historyList->addItem(item);
    ui->clearall->setEnabled(true);
    if (!fromHistory) {
      save(meta, translationId);
    }
  }
}

void History::loadHistoryItem(QString itemPath) {
  QFile file(itemPath);
  if (file.exists()) {
    // open file , read it to stringlist pass it to insertItem
    QJsonDocument doc = loadJson(itemPath);
    QJsonObject jsonObject = doc.object();
    QString from, to, src1, src2, date;
    from = jsonObject.value("from").toString();
    to = jsonObject.value("to").toString();
    src1 = jsonObject.value("src1").toString();
    src2 = jsonObject.value("src2").toString();
    date = jsonObject.value("date").toString();
    emit historyItemMeta(QStringList() << from << to << src1 << src2);
  }
  file.deleteLater();
}

void History::save(QStringList meta, QString translationId) {
  if (meta.count() >= 5) {
    QString from, to, src1, src2, date;
    from = meta.at(0);
    to = meta.at(1);
    src1 = meta.at(2);
    src2 = meta.at(3);
    date = meta.at(4);
    QString history_file_path =
        utils::returnPath("history") + "/" + translationId + ".json";
    QVariantMap map;
    map.insert("date", date);
    map.insert("from", from);
    map.insert("to", to);
    map.insert("src1", src1);
    map.insert("src2", src2);
    QJsonObject object = QJsonObject::fromVariantMap(map);
    QJsonDocument document;
    document.setObject(object);
    saveJson(document, history_file_path);
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

void History::loadHistory() {
  ui->historyList->clear();
  QString history_path = utils::returnPath("history");
  QDir dir(history_path);
  dir.setSorting(QDir::Time);
  QStringList filter;
  filter << +"*.json";
  QFileInfoList files = dir.entryInfoList(filter);
  ui->clearall->setEnabled(files.isEmpty() == false);
  foreach (QFileInfo fileInfo, files) {
    // open file , read it to stringlist pass it to insertItem
    QJsonDocument doc = loadJson(fileInfo.filePath());
    QJsonObject jsonObject = doc.object();
    QString from, to, src1, src2, date;
    from = jsonObject.value("from").toString();
    to = jsonObject.value("to").toString();
    src1 = jsonObject.value("src1").toString();
    src2 = jsonObject.value("src2").toString();
    date = jsonObject.value("date").toString();
    insertItem(QStringList()
                   << from << to << src1.left(100) << src2.left(100) << date,
               true, fileInfo.baseName());
  }
}
