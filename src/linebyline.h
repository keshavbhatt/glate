#ifndef LINEBYLINE_H
#define LINEBYLINE_H

#include <QClipboard>
#include <QWidget>

namespace Ui {
class LineByLine;
}

class LineByLine : public QWidget {
  Q_OBJECT

public:
  explicit LineByLine(QWidget *parent = nullptr,
                      QClipboard *clipborad = nullptr);
  ~LineByLine();

public slots:
  void setData(QList<QStringList> data, QString sLang, QString tLang,
               QString tId);
  void clearData();
  QString getTId();
private slots:
  void on_src1_currentRowChanged(int currentRow);

  void on_src2_currentRowChanged(int currentRow);

  void on_copySrc2_clicked();

  void on_copySrc1_clicked();

private:
  Ui::LineByLine *ui;
  QString translationId;
  QClipboard *clipborad = nullptr;
};

#endif // LINEBYLINE_H
