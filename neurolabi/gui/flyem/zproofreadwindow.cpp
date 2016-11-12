#include "zglew.h"
#include "zproofreadwindow.h"

#include <QHBoxLayout>
#include <QStackedWidget>
#include <QLabel>
#include <QProgressDialog>
#include <QMessageBox>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QDragEnterEvent>

#include "neutubeconfig.h"
#include "dialogs/dvidoperatedialog.h"
#include "flyemsplitcontrolform.h"
#include "dvid/zdvidtarget.h"
#include "zflyemproofmvc.h"
#include "flyem/zflyemproofdoc.h"
#include "flyemproofcontrolform.h"
#include "flyem/zflyemmessagewidget.h"
#include "zwidgetfactory.h"
#include "zdialogfactory.h"
#include "tz_math.h"
#include "zprogresssignal.h"
#include "zwidgetmessage.h"
#include "QsLog.h"
#include "zstackpresenter.h"
#include "flyem/zflyemproofpresenter.h"
#include "zflyembookmarkview.h"
#include "dialogs/flyembodyfilterdialog.h"
#include "zflyemdataloader.h"

ZProofreadWindow::ZProofreadWindow(QWidget *parent) :
  QMainWindow(parent)
{
  init();
}

template <typename T>
void ZProofreadWindow::connectMessagePipe(T *source)
{
  connect(source, SIGNAL(messageGenerated(QString, bool)),
          this, SLOT(dump(QString,bool)));
  connect(source, SIGNAL(errorGenerated(QString, bool)),
          this, SLOT(dumpError(QString,bool)));
  connect(source, SIGNAL(messageGenerated(ZWidgetMessage)),
          this, SLOT(dump(ZWidgetMessage)));
}

