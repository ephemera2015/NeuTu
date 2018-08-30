#ifndef TASKFALSESPLITREVIEW_H
#define TASKFALSESPLITREVIEW_H


#include "protocols/taskprotocoltask.h"
#include "zpoint.h"
#include <QObject>
#include <QTime>
#include <QVector>
#include <set>

class ZFlyEmBody3dDoc;
class ZMesh;
class QAction;
class QCheckBox;
class QComboBox;
class QLabel;
class QNetworkAccessManager;
class QNetworkReply;
class QPushButton;
class QShortcut;


class TaskFalseSplitReview : public TaskProtocolTask
{
  Q_OBJECT
public:
  TaskFalseSplitReview(QJsonObject json, ZFlyEmBody3dDoc *bodyDoc);
  virtual QString tasktype() const override;
  virtual QString actionString() override;
  virtual QString targetString() override;

  virtual void beforeNext() override;
  virtual void beforePrev() override;
  virtual void beforeDone() override;

  virtual QWidget *getTaskWidget() override;
  virtual QMenu *getTaskMenu() override;

  uint64_t getBodyId() const;

private slots:
  void onShowSupervoxelsChanged(int state);
  void onToggleShowSupervoxels();
  void onHideSelected();
  void onClearHidden();

private:
  ZFlyEmBody3dDoc *m_bodyDoc;
  uint64_t m_bodyId;

  QTime m_usageTimer;
  std::vector<int> m_usageTimes;

  QWidget *m_widget;
  QCheckBox *m_showSupervoxelsCheckBox;
  QMenu *m_menu;

  std::set<QString> m_warningTextToSuppress;

  std::set<uint64_t> m_hiddenIds;

  void buildTaskWidget();
  void updateColors();

  void selectBodies(const std::set<uint64_t>& toSelect);

  void applyPerTaskSettings();
  void applyColorMode(bool showingCleaving);

  void updateVisibility();

  void displayWarning(const QString& title, const QString& text,
                      const QString& details = "",
                      bool allowSuppression = false);

  virtual bool loadSpecific(QJsonObject json) override;
  virtual QJsonObject addToJson(QJsonObject json) override;
  virtual void onLoaded() override;
  virtual bool allowCompletion() override;
  virtual void onCompleted() override;

  virtual ProtocolTaskConfig getTaskConfig() const override;

};
#endif // TASKFALSESPLITREVIEW_H
