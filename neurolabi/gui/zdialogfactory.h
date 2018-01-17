#ifndef ZDIALOGFACTORY_H
#define ZDIALOGFACTORY_H

#include <QObject>
#include <QDialog>
#include <QVector>
#include <QPointer>
#include "dialogs/zdviddialog.h"
#include "dialogs/zdvidtargetproviderdialog.h"
#include "dialogs/dvidimagedialog.h"
#include "dialogs/zspinboxdialog.h"
#include "zparameter.h"
#include "dialogs/zspinboxgroupdialog.h"

class QSpacerItem;
class ZParameterArray;

#ifdef _WIN32
#undef GetOpenFileName
#undef GetSaveFileName
#endif

class ZDialogFactory
{
public:
  ZDialogFactory(QWidget *parentWidget);
  ~ZDialogFactory();

  enum ZDvidDialogType {
      ORIGINAL,
      BRANCH_BROWSER
  };

  static ZDvidTargetProviderDialog* makeDvidDialog(QWidget *parent = 0, ZDvidDialogType type = BRANCH_BROWSER);
  static QDialog* makeTestDialog(QWidget *parent = 0);
  static QDialog* makeStackDialog(QWidget *parent = 0);
  static DvidImageDialog *makeDvidImageDialog(
      ZDvidTargetProviderDialog *dvidDlg, QWidget *parent = 0);
  static ZSpinBoxDialog *makeSpinBoxDialog(QWidget *parent = 0);
  static ZSpinBoxGroupDialog *makeDownsampleDialog(QWidget *parent);
  static QDialog* makeParameterDialog(
      const ZParameterArray &parameterArray,
      QWidget *parent);
  static bool Ask(const QString &title, const QString &msg, QWidget *parent);
  static void Warn(const QString &title, const QString &msg, QWidget *parent);

  static QString GetDirectory(
      const QString &caption, const QString &filePath, QWidget *parent);
  static QString GetOpenFileName(
      const QString &caption, const QString &filePath, QWidget *parent);
  static QString GetSaveFileName(
      const QString &caption, const QString &filePath, QWidget *parent);

  static void Notify3DDisabled(QWidget *parent);

private:

  QWidget *m_parentWidget;
  static QString m_currentOpenFileName;
  static QString m_currentSaveFileName;
  static QString m_currentDirectory;
};

#endif // ZDIALOGFACTORY_H