void ZProofreadWindow::init()
{
  setFocusPolicy(Qt::ClickFocus);
  setAttribute(Qt::WA_DeleteOnClose);

  QWidget *widget = new QWidget(this);

  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->setMargin(1);
  widget->setLayout(layout);

  ZDvidTarget target;
//  target.set("http://emdata1.int.janelia.org", "9db", 8500);

  m_mainMvc = ZFlyEmProofMvc::Make(target);
  m_mainMvc->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

  layout->addWidget(m_mainMvc);

  QVBoxLayout *controlLayout = new QVBoxLayout(this);

  m_controlGroup = new QStackedWidget(this);
  controlLayout->addWidget(m_controlGroup);


  QHBoxLayout *titleLayout = new QHBoxLayout(this);
  titleLayout->addWidget(ZWidgetFactory::MakeHorizontalLine(this));
  QLabel *titleLabel = new QLabel("<font color=\"Green\">Message</font>", this);
  titleLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
  titleLayout->addWidget(titleLabel);
  titleLayout->addWidget(ZWidgetFactory::MakeHorizontalLine(this));
  controlLayout->addLayout(titleLayout);

  m_messageWidget = new ZFlyEmMessageWidget(this);
  controlLayout->addWidget(m_messageWidget);


  layout->addLayout(controlLayout);

  m_controlGroup->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

  FlyEmProofControlForm *controlForm = new FlyEmProofControlForm;
  controlForm->getUserBookmarkView()->setBookmarkModel(
        m_mainMvc->getUserBookmarkModel(FlyEM::PR_NORMAL));
  controlForm->getAssignedBookmarkView()->setBookmarkModel(
        m_mainMvc->getAssignedBookmarkModel(FlyEM::PR_NORMAL));
  m_mainMvc->registerBookmarkView(controlForm->getUserBookmarkView());
  m_mainMvc->registerBookmarkView(controlForm->getAssignedBookmarkView());
  controlForm->getAssignedBookmarkView()->enableDeletion(false);

  m_controlGroup->addWidget(controlForm);

  FlyEmSplitControlForm *splitControlForm = new FlyEmSplitControlForm;
  splitControlForm->getUserBookmarkView()->setBookmarkModel(
        m_mainMvc->getUserBookmarkModel(FlyEM::PR_SPLIT));
  splitControlForm->getAssignedBookmarkView()->setBookmarkModel(
        m_mainMvc->getAssignedBookmarkModel(FlyEM::PR_SPLIT));
  m_mainMvc->registerBookmarkView(splitControlForm->getUserBookmarkView());
  m_mainMvc->registerBookmarkView(splitControlForm->getAssignedBookmarkView());
  splitControlForm->getAssignedBookmarkView()->enableDeletion(false);

  m_controlGroup->addWidget(splitControlForm);
  splitControlForm->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

  m_mainMvc->connectControlPanel(controlForm);
  m_mainMvc->connectSplitControlPanel(splitControlForm);

  connect(controlForm, SIGNAL(splitTriggered(uint64_t)),
          this, SLOT(launchSplit(uint64_t)));
  connect(controlForm, SIGNAL(splitTriggered(uint64_t)),
          splitControlForm, SLOT(setSplit(uint64_t)));
  connect(controlForm, SIGNAL(splitTriggered()),
          this, SLOT(launchSplit()));
  connect(splitControlForm, SIGNAL(exitingSplit()),
          this, SLOT(exitSplit()));

  connectMessagePipe(m_mainMvc);

  connect(m_mainMvc, SIGNAL(splitBodyLoaded(uint64_t)),
          this, SLOT(presentSplitInterface(uint64_t)));
  connect(m_mainMvc, SIGNAL(dvidTargetChanged(ZDvidTarget)),
          this, SLOT(updateDvidTargetWidget(ZDvidTarget)));

  setCentralWidget(widget);

  m_progressDlg = new QProgressDialog(this);
  m_progressDlg->setWindowModality(Qt::WindowModal);
  m_progressDlg->setAutoClose(true);
  m_progressDlg->setCancelButton(0);
  m_progressDlg->setAutoClose(false);

  m_progressSignal = new ZProgressSignal(this);
  ZProgressSignal::ConnectProgress(m_mainMvc->getProgressSignal(),
        m_progressSignal);
//  m_progressSignal->connectProgress(m_mainMvc->getProgressSignal());
  m_progressSignal->connectSlot(this);

  createMenu();
  createToolbar();
  statusBar()->showMessage("Load a database to start proofreading");

  connect(m_segSlider, SIGNAL(valueChanged(int)),
          m_mainMvc, SLOT(setLabelAlpha(int)));
  m_mainMvc->setLabelAlpha(m_segSlider->value());


  m_mainMvc->enhanceTileContrast(m_contrastAction->isChecked());

  createDialog();
  m_flyemDataLoader = new ZFlyEmDataLoader(this);

  m_defaultPal = palette(); //This has to be the last line to avoid crash
}

ZProofreadWindow* ZProofreadWindow::Make(QWidget *parent)
{
  return new ZProofreadWindow(parent);
}

ZProofreadWindow* ZProofreadWindow::Make(QWidget *parent, ZDvidDialog *dvidDlg)
{
  ZProofreadWindow *window = new ZProofreadWindow(parent);
  if (dvidDlg != NULL) {
    window->setDvidDialog(dvidDlg);
  }

  return window;
}

void ZProofreadWindow::createDialog()
{
  m_dvidOpDlg = new DvidOperateDialog(this);
  m_dvidOpDlg->setDvidDialog(m_mainMvc->getDvidDialog());

  m_bodyFilterDlg = new FlyEmBodyFilterDialog(this);
}

void ZProofreadWindow::setDvidDialog(ZDvidDialog *dvidDlg)
{
  m_mainMvc->setDvidDialog(dvidDlg);
}

void ZProofreadWindow::testSlot()
{
  m_mainMvc->test();
}

