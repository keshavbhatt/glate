#ifndef HISTORY_H
#define HISTORY_H

#include <QWidget>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "ui_history_item.h"


namespace Ui {
class History;
}

class History : public QWidget
{
    Q_OBJECT

public:
    explicit History(QWidget *parent = nullptr);
    ~History();

public slots:
    void loadHistory();
    void save(QStringList meta);
    void insertItem(QStringList meta, bool fromHistory);
private slots:
    void on_clearall_clicked();

    QJsonDocument loadJson(QString fileName);
    void saveJson(QJsonDocument document, QString fileName);
private:
    Ui::History *ui;
    Ui::history_item history_item_ui;
};

#endif // HISTORY_H
