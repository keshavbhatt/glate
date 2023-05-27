#include "linebyline.h"
#include "ui_linebyline.h"
#include <QScrollBar>

LineByLine::LineByLine(QWidget *parent, QClipboard *clipborad)
    : QWidget(parent), ui(new Ui::LineByLine) {
  ui->setupUi(this);
  this->clipborad = clipborad;
  connect(ui->src1->verticalScrollBar(), &QScrollBar::valueChanged,
          [=](int val) { ui->src2->verticalScrollBar()->setValue(val); });
  connect(ui->src2->verticalScrollBar(), &QScrollBar::valueChanged,
          [=](int val) { ui->src1->verticalScrollBar()->setValue(val); });
}

LineByLine::~LineByLine() { delete ui; }

void LineByLine::setData(QList<QStringList> currentTranslationData,
                         QString sLang, QString tLang, QString tId) {
  translationId = tId;
  ui->sLang->setText(sLang);
  ui->tLang->setText(tLang);
  foreach (QStringList translationFregments, currentTranslationData) {
    ui->src2->addItem(translationFregments.at(0)); // translation
    ui->src1->addItem(translationFregments.at(1)); // source
  }
  if (ui->src1->count() > 0)
    ui->src1->setCurrentRow(0);
  if (ui->src2->count() > 0)
    ui->src2->setCurrentRow(0);
}

void LineByLine::clearData() {
  ui->src1->blockSignals(true);
  ui->src2->blockSignals(true);

  ui->src1->clear();
  ui->src2->clear();

  ui->src1->blockSignals(false);
  ui->src2->blockSignals(false);
}

void LineByLine::on_src1_currentRowChanged(int currentRow) {
  ui->src2->blockSignals(true);
  ui->src2->setCurrentRow(currentRow);
  ui->src2Browser->setText(ui->src2->item(currentRow)->text());
  ui->src1Browser->setText(ui->src1->item(currentRow)->text());
  ui->src2->blockSignals(false);
}

void LineByLine::on_src2_currentRowChanged(int currentRow) {
  ui->src1->blockSignals(true);
  ui->src1->setCurrentRow(currentRow);
  ui->src1Browser->setText(ui->src1->item(currentRow)->text());
  ui->src2Browser->setText(ui->src2->item(currentRow)->text());
  ui->src1->blockSignals(false);
}

QString LineByLine::getTId() { return translationId; }

void LineByLine::on_copySrc2_clicked() {
  clipborad->setText(ui->src2Browser->toPlainText());
}

void LineByLine::on_copySrc1_clicked() {
  clipborad->setText(ui->src1Browser->toPlainText());
}