void ZProofreadWindow::test()
{
  if (!m_mainMvc->getDvidTarget().isValid()) {
    m_mainMvc->setDvidTarget();
  }
  /*
  ZDvidTarget target;
  target.set("emdata2.int.janelia.org", "3303", 8500);
  target.setBodyLabelName("bodies3");
  target.setLabelBlockName("labels3");
  target.setSynapseName("mb6_synapses");

  m_mainMvc->setDvidTarget(target);
  m_mainMvc->getPresenter()->setObjectVisible(false);
  */

  testSlot();
}

void ZProofreadWindow::createMenu()
{
  QMenu *fileMenu = new QMenu("File", this);

  menuBar()->addMenu(fileMenu);

  m_importBookmarkAction = new QAction("Import Bookmarks", this);
  m_importBookmarkAction->setIcon(QIcon(":/images/import_bookmark.png"));
  fileMenu->addAction(m_importBookmarkAction);
  connect(m_importBookmarkAction, SIGNAL(triggered()),
          m_mainMvc, SLOT(loadBookmark()));

  m_viewMenu = new QMenu("View", this);

  m_viewSynapseAction = new QAction("Synapses", this);
  m_viewSynapseAction->setIcon(QIcon(":/images/synapse.png"));
  m_viewSynapseAction->setCheckable(true);
  m_viewSynapseAction->setChecked(true);
  connect(m_viewSynapseAction, SIGNAL(toggled(bool)),
          m_mainMvc, SLOT(showSynapseAnnotation(bool)));

  m_viewBookmarkAction = new QAction("Bookmarks", this);
  m_viewBookmarkAction->setIcon(QIcon(":/images/view_bookmark.png"));
  m_viewBookmarkAction->setCheckable(true);
  m_viewBookmarkAction->setChecked(true);
  connect(m_viewBookmarkAction, SIGNAL(toggled(bool)),
          m_mainMvc, SLOT(showBookmark(bool)));

  m_viewRoiAction = new QAction("ROI", this);
  m_viewRoiAction->setCheckable(true);
  m_viewRoiAction->setChecked(true);
  m_viewRoiAction->setIcon(QIcon(":/images/view_roi.png"));
  connect(m_viewRoiAction, SIGNAL(toggled(bool)),
          m_mainMvc, SLOT(showRoiMask(bool)));

  m_viewSegmentationAction = new QAction("Segmentation", this);
  m_viewSegmentationAction->setIcon(QIcon(":/images/view_segmentation.png"));
  m_viewSegmentationAction->setCheckable(true);
  m_viewSegmentationAction->setChecked(true);
  connect(m_viewSegmentationAction, SIGNAL(toggled(bool)),
          m_mainMvc, SLOT(showSegmentation(bool)));

  m_viewTodoAction = new QAction("Todo", this);
  m_viewTodoAction->setIcon(QIcon(":/images/view_todo.png"));
  m_viewTodoAction->setCheckable(true);
  m_viewTodoAction->setChecked(true);
  connect(m_viewTodoAction, SIGNAL(toggled(bool)),
          m_mainMvc, SLOT(showTodo(bool)));

  m_contrastAction = new QAction("Enhance Contrast", this);
  m_contrastAction->setCheckable(true);
  m_contrastAction->setChecked(false);
  m_contrastAction->setIcon(QIcon(":images/bc_enhance.png"));
  m_contrastAction->setChecked(true);
  connect(m_contrastAction, SIGNAL(toggled(bool)),
          m_mainMvc, SLOT(enhanceTileContrast(bool)));

  m_smoothAction = new QAction("Smooth Display", this);
  m_smoothAction->setCheckable(true);
  m_smoothAction->setChecked(false);
  m_smoothAction->setIcon(QIcon(":images/smooth.png"));
  m_smoothAction->setChecked(false);
  connect(m_smoothAction, SIGNAL(toggled(bool)),
          m_mainMvc, SLOT(smoothDisplay(bool)));

  m_openSkeletonAction = new QAction("3D Skeletons", this);
  connect(m_openSkeletonAction, SIGNAL(triggered()),
          m_mainMvc, SLOT(showSkeletonWindow()));

  m_openObject3dAction = new QAction("3D Objects", this);
  connect(m_openObject3dAction, SIGNAL(triggered()),
          m_mainMvc, SLOT(showObjectWindow()));

  m_openRoi3dAction = new QAction("3D ROI", this);
  connect(m_openRoi3dAction, SIGNAL(triggered()),
          m_mainMvc, SLOT(showRoi3dWindow()));

//  m_queryTableAction = new QAction("Query Table", this);
//  connect(m_queryTableAction, SIGNAL(triggered()),
//          m_mainMvc, SLOT(showQueryTabel()));

  m_openExtNeuronWindowAction = new QAction("3D Reference Neurons", this);
  m_openExtNeuronWindowAction->setIcon(QIcon(":images/swcpreview.png"));
  connect(m_openExtNeuronWindowAction, SIGNAL(triggered()),
          m_mainMvc, SLOT(showExternalNeuronWindow()));

  m_viewMenu->addAction(m_viewSynapseAction);
  m_viewMenu->addAction(m_viewBookmarkAction);
  m_viewMenu->addAction(m_viewSegmentationAction);
  m_viewMenu->addAction(m_viewTodoAction);
  m_viewMenu->addAction(m_viewRoiAction);
  m_viewMenu->addSeparator();
  m_viewMenu->addAction(m_contrastAction);
  m_viewMenu->addAction(m_smoothAction);
  m_viewMenu->addSeparator();
  m_viewMenu->addAction(m_openObject3dAction);
  m_viewMenu->addAction(m_openExtNeuronWindowAction);

//  menu->addAction(new QAction("test", menu));

  menuBar()->addMenu(m_viewMenu);

  m_toolMenu = new QMenu("Tools", this);

  m_openSequencerAction = new QAction("Open Sequencer", this);
  m_openSequencerAction->setIcon(QIcon(":/images/document.png"));
  connect(m_openSequencerAction, SIGNAL(triggered()),
          m_mainMvc, SLOT(openSequencer()));
  m_toolMenu->addAction(m_openSequencerAction);

  m_openTodoAction = new QAction("Show Todo List", this);
  m_openTodoAction->setIcon(QIcon(":/images/todo.png"));
  connect(m_openTodoAction, SIGNAL(triggered()), m_mainMvc, SLOT(openTodo()));
  m_toolMenu->addAction(m_openTodoAction);

  m_roiToolAction = new QAction("ROI Tool", this);
  m_roiToolAction->setIcon(QIcon(":images/roi.png"));
  connect(m_roiToolAction, SIGNAL(triggered()), m_mainMvc, SLOT(openRoiTool()));
  m_toolMenu->addAction(m_roiToolAction);

  m_openProtocolsAction = new QAction("Open Protocols", this);
  m_openProtocolsAction->setIcon(QIcon(":/images/protocol.png"));
  connect(m_openProtocolsAction, SIGNAL(triggered()),
          m_mainMvc, SLOT(openProtocol()));
  m_toolMenu->addAction(m_openProtocolsAction);

  m_bodyExplorerAction = new QAction("Explore Bodies", this);
  m_bodyExplorerAction->setIcon(QIcon(":/images/open_dvid.png"));
  connect(m_bodyExplorerAction, SIGNAL(triggered()),
          this, SLOT(exploreBody()));
//  m_toolMenu->addAction(m_bodyExplorerAction);

  QMenu *dvidMenu = new QMenu("DVID", this);
  m_dvidOperateAction = new QAction("Operate", this);
  connect(m_dvidOperateAction, SIGNAL(triggered()),
          this, SLOT(operateDvid()));

  dvidMenu->addAction(m_dvidOperateAction);


  m_toolMenu->addMenu(dvidMenu);

  menuBar()->addMenu(m_toolMenu);

  m_advancedMenu = new QMenu("Advanced", this);
  menuBar()->addMenu(m_advancedMenu);
  QAction *mainWindowAction = new QAction("Show NeuTu Desktop", this);
  connect(mainWindowAction, SIGNAL(triggered()),
          this, SIGNAL(showingMainWindow()));
  m_advancedMenu->addAction(mainWindowAction);

  QAction *testAction = new QAction("Test", this);
  connect(testAction, SIGNAL(triggered()), this, SLOT(testSlot()));
  m_advancedMenu->addAction(testAction);


//  m_viewMenu->setEnabled(false);

  m_viewSynapseAction->setEnabled(false);
  m_importBookmarkAction->setEnabled(false);
  m_viewBookmarkAction->setEnabled(false);
  m_viewSegmentationAction->setEnabled(false);
  m_viewRoiAction->setEnabled(false);
  m_viewTodoAction->setEnabled(false);
}

