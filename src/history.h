#ifndef HISTORY_H
#define HISTORY_H

#include "ui_history_item.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QWidget>

namespace Ui {
class History;
}

class History : public QWidget {
  Q_OBJECT

public:
  explicit History(QWidget *parent = nullptr);
  ~History();

signals:
  void historyItemMeta(QStringList historyItemMeta);
  void setTranslationId(QString transId);
public slots:
  void loadHistory();
  void save(QStringList meta, QString translationId);
  void insertItem(QStringList meta, bool fromHistory, QString translationId);
  void loadHistoryItem(QString itemPath);

private slots:
  void on_clearall_clicked();
  QJsonDocument loadJson(QString fileName);
  void saveJson(QJsonDocument document, QString fileName);

private:
  Ui::History *ui;
  Ui::history_item history_item_ui;
};

#endif // HISTORY_H