void ZProofreadWindow::addSynapseActionToToolbar()
{
  m_synapseToolbar = new QToolBar(this);
  m_synapseToolbar->setIconSize(QSize(24, 24));
  m_synapseToolbar->addSeparator();
  m_synapseToolbar->addWidget(new QLabel("Synapse"));
  m_synapseToolbar->addAction(
        m_mainMvc->getCompletePresenter()->getAction(
          ZActionFactory::ACTION_SYNAPSE_ADD_PRE));
  m_synapseToolbar->addAction(
        m_mainMvc->getCompletePresenter()->getAction(
          ZActionFactory::ACTION_SYNAPSE_ADD_POST));
  m_synapseToolbar->addAction(
        m_mainMvc->getCompletePresenter()->getAction(
          ZActionFactory::ACTION_SYNAPSE_DELETE));
  m_synapseToolbar->addAction(
        m_mainMvc->getCompletePresenter()->getAction(
          ZActionFactory::ACTION_SYNAPSE_MOVE));
  m_synapseToolbar->addAction(
        m_mainMvc->getCompletePresenter()->getAction(
          ZActionFactory::ACTION_SYNAPSE_LINK));
  m_synapseToolbar->addAction(
        m_mainMvc->getCompletePresenter()->getAction(
          ZActionFactory::ACTION_SYNAPSE_UNLINK));
  m_synapseToolbar->addAction(
        m_mainMvc->getCompletePresenter()->getAction(
          ZActionFactory::ACTION_SYNAPSE_HLPSD));

  m_synapseToolbar->addSeparator();
  m_synapseToolbar->addAction(m_mainMvc->getCompletePresenter()->getAction(
                                ZActionFactory::ACTION_ENTER_RECT_ROI_MODE));

  addToolBar(Qt::LeftToolBarArea, m_synapseToolbar);
}

void ZProofreadWindow::createToolbar()
{
  m_toolBar = new QToolBar(this);
  m_toolBar->setObjectName(QString::fromUtf8("toolBar"));
  m_toolBar->setIconSize(QSize(24, 24));
  addToolBar(Qt::TopToolBarArea, m_toolBar);

  m_toolBar->addAction(m_importBookmarkAction);

  m_toolBar->addSeparator();
  m_toolBar->addAction(m_viewSynapseAction);
  m_toolBar->addAction(m_viewBookmarkAction);
  m_toolBar->addAction(m_viewTodoAction);
  m_toolBar->addAction(m_viewRoiAction);

  m_toolBar->addSeparator();
  m_toolBar->addAction(m_viewSegmentationAction);
  m_segSlider = new QSlider(Qt::Horizontal, this);
  m_segSlider->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
  m_segSlider->setRange(0, 255);
  m_segSlider->setValue(128);
  m_toolBar->addWidget(m_segSlider);

  m_toolBar->addSeparator();
  m_toolBar->addAction(m_contrastAction);
  m_toolBar->addAction(m_smoothAction);
  m_toolBar->addSeparator();
  m_toolBar->addAction(m_openSequencerAction);
  m_toolBar->addAction(m_openTodoAction);
  m_toolBar->addAction(m_openProtocolsAction);
  m_toolBar->addAction(m_roiToolAction);

  addSynapseActionToToolbar();
}

void ZProofreadWindow::presentSplitInterface(uint64_t bodyId)
{
  m_controlGroup->setCurrentIndex(1);

  dump(ZWidgetMessage(
         QString("Body %1 loaded for split.").arg(bodyId),
         NeuTube::MSG_INFORMATION,
         ZWidgetMessage::TARGET_TEXT));
}

void ZProofreadWindow::operateDvid()
{
  m_dvidOpDlg->show();
  m_dvidOpDlg->raise();
}

void ZProofreadWindow::launchSplit(uint64_t bodyId)
{
//  emit progressStarted("Launching split ...");
  dump("Launching split ...", false);
//  m_progressSignal->advanceProgress(0.1);
//  advanceProgress(0.1);
  m_mainMvc->launchSplit(bodyId);

  /*
  if (m_mainMvc->launchSplit(bodyId)) {
    m_controlGroup->setCurrentIndex(1);
    dump(QString("Body %1 loaded for split.").arg(bodyId), true);
  } else {
    dumpError(QString("Failed to load %1").arg(bodyId), true);
  }
  */
}

void ZProofreadWindow::launchSplit()
{
  ZSpinBoxDialog *dlg = ZDialogFactory::makeSpinBoxDialog(this);
  dlg->setValueLabel("Body ID");
  if (dlg->exec()) {
    if (dlg->isSkipped()) {
      m_mainMvc->notifySplitTriggered();
    } else {
      if (dlg->getValue() > 0) {
        launchSplit(dlg->getValue());
      }
    }
  }
}

void ZProofreadWindow::exitSplit()
{
  m_mainMvc->exitSplit();
  m_controlGroup->setCurrentIndex(0);
  dump(ZWidgetMessage(
         "Back from splitting mode.", NeuTube::MSG_INFORMATION,
         ZWidgetMessage::TARGET_TEXT));
}

void ZProofreadWindow::dump(const QString &message, bool appending,
                            bool logging)
{
//  qDebug() << message;
  if (logging) {
    LINFO() << message;
  }

  m_messageWidget->dump(message, appending);
}

void ZProofreadWindow::dumpError(
    const QString &message, bool appending, bool logging)
{
  if (logging) {
    LERROR() << message;
  }

  m_messageWidget->dumpError(message, appending);
}

void ZProofreadWindow::dump(const ZWidgetMessage &msg)
{
  switch (msg.getTarget()) {
  case ZWidgetMessage::TARGET_TEXT:
  case ZWidgetMessage::TARGET_TEXT_APPENDING:
    dump(msg.toHtmlString(), msg.isAppending(), false);
    break;
  case ZWidgetMessage::TARGET_DIALOG:
    QMessageBox::information(this, "Notice", msg.toHtmlString());
    break;
  case ZWidgetMessage::TARGET_STATUS_BAR:
    statusBar()->showMessage(msg.toHtmlString());
    break;
  case ZWidgetMessage::TARGET_CUSTOM_AREA:
    m_mainMvc->dump(msg.toHtmlString());
    break;
  default:
    break;
  }

  //Record message in files
  switch (msg.getType()) {
  case NeuTube::MSG_INFORMATION:
    LINFO() << msg.toPlainString();
    break;
  case NeuTube::MSG_WARNING:
    LWARN() << msg.toPlainString();
    break;
  case NeuTube::MSG_ERROR:
    LERROR() << msg.toPlainString();
    break;
  case NeuTube::MSG_DEBUG:
    LDEBUG() << msg.toPlainString();
    break;
  }
}

void ZProofreadWindow::closeEvent(QCloseEvent */*event*/)
{
  emit proofreadWindowClosed();
}


void ZProofreadWindow::advanceProgress(double dp)
{
  if (getProgressDialog()->isVisible()) {
    if (getProgressDialog()->value() < getProgressDialog()->maximum()) {
      int range = getProgressDialog()->maximum() - getProgressDialog()->minimum();
      getProgressDialog()->setValue(getProgressDialog()->value() + iround(dp * range));
    }
  }
}

void ZProofreadWindow::startProgress()
{
  getProgressDialog()->show();
}

void ZProofreadWindow::startProgress(const QString &title, int nticks)
{
  initProgress(nticks);
  getProgressDialog()->setLabelText(title);
  getProgressDialog()->show();
}

void ZProofreadWindow::startProgress(const QString &title)
{
  startProgress(title, 100);
}

void ZProofreadWindow::endProgress()
{
  getProgressDialog()->reset();
  getProgressDialog()->close();
}

void ZProofreadWindow::initProgress(int nticks)
{
  getProgressDialog()->setRange(0, nticks);
}

void ZProofreadWindow::updateDvidTargetWidget(const ZDvidTarget &target)
{
  setWindowTitle((target.getName() + " @ " + target.getSourceString(false)).c_str());

  m_viewSynapseAction->setEnabled(target.isValid());
  m_importBookmarkAction->setEnabled(target.isValid());
  m_viewBookmarkAction->setEnabled(target.isValid());
  m_viewSegmentationAction->setEnabled(target.isValid());
  m_viewRoiAction->setEnabled(target.isValid());
  m_viewTodoAction->setEnabled(target.isValid());

  m_viewMenu->setEnabled(true);
}

void ZProofreadWindow::dragEnterEvent(QDragEnterEvent *event)
{
  if (event->mimeData()->hasFormat("text/uri-list")) {
    event->acceptProposedAction();
  }
}

void ZProofreadWindow::logMessage(const QString &msg)
{
  LINFO() << msg;
}

void ZProofreadWindow::logMessage(const ZWidgetMessage &msg)
{
  switch (msg.getType()) {
  case NeuTube::MSG_INFORMATION:
    LINFO() << msg.toPlainString();
    break;
  case NeuTube::MSG_WARNING:
    LWARN() << msg.toPlainString();
    break;
  case NeuTube::MSG_ERROR:
    LERROR() << msg.toPlainString();
    break;
  case NeuTube::MSG_DEBUG:
    LDEBUG() << msg.toPlainString();
    break;
  }
}

void ZProofreadWindow::changeEvent(QEvent *event)
{
  if (event->type() == QEvent::ActivationChange) {
    displayActiveHint(isActiveWindow());
  }
}

void ZProofreadWindow::keyPressEvent(QKeyEvent *event)
{
  event->ignore();
}

void ZProofreadWindow::exploreBody()
{
  bool continueLoading = false;
  ZDvidTarget target;
  if (m_mainMvc->getDvidDialog()->exec()) {
    target = m_mainMvc->getDvidDialog()->getDvidTarget();
    if (!target.isValid()) {
      ZDialogFactory::Warn("Invalid DVID", "Invalid DVID server.", this);
    } else {
      continueLoading = true;
    }
  }

  if (continueLoading && m_bodyFilterDlg->exec()) {
    ZDvidFilter dvidFilter = m_bodyFilterDlg->getDvidFilter();
    dvidFilter.setDvidTarget(target);
    m_flyemDataLoader->loadDataBundle(dvidFilter);
  }
}

void ZProofreadWindow::displayActiveHint(bool on)
{
//  setAutoFillBackground(on);
#if 1
  if (on) {
    setPalette(m_defaultPal);
  } else {
    QPalette pal(m_defaultPal);
//    QColor color = pal.background().color();
//    color.setAlpha(200);
    pal.setColor(QPalette::Background, QColor(200, 164, 164, 200));
    setPalette(pal);
  }
#if 0
  (palette());

  // set black background
  Pal.setColor(QPalette::Background, Qt::black);
  m_pMyWidget->setAutoFillBackground(true);
  m_pMyWidget->setPalette(Pal);

  if (on) {
//    setWindowOpacity(1.0);
    setStyleSheet("background-color:black;");
  } else {
//    setWindowOpacity(0.8);
    setStyleSheet("background-color:blue;");
  }

  setPalette(pal);
#endif
#endif
}
