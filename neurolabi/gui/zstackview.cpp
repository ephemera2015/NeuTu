#include <iostream>
#include <QElapsedTimer>
#include <QMdiArea>
#include <QImageWriter>

#include "zstackview.h"
#include "widgets/zimagewidget.h"
#include "z3dwindow.h"
#include "zimage.h"
#include "zstackdoc.h"
#include "zstackframe.h"
#include "zstackpresenter.h"
#include "zstackdrawable.h"
#include "zslider.h"
#include "zinteractivecontext.h"
#include "zstack.hxx"
#include "zstackdoc.h"
#include "zclickablelabel.h"
#include "tz_error.h"
#include "zstackball.h"
#include "swctreenode.h"
#include "QsLog.h"
#include "zstroke2d.h"
#include "tz_rastergeom.h"
#include "neutubeconfig.h"
#include "zsparsestack.h"
#include "zstackviewparam.h"
#include "zstackfactory.h"
#include "zstackpatch.h"
#include "zstackobjectsourcefactory.h"
#include "zmessagemanager.h"
#include "zmessage.h"
#include "zmessagefactory.h"
#include "zbodysplitbutton.h"
#include "zstackmvc.h"
#include "zpixmap.h"
#include "zlabeledspinboxwidget.h"
#include "zbenchtimer.h"
#include "zstackobjectpainter.h"
#include "dvid/zdvidlabelslice.h"
#include "zstackviewlocator.h"
#include "zscrollslicestrategy.h"

using namespace std;

ZStackView::ZStackView(ZStackFrame *parent) : QWidget(parent)
{
  init();
}

ZStackView::ZStackView(QWidget *parent) : QWidget(parent)
{
  init();
}

ZStackView::~ZStackView()
{
//  m_imagePainter.end();
  m_objectCanvasPainter.end();
  m_tileCanvasPainter.end();
//  m_dynamicObjectCanvasPainter.end();

  if (m_image != NULL) {
    delete m_image;
  }

//  delete m_objectCanvas;
  delete m_dynamicObjectCanvas;

  delete m_activeDecorationCanvas;


  if (m_ctrlLayout != NULL) {
    if (m_ctrlLayout->parent() == NULL) {
      delete m_ctrlLayout;
    }
  }
  delete m_imageMask;

  delete m_sliceStrategy;
//  delete m_tileCanvas;
}

void ZStackView::init()
{
  setFocusPolicy(Qt::ClickFocus);
  m_depthControl = new ZSlider(true, this);
  m_depthControl->setFocusPolicy(Qt::NoFocus);

  m_zSpinBox = new ZLabeledSpinBoxWidget(this);
  m_zSpinBox->setLabel("Z:");
  m_zSpinBox->setFocusPolicy(Qt::ClickFocus);

  m_imageWidget = new ZImageWidget(this);
  m_imageWidget->setSizePolicy(QSizePolicy::Expanding,
                               QSizePolicy::Expanding);
  m_imageWidget->setFocusPolicy(Qt::ClickFocus);
  m_imageWidget->setPaintBundle(&m_paintBundle);

  setSliceAxis(neutube::Z_AXIS);

  m_infoLabel = new QLabel(this);
  m_infoLabel->setText(tr("Stack Information"));
  m_infoLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  m_infoLabel->setFocusPolicy(Qt::NoFocus);

  m_msgLabel = new QLabel(this);
  m_msgLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
  m_msgLabel->setFocusPolicy(Qt::NoFocus);

  m_activeLabel = new QLabel(this);
  m_activeLabel->setWindowFlags(Qt::FramelessWindowHint);
  m_activeLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
  m_activeLabel->setText("<font color='green'>*Active*</font>");
  m_activeLabel->hide();
  m_activeLabel->setFocusPolicy(Qt::NoFocus);

  m_progress = new QProgressBar(this);
  m_progress->setVisible(false);
  m_progress->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
  m_progress->setFocusPolicy(Qt::NoFocus);

  m_topLayout = new QHBoxLayout;
  m_topLayout->addWidget(m_infoLabel);
//  m_topLayout->addWidget(m_msgLabel);
  m_topLayout->addWidget(m_activeLabel);

  //m_topLayout->addWidget(m_progress);


  m_secondTopLayout = new QHBoxLayout;

  m_channelControlLayout = new QHBoxLayout;

  m_secondTopLayout->addLayout(m_channelControlLayout);
  m_secondTopLayout->addWidget(m_msgLabel);
//  m_msgLabel->setText("test");
  m_secondTopLayout->addWidget(m_progress);
//  m_secondTopLayout->addWidget(m_zSpinBox);

  m_secondTopLayout->addSpacerItem(new QSpacerItem(1, m_progress->height(),
                                               QSizePolicy::Preferred,
                                               QSizePolicy::Fixed));


  m_layout = new QVBoxLayout;
  m_layout->setSpacing(0);
  //m_layout->addWidget(m_infoLabel);
  m_layout->addLayout(m_topLayout);
  m_layout->addLayout(m_secondTopLayout);
  m_layout->addSpacing(6);
  m_layout->addWidget(m_imageWidget);

  m_zControlLayout = new QHBoxLayout;
  m_zControlLayout->addWidget(m_depthControl);
  m_zControlLayout->addWidget(m_zSpinBox);
  m_layout->addLayout(m_zControlLayout);

//  m_layout->addWidget(m_depthControl);

#if defined(_ADVANCED_) && !defined(_FLYEM_)
  m_thresholdSlider = new ZSlider(false, this);
  m_thresholdSlider->setFocusPolicy(Qt::NoFocus);

  m_autoThreButton = new QPushButton("Auto", this);
  m_autoThreButton->setFocusPolicy(Qt::NoFocus);

  m_ctrlLayout = new QHBoxLayout;
  m_ctrlLayout->addWidget(m_autoThreButton);  
  m_ctrlLayout->addWidget(m_thresholdSlider);
  m_layout->addLayout(m_ctrlLayout);

  if (!NeutubeConfig::getInstance().getMainWindowConfig().isThresholdControlOn()) {
    hideThresholdControl();
  }
#else
  m_ctrlLayout = NULL;
  m_thresholdSlider = NULL;
  m_autoThreButton = NULL;
#endif

  setLayout(m_layout);

  //m_parent = parent;
  m_image = NULL;
  m_imageMask = NULL;
//  m_objectCanvas = NULL;
  m_dynamicObjectCanvas = NULL;
  m_activeDecorationCanvas = NULL;

  m_dynamicObjectOpacity = 0.5;
//  m_tileCanvas = NULL;
  //m_scrollEnabled = false;
  //updateScrollControl();

  //m_imageWidget->setFocus(); //Cause problem in creating subwindows

  connectSignalSlot();

  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  m_sizeHintOption = neutube::SIZE_HINT_DEFAULT;

  m_isRedrawBlocked = false;

  m_messageManager = NULL;
  m_splitButton = NULL;

  setDepthFrozen(false);
  setViewPortFrozen(false);
  blockViewChangeEvent(false);

  m_sliceStrategy = new ZScrollSliceStrategy;
  //customizeWidget();
}

void ZStackView::enableMessageManager()
{
  if (m_messageManager == NULL) {
    m_messageManager = ZMessageManager::Make<MessageProcessor>(this);
  }
  if (m_splitButton != NULL) {
    m_splitButton->enableMessageManager(m_messageManager);
  }
}


void ZStackView::hideThresholdControl()
{
  m_layout->removeItem(m_ctrlLayout);
  if (m_thresholdSlider != NULL) {
    m_thresholdSlider->hide();
  }
  if (m_autoThreButton != NULL) {
    m_autoThreButton->hide();
  }
}

void ZStackView::setDynamicObjectAlpha(int alpha)
{
  if (alpha < 0) {
    m_dynamicObjectOpacity = 0;
  } else if (alpha > 255) {
    m_dynamicObjectOpacity = 1;
  } else {
    m_dynamicObjectOpacity = (double) alpha / 255.0;
  }
}

void ZStackView::resetViewProj()
{
  ZIntCuboid box = getViewBoundBox();
  m_imageWidget->resetViewProj(
        box.getFirstCorner().getX(), box.getFirstCorner().getY(),
        box.getWidth(), box.getHeight());
}

void ZStackView::setInfo(QString info)
{
  if (m_infoLabel != NULL) {
    m_infoLabel->setText(info);
//    m_infoLabel->update();
  }
}

bool ZStackView::event(QEvent *event)
{
  if (event->type() == QEvent::KeyPress) {
    QKeyEvent *ke = (QKeyEvent*)(event);
    if (ke != NULL) {
      if (ke->key() == Qt::Key_Tab) {
        event->ignore();
        return false;
      }
    }
  }

  return QWidget::event(event);
}


void ZStackView::connectSignalSlot()
{
  /*
  connect(m_depthControl, SIGNAL(valueChanged(int)),
          this, SIGNAL(currentSliceChanged(int)));
  */


  connect(m_depthControl, SIGNAL(valueChanged(int)),
          this, SLOT(processDepthSliderValueChange(int)));
  connect(m_depthControl, SIGNAL(sliderPressed()),
          this, SIGNAL(sliceSliderPressed()));
  connect(m_depthControl, SIGNAL(sliderReleased()),
          this, SIGNAL(sliceSliderReleased()));
  connect(m_zSpinBox, SIGNAL(valueChanged(int)),
          this, SLOT(setZ(int)));
  connect(m_depthControl, SIGNAL(valueChanged(int)),
          this, SLOT(updateZSpinBoxValue()));

//  connect(this, SIGNAL(currentSliceChanged(int)), this, SLOT(redraw()));

  connect(m_imageWidget, SIGNAL(mouseReleased(QMouseEvent*)),
    this, SLOT(mouseReleasedInImageWidget(QMouseEvent*)));
  connect(m_imageWidget, SIGNAL(mouseMoved(QMouseEvent*)),
    this, SLOT(mouseMovedInImageWidget(QMouseEvent*)));
  connect(m_imageWidget, SIGNAL(mousePressed(QMouseEvent*)),
    this, SLOT(mousePressedInImageWidget(QMouseEvent*)));
  connect(m_imageWidget, SIGNAL(mouseDoubleClicked(QMouseEvent*)),
          this, SLOT(mouseDoubleClickedInImageWidget(QMouseEvent*)));
  connect(m_imageWidget, SIGNAL(mouseWheelRolled(QWheelEvent*)),
          this, SLOT(mouseRolledInImageWidget(QWheelEvent*)));

  if (m_thresholdSlider != NULL) {
    connect(m_thresholdSlider, SIGNAL(valueChanged(int)),
            this, SLOT(paintStack()));
  }

  if (m_autoThreButton != NULL) {
    connect(m_autoThreButton, SIGNAL(clicked()),
            this, SLOT(autoThreshold()));
  }

//  connect(this, SIGNAL(viewPortChanged()), this, SLOT(paintActiveTile()));
}

void ZStackView::updateZSpinBoxValue()
{
#if 0
  int z0 = buddyDocument()->getStackOffset(m_sliceAxis);
  int prevIndex = m_zSpinBox->getValue() - z0;
  int currentIndex = getCurrentZ() - z0;
  int newPos = m_sliceStrategy->scroll(prevIndex, currentIndex - prevIndex);
#ifdef _DEBUG_
  std::cout << "Scrolling: " << currentIndex << " " << prevIndex << " "
            << newPos << std::endl;
#endif
#endif

  m_zSpinBox->setValue(getCurrentZ());
}

void ZStackView::setInfo()
{
  if (m_infoLabel != NULL) {
    ZIntCuboid box = getViewBoundBox();
    setInfo(QString("%1 x %2 => %3 x %4").arg(box.getWidth()).
            arg(box.getHeight()).
            arg(m_imageWidget->screenSize().width()).
            arg(m_imageWidget->screenSize().height()));
  }
}

double ZStackView::getCanvasWidthZoomRatio() const
{
  return (double) m_imageWidget->canvasSize().width() /
      m_imageWidget->viewPort().width();
}

double ZStackView::getCanvasHeightZoomRatio() const
{
  return (double) m_imageWidget->canvasSize().height() /
      m_imageWidget->viewPort().height();
}

double ZStackView::getProjZoomRatio() const
{
  return (double) m_imageWidget->projectSize().width() /
      m_imageWidget->viewPort().width();
}

ZIntCuboid ZStackView::getViewBoundBox() const
{
  ZStack *stack = stackData();
  ZIntCuboid box;
  if (stack != NULL) {
    box = stack->getBoundBox();
    box.shiftSliceAxis(m_sliceAxis);
  }

  return box;
}

int ZStackView::getDepth() const
{
  ZStack *stack = stackData();
  if (stack != NULL) {
    return stack->getBoundBox().getDim(m_sliceAxis);
  }

  return 0;
}

void ZStackView::setSliceAxis(neutube::EAxis axis)
{
  m_sliceAxis = axis;
  m_imageWidget->setSliceAxis(axis);
  m_paintBundle.setSliceAxis(axis);
}

void ZStackView::setSliceRange(int minSlice, int maxSlice)
{
  m_depthControl->setRangeQuietly(minSlice, maxSlice);
  m_zSpinBox->setRange(minSlice, maxSlice);
  m_sliceStrategy->setRange(minSlice, maxSlice);
}


#if 0
void ZStackView::resetDepthControl()
{
  ZStack *stack = stackData();
  if (stack != NULL) {
    updateSlider();
//    setSliceRange(0, getDepth() - 1);
//    m_depthControl->setRange(0, getDepth() - 1);
    m_depthControl->setValue(getDepth() / 2);
  }
}
#endif

void ZStackView::reset(bool updatingScreen)
{ 
  ZStack *stack = stackData();
  updateChannelControl();
  if (stack != NULL) {
    updateSlider();
    m_depthControl->initValue(getDepth() / 2);
//    resetDepthControl();
//    m_imageWidget->reset();

    if (updatingScreen) {
      redraw(UPDATE_DIRECT);
    }

    if (stack->isThresholdable()) {
      if (m_thresholdSlider != NULL) {
        m_thresholdSlider->setRange(stack->min(), stack->max());
        m_thresholdSlider->setValue(stack->max());
      }
    } else {
      hideThresholdControl();
    }
  }
  setInfo();
}

void ZStackView::updateThresholdSlider()
{
  if (stackData() != NULL && m_thresholdSlider != NULL) {
    m_thresholdSlider->setRangeQuietly(stackData()->min(), stackData()->max());
    m_thresholdSlider->setValueQuietly(stackData()->max());
  }
}

void ZStackView::updateSlider()
{
  if (stackData() != NULL) {
    ZIntCuboid box = getViewBoundBox();

    int value = m_depthControl->value();
    m_depthControl->setRangeQuietly(0, box.getDepth() - 1);
    if (value >= box.getDepth()) {
      m_depthControl->setValueQuietly(box.getDepth() - 1);
    }

    m_sliceStrategy->setRange(
          m_depthControl->minimum(), m_depthControl->maximum());

    m_zSpinBox->setRange(box.getFirstCorner().getZ(),
                         box.getLastCorner().getZ());
  }
}

void ZStackView::updateViewBox()
{
  resetViewProj();
  updateSlider();
  updateImageCanvas();
  updateObjectCanvas();
  updateTileCanvas();
  updateActiveDecorationCanvas();

  setSliceIndexQuietly(m_depthControl->maximum() / 2);
  processViewChange(true, true);
}

void ZStackView::updateChannelControl()
{  
  QLayoutItem *child;
  while ((child = m_channelControlLayout->takeAt(0)) != 0) {
    if (child->widget()) {
      m_channelControlLayout->removeWidget(child->widget());
      delete child->widget();
    }
    delete child;
  }
  m_chVisibleState.clear();
  m_zSpinBox->setVisible(false);
  ZStack *stack = stackData();
  if (stack != NULL) {
    if (getDepth() > 1) {
      m_zSpinBox->setVisible(true);
    }

    if (!stack->isVirtual()) {
      std::vector<ZVec3Parameter*>& channelColors = stack->channelColors();
      for (int i=0; i<stack->channelNumber(); ++i) {
        m_chVisibleState.push_back(new ZBoolParameter("", true, this));
      }

      for (int ch=0; ch<stack->channelNumber(); ++ch) {
        QWidget *checkWidget = m_chVisibleState[ch]->createWidget();
        checkWidget->setFocusPolicy(Qt::NoFocus);
        if (buddyDocument()->getTag() != neutube::Document::FLYEM_ORTHO) {
          m_channelControlLayout->addWidget(checkWidget, 0, Qt::AlignLeft);
          m_channelControlLayout->addWidget(
                channelColors[ch]->createNameLabel(),0,Qt::AlignLeft);
        }
        ZClickableColorLabel *colorWidget = qobject_cast<ZClickableColorLabel*>
            (channelColors[ch]->createWidget());
        colorWidget->setMinimumHeight(20);
        colorWidget->setMinimumWidth(30);
        colorWidget->setMaximumHeight(20);
        colorWidget->setMaximumWidth(30);
        colorWidget->setFocusPolicy(Qt::NoFocus);
        if (buddyDocument()->getTag() != neutube::Document::FLYEM_ORTHO) {
          m_channelControlLayout->addWidget(colorWidget,0,Qt::AlignLeft);
          m_channelControlLayout->addSpacing(20);
        }

        connect(channelColors[ch], SIGNAL(valueChanged()),
                this, SLOT(redraw()));
        connect(m_chVisibleState[ch], SIGNAL(valueChanged()),
                this, SLOT(redraw()));
      }
      if (!m_channelControlLayout->isEmpty()) {
        m_channelControlLayout->addStretch(1);
      }
    }
  }
}

void ZStackView::autoThreshold()
{
#ifdef _ADVANCED_
  buddyPresenter()->autoThreshold();
#endif
}

#define MULTI_THREAD_VIEW_SIZE_THRESHOLD 65536

QImage::Format ZStackView::stackKindToImageFormat(int kind)
{
  switch (kind) {
  case GREY:
    return QImage::Format_Indexed8;
  case GREY16:
    return QImage::Format_RGB16;
  case COLOR:
    return QImage::Format_RGB888;
  default:
    return QImage::Format_Invalid;
  }
}

ZStack* ZStackView::stackData() const
{
  return (buddyDocument()) ? buddyDocument()->getStack() : NULL;
}

int ZStackView::maxSliceIndex()
{
  return m_depthControl->maximum();
}

int ZStackView::sliceIndex() const
{
  return m_depthControl->value();
}

int ZStackView::getCurrentZ() const
{
  return sliceIndex() +
      buddyDocument()->getStackOffset(m_sliceAxis);
}

void ZStackView::setZ(int z)
{
  setSliceIndex(
        z - buddyDocument()->getStackOffset(m_sliceAxis));
}

void ZStackView::setSliceIndex(int slice)
{
  if (!isDepthFronzen()) {
//    setDepthFrozen(true);
    m_depthControl->setValue(slice);
  }

  //emit viewChanged(getViewParameter(NeuTube::COORD_STACK));
}

void ZStackView::setSliceIndexQuietly(int slice)
{
  if (!isDepthFronzen()) {
    m_depthControl->setValueQuietly(slice);
    m_zSpinBox->setValueQuietly(slice);
  }
}

void ZStackView::stepSlice(int step)
{
  int newIndex = sliceIndex() + step;
  if (newIndex < 0) {
    newIndex = 0;
  } else if (newIndex > maxSliceIndex()) {
    newIndex = maxSliceIndex();
  }

  if (newIndex != sliceIndex()) {
    setSliceIndex(newIndex);
  }
}

int ZStackView::getIntensityThreshold()
{
  int threshold = -1;
  if (m_thresholdSlider != NULL) {
    if (m_thresholdSlider->value() < m_thresholdSlider->maximum()) {
      threshold = m_thresholdSlider->value();
    }
  }

  return threshold;
}

void ZStackView::updatePaintBundle()
{
//  m_paintBundle.unsetSwcNodeList();
  m_paintBundle.clearAllDrawableLists();
  if (buddyDocument()) {
    m_paintBundle.setStackOffset(buddyDocument()->getStackOffset());
  }

  int slice = m_depthControl->value();
  if (buddyPresenter()->interactiveContext().isObjectProjectView()) {
    slice = -slice - 1;
  }
  m_paintBundle.setSliceIndex(slice);
  m_paintBundle.setDisplayStyle(buddyPresenter()->objectStyle());

  m_paintBundle.clearAllDrawableLists();
  // obj
  if (buddyPresenter()->isObjectVisible()) {
    if (buddyDocument()->hasDrawable()) {
      m_paintBundle.addDrawableList(buddyDocument()->drawableList());
    }

    if (buddyPresenter()->hasObjectToShow()) {
      m_paintBundle.addDrawableList(buddyPresenter()->decorations());
    }
  }

  if (buddyPresenter()->hightlightOn()) {
    m_paintBundle.addDrawableList(&(buddyPresenter()->getHighlightDecorationList()));
    buddyPresenter()->setHighlight(false);
  }

  // active deco
  m_paintBundle.addDrawableList(&(buddyPresenter()->getActiveDecorationList()));

}

void ZStackView::restoreFromBadView()
{
  imageWidget()->restoreFromBadView();
}

void ZStackView::updateImageScreen(EUpdateOption option)
{
  ZOUT(LTRACE(), 5) << "ZStackView::updateImageScreen: index="
           << this->getZ(neutube::COORD_STACK);

  if (option != UPDATE_NONE) {
    updatePaintBundle();

    bool blockingPaint = m_isRedrawBlocked || !buddyDocument()->isReadyForPaint();


    m_imageWidget->blockPaint(blockingPaint);

    ZOUT(LTRACE(), 5) << "Updating image widget" << m_imageWidget->screenSize();

    switch (option) {
    case UPDATE_QUEUED:
      m_imageWidget->update();
      break;
    case UPDATE_DIRECT:
      m_imageWidget->repaint();
      break;
    default:
      break;
    }
  }
}

QSize ZStackView::sizeHint() const
{
  QSize viewSize = QWidget::sizeHint();

  switch (m_sizeHintOption) {
  case neutube::SIZE_HINT_CURRENT_BEST:
    //m_imageWidget->updateGeometry();
    viewSize = QWidget::sizeHint();
    break;
  case neutube::SIZE_HINT_TAKING_SPACE:
  {
    ZStackFrame *frame = getParentFrame();
    if (frame != NULL) {
      QMdiArea *area = frame->mdiArea();
      QSize space = area->size();
      QSize margin = QSize(30, 50);
      QSize frameSize = m_imageWidget->canvasSize() + margin;
      if (frameSize.isValid()) {
        int nw, nh;
        Raster_Ratio_Scale(frameSize.width(), frameSize.height(),
                           space.width(), space.height(), &nw, &nh);
        viewSize.setWidth(nw);
        viewSize.setHeight(nh);
      }
      viewSize -= margin;
    } else {
      viewSize = QWidget::sizeHint();
    }
  }
    break;
  default:
    break;
  }

  if (!viewSize.isValid()) {
    viewSize = minimumSizeHint();
  }

  return viewSize;
  //return m_layout->minimumSize();
  /*
  return m_layout->sizeHint() + QSize(
        m_layout->contentsMargins().right() - m_layout->contentsMargins().left(),
        m_layout->contentsMargins().bottom() - m_layout->contentsMargins().top());*/
}

void ZStackView::mouseReleasedInImageWidget(QMouseEvent *event)
{
  buddyPresenter()->processMouseReleaseEvent(event);
}

void ZStackView::mousePressedInImageWidget(QMouseEvent *event)
{
  buddyPresenter()->processMousePressEvent(event);
}

void ZStackView::mouseMovedInImageWidget(QMouseEvent *event)
{
  buddyPresenter()->processMouseMoveEvent(event);
}

void ZStackView::mouseDoubleClickedInImageWidget(QMouseEvent *event)
{
  buddyPresenter()->processMouseDoubleClickEvent(event);
}

bool ZStackView::isDepthScrollable()
{
  return m_depthControl->isEnabled();
}

void ZStackView::mouseRolledInImageWidget(QWheelEvent *event)
{
  int numSteps = -event->delta();

#ifdef _DEBUG_
  std::cout << "Event time: " << event->timestamp() << std::endl;
  std::cout << "Time to event: " << QDateTime::currentMSecsSinceEpoch() << std::endl;
#endif

#if defined(_NEUTUBE_MAC_)
  switch (QSysInfo::MacintoshVersion) {
  case QSysInfo::MV_10_5:
  case QSysInfo::MV_10_6:
#if (QT_VERSION >= QT_VERSION_CHECK(4, 8, 0))
  case QSysInfo::MV_10_7:
#endif
#if (QT_VERSION > QT_VERSION_CHECK(4, 8, 1))
  case QSysInfo::MV_10_8:
#endif
    numSteps = -numSteps;
    break;
  default:
    break;
  }
#endif

//  numSteps = -numSteps;

  if ((abs(numSteps) > 0) && (abs(numSteps) < 120)) {
    if (numSteps > 0) {
      numSteps = 1;
    } else {
      numSteps = -1;
    }
  } else {
    numSteps /= 120;
  }

  if (event->modifiers() == Qt::NoModifier ||
      event->modifiers() == Qt::ShiftModifier) {
    if (isDepthScrollable()) {
      setAttribute(Qt::WA_TransparentForMouseEvents);
      //for strange mighty mouse response in Qt 4.6.2
      if (numSteps != 0) {
        int ratio = 1;
        if (event->modifiers() == Qt::ShiftModifier) {
          ratio = 10;
        }

        int step = numSteps * ratio;
        int newPos = m_sliceStrategy->scroll(sliceIndex(), step);


//        int newPos = m_depthControl->value() + numSteps * ratio;
        if ((newPos >= m_depthControl->minimum()) &&
            (newPos <= m_depthControl->maximum())) {
          setSliceIndex(newPos);
          //m_depthControl->setValue(newPos);

          QPointF pos = imageWidget()->canvasCoordinate(event->pos());
          int z = sliceIndex();
          if (buddyPresenter()->interactiveContext().isProjectView()) {
            z = -1;
          }

          ZPoint pt = ZPoint(pos.x(), pos.y(), z);
//          pt.shiftSliceAxisInverse(getSliceAxis());
          setInfo(buddyDocument()->rawDataInfo(
                    pt.x(), pt.y(), pt.z(), getSliceAxis()));
        }
      }
      setAttribute(Qt::WA_TransparentForMouseEvents, false);
    }
  } else if (event->modifiers() == Qt::ControlModifier) {
    if (numSteps < 0) {
      increaseZoomRatio(event->pos().x(), event->pos().y());
    } else if (numSteps > 0) {
      decreaseZoomRatio(event->pos().x(), event->pos().y());
    }
  }
}

void ZStackView::resizeEvent(QResizeEvent *event)
{
  setInfo();
  event->accept();

  updateActiveDecorationCanvas();
  //buddyPresenter()->updateInteractiveContext();
}

void ZStackView::processStackChange(bool rangeChanged)
{
  updateChannelControl();

  if (rangeChanged) {
    resetViewProj();
    updateSlider();
    setSliceIndexQuietly(m_depthControl->maximum() / 2);
    updateObjectCanvas();
    updateTileCanvas();
    updateActiveDecorationCanvas();
    updateImageCanvas();
  }

  processViewChange(true, true);
}

void ZStackView::redrawObject()
{
  paintObjectBuffer();
  paintDynamicObjectBuffer();
  updateImageScreen(UPDATE_QUEUED);
}

void ZStackView::redraw(EUpdateOption option)
{
//  tic();
  QElapsedTimer timer;
//  ZBenchTimer timer;
  timer.start();

  ZIntCuboid box = getViewBoundBox();

#ifdef _DEBUG_
  std::cout << "View box: " << m_sliceAxis << std::endl;
  std::cout << "  " << box.toJsonArray().dumpString(0) << std::endl;;
#endif

  m_imageWidget->setCanvasRegion(
        box.getFirstCorner().getX(),
        box.getFirstCorner().getY(),
        box.getWidth(), box.getHeight());

  paintStackBuffer();
  qint64 stackPaintTime = timer.elapsed();
  ZOUT(LTRACE(), 5) << "paint stack per frame: " << stackPaintTime;
  paintMaskBuffer();
  paintTileCanvasBuffer();
  qint64 tilePaintTime = timer.elapsed();
  ZOUT(LTRACE(), 5) << "paint tile per frame: " << tilePaintTime;
  paintActiveDecorationBuffer();
  paintDynamicObjectBuffer();
  paintObjectBuffer();
  qint64 objectPaintTime = timer.elapsed();
  ZOUT(LTRACE(), 5) << "paint object per frame: " << objectPaintTime;

  updateImageScreen(option);

//  timer.stop();
//  std::cout << "Paint time per frame: " << timer.time() * 1000 << " ms" << std::endl;
//  std::cout << "paint time per frame: " << toc() << std::endl;
#if defined(_FLYEM_)
  qint64 paintTime = timer.elapsed();

  ZOUT(LTRACE(), 3) << "paint time per frame: " << paintTime;
  if (paintTime > 3000) {
    LWARN() << "Debugging for hiccup: " << "stack: " << stackPaintTime
            << "; tile: " << tilePaintTime << "; object: " << objectPaintTime;
  }
#endif
}


void ZStackView::prepareDocument()
{
  updateSlider();
//  m_objectUpdater.setDocument(buddyDocument());
}

QMenu* ZStackView::leftMenu()
{
  return m_imageWidget->leftMenu();
}

QMenu* ZStackView::rightMenu()
{
  return m_imageWidget->rightMenu();
}

bool ZStackView::popLeftMenu(const QPoint &pos)
{
  return m_imageWidget->popLeftMenu(pos);
}

bool ZStackView::popRightMenu(const QPoint &pos)
{
  return m_imageWidget->popRightMenu(pos);
}

bool ZStackView::showContextMenu(QMenu *menu, const QPoint &pos)
{
  return m_imageWidget->showContextMenu(menu, pos);
}

QStringList ZStackView::toStringList() const
{
  QStringList list;

  list.append(QString("screen size: %1 x %2")
              .arg(imageWidget()->screenSize().width())
              .arg(imageWidget()->screenSize().height()));

  list.append(QString("Offset: %1, %2")
              .arg(imageWidget()->viewPort().left())
              .arg(imageWidget()->viewPort().top()));

  return list;
}

/*
void ZStackView::setImageWidgetCursor(const QCursor & cursor)
{
  imageWidget()->setCursor(cursor);
}
*/
void ZStackView::setScreenCursor(const QCursor &cursor)
{
  imageWidget()->setCursor(cursor);
}

void ZStackView::setScrollStrategy(ZScrollSliceStrategy *strategy)
{
  delete m_sliceStrategy;

  m_sliceStrategy = strategy;
  m_sliceStrategy->setRange(
        m_depthControl->minimum(), m_depthControl->maximum());
}

/*
void ZStackView::resetScreenCursor()
{
  imageWidget()->setCursor(Qt::CrossCursor);
}
*/
void ZStackView::setThreshold(int thre)
{
  if (m_thresholdSlider != NULL) {
    m_thresholdSlider->setValue(thre);
  }
}

void ZStackView::takeScreenshot(const QString &filename)
{
  QImageWriter writer(filename);
  writer.setCompression(1);

  QImage image(iround(m_imageWidget->projectSize().width()),
               iround(m_imageWidget->projectSize().height()),
               QImage::Format_ARGB32);

  m_imageWidget->setViewHintVisible(false);
  m_imageWidget->render(&image);
  m_imageWidget->setViewHintVisible(true);
  ZImage::writeImage(image, filename);
}

//void ZStackView::updateView()
//{
//  redraw(UPDATE_QUEUED);
//}

void ZStackView::displayActiveDecoration(bool display)
{
  m_activeLabel->setVisible(display);
}

void ZStackView::paintSingleChannelStackSlice(ZStack *stack, int slice)
{
  switch (m_sliceAxis) {
  case neutube::Z_AXIS:
  {
    void *dataArray = stack->getDataPointer(0, slice);

    switch (stack->kind()) {
    case GREY:
      if (stack->isBinary()) {
        m_image->setBinaryData(static_cast<uint8_t*>(dataArray),
                               (uint8_t) (stack->min()), getIntensityThreshold());
      } else {
        ZImage::DataSource<uint8_t> stackData(static_cast<uint8_t*>(dataArray),
                                              buddyPresenter()->greyScale(0),
                                              buddyPresenter()->greyOffset(0),
                                              stack->getChannelColor(0));
        m_image->useContrastProtocal(buddyPresenter()->usingHighContrastProtocal());
        m_image->setData(stackData, getIntensityThreshold());
      }
      break;
    case GREY16:
      if (stack->isBinary()) {
        m_image->setBinaryData(static_cast<uint16_t*>(dataArray),
                               (uint16) (stack->min()), getIntensityThreshold());
      } else {
        ZImage::DataSource<uint16_t> stackData(static_cast<uint16_t*>(dataArray),
                                               buddyPresenter()->greyScale(0),
                                               buddyPresenter()->greyOffset(0),
                                               stack->getChannelColor(0));
        m_image->setData(stackData, getIntensityThreshold());
      }
      break;
    default:
      break;
    }
  }
    break;
  case neutube::X_AXIS:
  case neutube::Y_AXIS:
    switch (stack->kind()) {
    case GREY:
      m_image->setData(
            stack->array8(), stack->width(), stack->height(), stack->depth(),
            slice, buddyPresenter()->getGrayScale(),
            buddyPresenter()->getGrayOffset(), m_sliceAxis);
      m_image->enhanceContrast(buddyPresenter()->usingHighContrastProtocal());
      break;
    default:
      break;
    }
    break;
  }
}

void ZStackView::paintMultipleChannelStackSlice(ZStack *stack, int slice)
{
  bool usingMt = false;

  if (stack->width() * stack->height() > MULTI_THREAD_VIEW_SIZE_THRESHOLD) {
    usingMt = true;
  }

  switch (stack->kind()) {
  case GREY: {
    std::vector<ZImage::DataSource<uint8_t> > stackData8;
    for (size_t i=0; i<m_chVisibleState.size(); ++i) {
      if (m_chVisibleState[i]->get()) {
        stackData8.push_back(
              ZImage::DataSource<uint8_t>(
                static_cast<uint8_t*>(stack->getDataPointer(i, slice)),
                buddyPresenter()->greyScale(i),
                buddyPresenter()->greyOffset(i),
                stack->getChannelColor(i)));
#ifdef _DEBUG_
        std::cout <<  stack->getChannelColor(i) << std::endl;
#endif
      }
    }
    m_image->setData(stackData8, 255, usingMt);
  }
    break;
  case GREY16: {
    std::vector<ZImage::DataSource<uint16_t> > stackData16;
    for (size_t i=0; i<m_chVisibleState.size(); ++i) {
      if (m_chVisibleState[i]->get()) {
        stackData16.push_back(
              ZImage::DataSource<uint16_t>(
                static_cast<uint16_t*>(stack->getDataPointer(i, slice)),
                buddyPresenter()->greyScale(i),
                buddyPresenter()->greyOffset(i),
                stack->getChannelColor(i)));
      }
    }
    m_image->setData(stackData16, 255, usingMt);
  }
    break;
  default:
    break;
  }
}

void ZStackView::paintSingleChannelStackMip(ZStack *stack)
{
  Image_Array ima;
  ima.array = (uint8*) stack->projection(
        buddyDocument()->getStackBackground(), ZSingleChannelStack::Z_AXIS);

  switch (stack->kind()) {
  case GREY:
    if (stack->isBinary()) {
      m_image->setBinaryData(ima.array8, (uint8) (stack->min()),
                             getIntensityThreshold());
    } else {
      ZImage::DataSource<uint8_t> stackData(ima.array8,
                                            buddyPresenter()->greyScale(0),
                                            buddyPresenter()->greyOffset(0),
                                            stack->getChannelColor(0));
      m_image->setData(stackData, getIntensityThreshold());
    }
    break;
  case GREY16:
    if (stack->isBinary()) {
      m_image->setBinaryData(ima.array16, (uint16) (stack->min()),
                             getIntensityThreshold());
    } else {
      ZImage::DataSource<uint16_t> stackData(ima.array16,
                                             buddyPresenter()->greyScale(0),
                                             buddyPresenter()->greyOffset(0),
                                             stack->getChannelColor(0));
      m_image->setData(stackData, getIntensityThreshold());
    }
    break;
  }
}

void ZStackView::paintMultipleChannelStackMip(ZStack *stack)
{
  bool usingMt = false;

  if (stack->width() * stack->height() > MULTI_THREAD_VIEW_SIZE_THRESHOLD) {
    usingMt = true;
  }
  switch (stack->kind()) {
  case GREY: {
    std::vector<ZImage::DataSource<uint8_t> > stackData8;
    for (size_t i=0; i<m_chVisibleState.size(); ++i) {
      if (m_chVisibleState[i]->get()) {
        Image_Array ima;
        ima.array8 = (uint8*) stack->projection(
              buddyDocument()->getStackBackground(), ZSingleChannelStack::Z_AXIS, i);
        stackData8.push_back(
              ZImage::DataSource<uint8_t>(ima.array8,
                                          buddyPresenter()->greyScale(i),
                                          buddyPresenter()->greyOffset(i),
                                          stack->getChannelColor(i)));
      }
    }
    m_image->setData(stackData8, 255, usingMt);
  }
    break;
  case GREY16: {
    std::vector<ZImage::DataSource<uint16_t> > stackData16;
    for (size_t i=0; i<m_chVisibleState.size(); ++i) {
      if (m_chVisibleState[i]->get()) {
        Image_Array ima;
        ima.array16 = (uint16*) stack->projection(
              buddyDocument()->getStackBackground(), ZSingleChannelStack::Z_AXIS, i);
        stackData16.push_back(ZImage::DataSource<uint16_t>(ima.array16,
                                                           buddyPresenter()->greyScale(i),
                                                           buddyPresenter()->greyOffset(i),
                                                           stack->getChannelColor(i)));
      }
    }
    m_image->setData(stackData16, 255, usingMt);
  }
    break;
  }
}

void ZStackView::clearCanvas()
{
//  m_imagePainter.end();
  delete m_image;
  m_image = NULL;

  m_imageWidget->setImage(NULL);
}

template<typename T>
void ZStackView::resetCanvasWithStack(T &canvas, ZPainter *painter)
{
  if (canvas != NULL) {
    ZIntCuboid box = getViewBoundBox();
    if (canvas->width() != box.getWidth() ||
        canvas->height() != box.getHeight() ||
        iround(canvas->getTransform().getTx()) !=
        -box.getFirstCorner().getX() ||
        iround(canvas->getTransform().getTy()) !=
        -box.getFirstCorner().getY()) {
      if (painter != NULL) {
        painter->end();
      }
      m_imageWidget->removeCanvas(canvas);
      delete canvas;
      canvas = NULL;
    }
  }
}

void ZStackView::updateContrastProtocal()
{
  if (m_image != NULL) {
    if (buddyPresenter()->hasHighContrastProtocal()) {
      m_image->loadHighContrastProtocal(
            buddyPresenter()->getHighContrastProtocal());
    } else {
      m_image->setDefaultContrastProtocal();
    }
  }
}

void ZStackView::updateImageCanvas()
{
  resetCanvasWithStack(m_image, NULL);
  if (buddyDocument()->hasStackPaint()) {
    ZIntCuboid box = getViewBoundBox();
    if (m_image != NULL) {
      m_image->setOffset(-box.getFirstCorner().getX(),
                         -box.getFirstCorner().getY());
      if ((m_image->width() != box.getWidth()) ||
          (m_image->height() != box.getHeight())) {
        clearCanvas();
      }
    }

    if (m_image == NULL) {
//      double scale = 0.5;
      if (buddyDocument()->hasStackData() &&
          buddyDocument()->getStack()->kind() == GREY &&
          buddyDocument()->getStack()->channelNumber() == 1) {
        m_image = new ZImage(
              box.getWidth(), box.getHeight(), QImage::Format_Indexed8);
      } else {
        m_image = new ZImage(box.getWidth(), box.getHeight());
      }

      updateContrastProtocal();

      m_image->setOffset(-box.getFirstCorner().getX(),
                         -box.getFirstCorner().getY());
//      m_image->setScale(scale, scale);
//      m_imagePainter.begin(m_image);
//      m_imagePainter.setZOffset(box.getFirstCorner().getZ());
      m_imageWidget->setImage(m_image);
    }
  }
}

void ZStackView::updateMaskCanvas()
{
  ZStack *stackMask = buddyDocument()->stackMask();

  if (stackMask == NULL) {
    return;
  }

  if (m_imageMask != NULL) {
    if (m_imageMask->width() != m_image->width() ||
        m_imageMask->height() != m_image->height()) {
      delete m_imageMask;
      m_imageMask = NULL;
    }
  }
  if (m_imageMask == NULL) {
    m_imageMask = m_image->createMask();
    m_imageWidget->setMask(m_imageMask, 0);
  }
}

void ZStackView::clearObjectCanvas()
{
  m_objectCanvasPainter.end();

  m_objectCanvas.clear();
  m_imageWidget->setObjectCanvas(NULL);

#if 0
  m_objectCanvasPainter.end();
  delete m_objectCanvas;
  m_objectCanvas = NULL;

  m_imageWidget->setObjectCanvas(NULL);
#endif
}

void ZStackView::clearTileCanvas()
{
  m_tileCanvasPainter.end();

  m_tileCanvas.clear();
  m_imageWidget->setTileCanvas(NULL);
}

QSize ZStackView::getCanvasSize() const
{
  QSize size(0, 0);
  if (buddyDocument()->hasStack()) {
    ZIntCuboid box = getViewBoundBox();
    size.setWidth(box.getWidth());
    size.setHeight(box.getHeight());
  }

  return size;
}

QSize ZStackView::getScreenSize() const
{
  return m_imageWidget->size();
}

void ZStackView::resetCanvasWithStack(
    ZMultiscalePixmap &canvas, ZPainter *painter)
{
  QSize canvasSize = getCanvasSize();

  ZIntCuboid box = getViewBoundBox();

  int tx = -box.getFirstCorner().getX();
  int ty = -box.getFirstCorner().getY();

  if (canvas.getWidth() != canvasSize.width() ||
      canvas.getHeight() != canvasSize.height() ||
      canvas.getTx() != tx || canvas.getTy() != ty) {
    clearTileCanvas();
    clearObjectCanvas();
    canvas.setSize(canvasSize);
    canvas.setOffset(QPoint(tx, ty));
    if (painter != NULL) {
      painter->setZOffset(box.getFirstCorner().getZ());
    }
  }
}

bool ZStackView::reloadObjectCanvas(bool repaint)
{
  bool reloaded = false;

  QSize canvasSize = getCanvasSize();

  if (!canvasSize.isEmpty() &&
      (buddyDocument()->hasDrawable(ZStackObject::TARGET_OBJECT_CANVAS) ||
      buddyPresenter()->hasDrawable(ZStackObject::TARGET_OBJECT_CANVAS))) {
    double zoomRatio = getProjZoomRatio();
    int level = 0;
    if (zoomRatio < 0.5 && zoomRatio > 0) {
      level = (int) std::floor(1.0 / zoomRatio - 1);
    }
//    level  = 0;
    ZPixmap *pixmap = m_objectCanvas.getPixmap(level);
    m_imageWidget->setObjectCanvas(pixmap);

//    pixmap->cleanUp();
    if (static_cast<QPaintDevice*>(pixmap) != m_objectCanvasPainter.device()) {
      m_objectCanvasPainter.end();

      reloaded = true;

      m_objectCanvasPainter.begin(pixmap);
      m_objectCanvasPainter.setCompositionMode(
            QPainter::CompositionMode_SourceOver);
    }

    TZ_ASSERT(pixmap == m_imageWidget->getObjectCanvas(), "Invalid pixmap");

    if (reloaded && repaint) {
      pixmap->cleanUp();
      paintObjectBuffer();
    }
  } else {
    m_objectCanvas.setVisible(false);
  }

  return reloaded;
}

void ZStackView::reloadTileCanvas()
{
  QSize canvasSize = getCanvasSize();

  if (!canvasSize.isEmpty() &&
      buddyDocument()->hasDrawable(ZStackObject::TARGET_TILE_CANVAS)) {
    double zoomRatio = getProjZoomRatio();
    int level = 1;
    if (zoomRatio > 0) {
      level = (int) std::floor(1.0 / zoomRatio);
    }
    ZPixmap *pixmap = m_tileCanvas.getPixmap(level);

    if (static_cast<QPaintDevice*>(pixmap) != m_tileCanvasPainter.device()) {
      m_tileCanvasPainter.end();
      m_tileCanvasPainter.begin(pixmap);
      m_imageWidget->setTileCanvas(pixmap);
    }
  }
}

void ZStackView::updateObjectCanvas()
{
  resetCanvasWithStack(m_objectCanvas, &m_objectCanvasPainter);
  reloadObjectCanvas();
//#if 0
  ZPixmap *canvas = getCanvas(ZStackObject::TARGET_OBJECT_CANVAS);
  if (canvas != NULL) {
    m_objectCanvasPainter.end();
    canvas->cleanUp();
    m_objectCanvasPainter.begin(canvas);
  }
//#endif
}

void ZStackView::updateTileCanvas()
{
  resetCanvasWithStack(m_tileCanvas, &m_tileCanvasPainter);
  reloadTileCanvas();

#if 0
    if (m_tileCanvas == NULL) {
      double scale = 1.0;
      m_tileCanvas = new ZPixmap(canvasSize * scale);
      m_tileCanvas->setOffset(-buddyDocument()->getStackOffset().getX(),
                              -buddyDocument()->getStackOffset().getY());
//      m_tileCanvas->setScale(scale, scale);
//      m_tileCanvas = new ZPixmap(canvasSize);
//      m_tileCanvas->setOffset(-buddyDocument()->getStackOffset().getX(),
//                              -buddyDocument()->getStackOffset().getY());
      m_tileCanvasPainter.begin(m_tileCanvas);
      m_tileCanvasPainter.setZOffset(buddyDocument()->getStackOffset().getZ());
      m_imageWidget->setTileCanvas(m_tileCanvas);
    }
#endif

    //m_tileCanvas->fill(Qt::transparent);
}

ZPixmap *ZStackView::updateViewPortCanvas(ZPixmap *canvas)
{
  ZStTransform transform = getViewTransform();

  QRect viewPort = getViewPort(neutube::COORD_STACK);
  QSize viewPortSize = viewPort.size();
  QSize newSize = viewPortSize;

  if (canvas != NULL) {
    if (canvas->size() != newSize) {
      delete canvas;
      canvas = NULL;
    }
  }

  if (canvas == NULL) {
    canvas = new ZPixmap(newSize);
  }

  canvas->getProjTransform().estimate(
        QRectF(QPointF(0, 0), QSizeF(viewPortSize)), getProjRegion());
  transform.setScale(1.0, 1.0);
  transform.setOffset(-viewPort.left(), -viewPort.top());

  canvas->setTransform(transform);

  if (canvas != NULL) {
    if (canvas->isVisible()){
      canvas->cleanUp();
    }
  }

  return canvas;
}

#if 1
ZPixmap *ZStackView::updateProjCanvas(ZPixmap *canvas)
{
  QRectF projRect = getProjRegion();
  QSize newCanvasSize = projRect.size().toSize();

  bool usingProjSize = true;

  QRect viewPort = getViewPort(neutube::COORD_STACK);

  //Get transform from viewport to projection region
  ZStTransform transform = getViewTransform();

  //When the projection region is not much smaller or even bigger than viewport,
  //use viewport instead for precise painting. It means that the canvas size
  //will be the same as the viewport size, and as a result, the scale is 1 for
  //painting world objects to the canvas.
  if (transform.getSx() > 1.1) {
    newCanvasSize = viewPort.size();
    usingProjSize = false;
  }

//  qDebug() << "  Canvas size" << newSize;

  if (canvas != NULL) {
    if (canvas->size() != newCanvasSize) {
      delete canvas;
      canvas = NULL;
    }
  }

  if (canvas == NULL) {
    canvas = new ZPixmap(newCanvasSize);
  }

  if (usingProjSize) {
    canvas->getProjTransform().setScale(1.0, 1.0);
    canvas->getProjTransform().setOffset(projRect.left(), projRect.top());
  } else {
    canvas->getProjTransform().estimate(
          QRectF(QPointF(0, 0), QSizeF(newCanvasSize)), getProjRegion());
    transform.setScale(1.0, 1.0);
    transform.setOffset(-viewPort.left(), -viewPort.top());
  }

  canvas->setTransform(transform);

  if (canvas != NULL) {
    if (canvas->isVisible()){
      canvas->cleanUp();
    }
  }

  return canvas;
}
#endif

#if 0
ZPixmap *ZStackView::updateProjCanvas(ZPixmap *canvas)
{
  ZStTransform transform = getViewTransform();

  QSize newSize = getProjRegion().size().toSize();

//  qDebug() << "  Canvas size" << newSize;

  if (canvas != NULL) {
    if (canvas->size() != newSize) {
      delete canvas;
      canvas = NULL;
    }
  }

  if (canvas == NULL) {
    canvas = new ZPixmap(newSize);
  }

  if (transform.getSx() > 1.1) {
    QRect viewPort = getViewPort(NeuTube::COORD_STACK);
    newSize = viewPort.size();
    canvas->getProjTransform().estimate(
          QRectF(QPointF(0, 0), QSizeF(newSize)), getProjRegion());
    transform.setScale(1.0, 1.0);
    transform.setOffset(-viewPort.left(), -viewPort.top());
  } else {
    canvas->getProjTransform().setScale(1.0, 1.0);
  }

  canvas->setTransform(transform);

  if (canvas != NULL) {
    if (canvas->isVisible()){
      canvas->cleanUp();
    }
  }

  return canvas;
}
#endif

void ZStackView::updateDynamicObjectCanvas()
{
//  ZPixmap *newCanvas = updateViewPortCanvas(m_dynamicObjectCanvas);
  ZPixmap *newCanvas = updateProjCanvas(m_dynamicObjectCanvas);
  m_imageWidget->setDynamicObjectCanvas(newCanvas);

  if (m_dynamicObjectCanvas != newCanvas) {
    m_dynamicObjectCanvas = newCanvas;
    /* //doesn't work here. not sure why
    m_dynamicObjectCanvasPainter.end();
    if (m_dynamicObjectCanvas != NULL) {
      m_dynamicObjectCanvasPainter.begin(m_dynamicObjectCanvas);
    }
    */
  }
}

void ZStackView::updateActiveDecorationCanvas()
{
  m_activeDecorationCanvas = updateProjCanvas(m_activeDecorationCanvas);
  m_imageWidget->setActiveDecorationCanvas(m_activeDecorationCanvas);
}

void ZStackView::paintMultiresImageTest(int resLevel)
{
  ZStackPatch *patch = new ZStackPatch(
        ZStackFactory::makeSlice(*(buddyDocument()->getStack()), getCurrentZ()));
  patch->setSource(ZStackObjectSourceFactory::MakeCurrentMsTileSource(resLevel));
  if (resLevel > 0) {
    patch->getStack()->downsampleMax(resLevel, resLevel, 0);
    patch->setXScale(resLevel + 1);
    patch->setYScale(resLevel + 1);
  }
  buddyDocument()->blockSignals(true);
  buddyDocument()->addObject(patch);
  buddyDocument()->blockSignals(false);
  //paintObjectBuffer();

  if (resLevel > 0) {
    paintMultiresImageTest(resLevel - 1);
    //QtConcurrent::run(this, &ZStackView::paintMultiresImageTest, resLevel - 1);
  }
}

void ZStackView::paintStackBuffer()
{
#if 0
  updateCanvas();

  paintMultiresImageTest(1);
#endif

#if 1
  ZStack *stack = stackData();

  if (stack == NULL) {
    return;
  }

  bool showImage = false;
  for (size_t i=0; i<m_chVisibleState.size(); ++i) {
    if (m_chVisibleState[i]->get()) {
      showImage = true;
      break;
    }
  }

  if (!showImage) {
    if (!buddyDocument()->hasVisibleSparseStack()) {
      return;
    }
  }

  updateImageCanvas();

  if (buddyPresenter() != NULL) {
    if (!buddyPresenter()->interactiveContext().isProjectView()) {
      if (!stack->isVirtual() && showImage) {
        if (stack->channelNumber() == 1) {   //grey
          paintSingleChannelStackSlice(stack, m_depthControl->value());
        } else { // multi channel image
          paintMultipleChannelStackSlice(stack, m_depthControl->value());
        }
      } else {
        m_image->setBackground();
//        paintObjectBuffer(m_imagePainter, ZStackObject::TARGET_STACK_CANVAS);

        if (buddyDocument()->hasVisibleSparseStack()) {
          ZStack *slice =
              buddyDocument()->getSparseStack()->getSlice(getCurrentZ());
          //paintSingleChannelStackSlice(slice, 0);
          slice->translate(-buddyDocument()->getStackOffset());
          slice->getOffset().setZ(0);

          m_image->setData(slice, 0, true);
          delete slice;
        }
      }
      //m_scrollEnabled = true;
    } else if (buddyPresenter()->interactiveContext().isProjectView()) {
      //m_scrollEnabled = false;
      if (!stack->isVirtual() && showImage) {
        if (stack->channelNumber() == 1) {
          paintSingleChannelStackMip(stack);
        } else {    // for color image
          paintMultipleChannelStackMip(stack);
        }
      } else {
        m_image->setBackground();
        if (buddyDocument()->hasVisibleSparseStack()) {
          ZStack *slice =
              buddyDocument()->getConstSparseStack()->getMip();
          if (slice != NULL) {
            slice->translate(-buddyDocument()->getStackOffset());
            slice->getOffset().setZ(0);

            m_image->setData(slice, 0, false, true);
            delete slice;
          }
        }
      }
    }
  }
#endif

}

void ZStackView::paintStack()
{
  paintStackBuffer();
  updateImageScreen(UPDATE_QUEUED);
}

void ZStackView::paintMaskBuffer()
{
  ZStack *stackMask = buddyDocument()->stackMask();
  if (stackMask == NULL) {
    return;
  }

  updateMaskCanvas();

  int slice = sliceIndex();
  if (stackMask->kind() == GREY) {

    if (stackMask->channelNumber() >= 3) {
      m_imageMask->set3ChannelData(
            static_cast<uint8_t*>(stackMask->getDataPointer(0, slice)), 1, 0,
            static_cast<uint8_t*>(stackMask->getDataPointer(1, slice)), 1, 0,
            static_cast<uint8_t*>(stackMask->getDataPointer(2, slice)), 1, 0, 100);
    } else if (stackMask->channelNumber() == 2) {
      m_imageMask->set2ChannelData(
            static_cast<uint8_t*>(stackMask->getDataPointer(0, slice)), 1, 0,
            static_cast<uint8_t*>(stackMask->getDataPointer(1, slice)), 1, 0,
            100);
    } else {
      m_imageMask->setCData(static_cast<uint8_t*>(
                              stackMask->getDataPointer(0, slice)), 100);
    }
  } else if (stackMask->kind() == GREY16) {
    m_imageMask->setCData(static_cast<uint8_t*>(
                            stackMask->getDataPointer(0, slice)), 100);
  }
  if (buddyPresenter()->objectStyle() == ZStackObject::BOUNDARY) {
    m_imageMask->enhanceEdge();
  }
}

void ZStackView::paintMask()
{
  paintMaskBuffer();
  updateImageScreen(UPDATE_QUEUED);
}

void ZStackView::paintObjectBuffer(
    ZPainter &painter, ZStackObject::ETarget target)
{
  ZOUT(LTRACE(), 5) << painter.getTransform();

  ZStackObjectPainter paintHelper;
  paintHelper.setRestoringPainter(true);

  painter.setPainted(false);

  bool visible = true;
  if (target == ZStackObject::TARGET_OBJECT_CANVAS ||
      target == ZStackObject::TARGET_DYNAMIC_OBJECT_CANVAS) {
    visible = buddyPresenter()->isObjectVisible();
  }

  if (visible) {
    int slice = m_depthControl->value();
//    int z = slice + buddyDocument()->getStackOffset().getZ();
    int z = getCurrentZ();
    if (buddyPresenter()->interactiveContext().isObjectProjectView()) {
      slice = -slice - 1;
    }

//    painter.setStackOffset(buddyDocument()->getStackOffset());

    if (buddyDocument()->hasDrawable()) {
      QList<ZStackObject*> *objs = buddyDocument()->drawableList();
      QList<const ZStackObject*> visibleObject;
      QList<ZStackObject*>::const_iterator iter = objs->end() - 1;
      for (;iter != objs->begin() - 1; --iter) {
        const ZStackObject *obj = *iter;
#ifdef _DEBUG_2
        std::cout << "Object to display:" << obj << std::endl;
        std::cout << "  " << obj->getSource() << std::endl;
        std::cout << "  " << obj->getTarget() << std::endl;
        std::cout << "  " << obj->isSliceVisible(z, m_sliceAxis) << std::endl;
#endif
        if ((obj->isSliceVisible(z, m_sliceAxis) || slice < 0) &&
            obj->getTarget() == target) {
          visibleObject.append(obj);
        }
      }
      std::sort(visibleObject.begin(), visibleObject.end(),
                ZStackObject::ZOrderLessThan());


      ZOUT(LTRACE(), 5) << "Displaying objects ...";

      for (int i = 0; i < visibleObject.size(); ++i) {
        const ZStackObject *obj = visibleObject[i];
        if (slice == m_depthControl->value() || slice < 0) {
          ZOUT(LTRACE(), 5) << obj->className();

          paintHelper.paint(
                obj, painter, slice, buddyPresenter()->objectStyle(),
                m_sliceAxis);
        }
      }
    }

    if (buddyPresenter()->hasObjectToShow()) {
      QList<ZStackObject*> *objs = buddyPresenter()->decorations();
      for (QList<ZStackObject*>::const_iterator obj = objs->end() - 1;
           obj != objs->begin() - 1; --obj) {
        //(*obj)->display(m_objectCanvas, slice, buddyPresenter()->objectStyle());
        if ((*obj)->getTarget() == target) {
          paintHelper.paint(
                *obj, painter, slice, buddyPresenter()->objectStyle(),
                m_sliceAxis);
//          (*obj)->display(painter, slice, buddyPresenter()->objectStyle());
//          painted = true;
        }
      }
    }
  }

  if (painter.isPainted()) {
    ZPixmap *canvas = getCanvas(target);
    if (canvas != NULL) {
      canvas->setVisible(true);
    }
  }

//  return painted;
}

void ZStackView::paintObjectBuffer()
{
  if (NeutubeConfig::GetVerboseLevel() >= 2) {
    std::cout << "ZStackView::paintObjectBuffer" << std::endl;
  }

  if (buddyPresenter()->isObjectVisible()) {
    updateObjectCanvas();

    paintObjectBuffer(m_objectCanvasPainter, ZStackObject::TARGET_OBJECT_CANVAS);
  } else {
    m_objectCanvasPainter.setPainted(false);
    m_objectCanvas.setVisible(false);
  }
}

bool ZStackView::paintTileCanvasBuffer()
{
#ifdef _DEBUG_
  std::cout << "ZStackView::paintTileCanvasBuffer" << std::endl;
#endif
  bool painted = false;

//  QElapsedTimer timer;
//  timer.start();
  if (buddyDocument()->hasObject(ZStackObject::TARGET_TILE_CANVAS)) {
#ifdef _DEBUG_
    std::cout << "updating tile canvas ..." << std::endl;
#endif
    updateTileCanvas();

    //std::cout << "update time canvas time: " << timer.elapsed() << std::endl;
    if (m_tileCanvasPainter.isActive()) {
#ifdef _DEBUG_
      std::cout << "Painting tile buffer ..." << std::endl;
#endif
      paintObjectBuffer(m_tileCanvasPainter, ZStackObject::TARGET_TILE_CANVAS);
      painted = true;
    }
    //std::cout << "paint tile time: " << timer.elapsed() << std::endl;
  }

//  setDepthFrozen(false);
  setViewPortFrozen(false);

  return painted;
}

void ZStackView::paintObject()
{
  paintObjectBuffer();
  paintDynamicObjectBuffer();
  updateImageScreen(UPDATE_QUEUED);
}

void ZStackView::paintActiveTile()
{
  if (paintTileCanvasBuffer()) {
    updateImageScreen(UPDATE_QUEUED);
  }
}

void ZStackView::paintObject(
    QList<ZStackObject *> selected,
    QList<ZStackObject *> deselected)
{
  bool updatingObjectCanvas = false;
  bool updatingImageCanvas = false;
  foreach (ZStackObject *obj, selected) {
    if (obj->getTarget() == ZStackObject::TARGET_OBJECT_CANVAS) {
      updatingObjectCanvas = true;
    } else if (obj->getTarget() == ZStackObject::TARGET_STACK_CANVAS) {
      updatingImageCanvas = true;
    }
  }

  foreach (ZStackObject *obj, deselected) {
    if (obj->getTarget() == ZStackObject::TARGET_OBJECT_CANVAS) {
      updatingObjectCanvas = true;
    } else if (obj->getTarget() == ZStackObject::TARGET_STACK_CANVAS) {
      updatingImageCanvas = true;
    }
  }

  if (updatingObjectCanvas) {
    paintObjectBuffer();
  }

  if (updatingImageCanvas) {
    paintStackBuffer();
  }

  updateImageScreen(UPDATE_QUEUED);
}

void ZStackView::paintDynamicObjectBuffer()
{
#if 1
  updateDynamicObjectCanvas();

  if (m_dynamicObjectCanvas != NULL) {
    ZPainter painter(m_dynamicObjectCanvas);
    painter.setOpacity(m_dynamicObjectOpacity);
    paintObjectBuffer(painter, ZStackObject::TARGET_DYNAMIC_OBJECT_CANVAS);

    if (painter.isPainted()) {
      m_dynamicObjectCanvas->setVisible(true);
    }
  }
#endif
}

void ZStackView::paintActiveDecorationBuffer()
{
#if 1
//  bool painted = false;
  const QList<ZStackObject*>& drawableList =
      buddyPresenter()->getActiveDecorationList();

  if (!drawableList.isEmpty()) {
    updateActiveDecorationCanvas();

    if (m_activeDecorationCanvas != NULL) {
      ZPainter painter(m_activeDecorationCanvas);
//      qDebug() << "Active painter transform: " << painter.getTransform();

      ZIntPoint pt = buddyDocument()->getStackOffset();
      pt.shiftSliceAxis(getSliceAxis());
//      painter.setStackOffset(pt);

      foreach (ZStackObject *obj, drawableList) {
        if (obj->getTarget() == ZStackObject::TARGET_OBJECT_CANVAS) {
          obj->display(painter, sliceIndex(), ZStackObject::NORMAL, m_sliceAxis);
//          painted = true;
        }
      }
      if (painter.isPainted()) {
        m_activeDecorationCanvas->setVisible(true);
#ifdef _DEBUG_2
        m_activeDecorationCanvas->save((GET_TEST_DATA_DIR + "/test.tif").c_str());
#endif
      }
    }
  }

//  return painted;
#endif
}

void ZStackView::paintActiveDecoration()
{
  paintActiveDecorationBuffer();
  updateImageScreen(UPDATE_QUEUED);
}

ZStack* ZStackView::getStrokeMask(neutube::EColor color)
{
  ZStack *stack = NULL;

  if (!m_objectCanvas.isEmpty()){
    updateObjectCanvas();

    int slice = m_depthControl->value();
    if (buddyPresenter()->interactiveContext().isObjectProjectView()) {
      slice = -slice - 1;
    }

    bool painted = false;
    foreach (ZStroke2d *obj, buddyDocument()->getStrokeList()) {
      bool isMask = false;
      if (obj->isEraser()) {
        isMask = true;
      } else {
        switch (color) {
        case neutube::COLOR_RED:
          isMask = (obj->getColor().red() > 0 && obj->getColor().green() == 0 &&
                    obj->getColor().blue() == 0);
          break;
        case neutube::COLOR_GREEN:
          isMask = (obj->getColor().red() == 0 && obj->getColor().green() > 0 &&
                    obj->getColor().blue() == 0);
          break;
        case neutube::COLOR_BLUE:
          isMask = (obj->getColor().red() == 0 && obj->getColor().green() == 0 &&
                    obj->getColor().blue() > 0);
          break;
        case neutube::COLOR_ALL:
          isMask = true;
          break;
        }

        if (isMask) {
          painted = true;
        }
      }

      if (isMask && painted) {
        m_objectCanvasPainter.end();
        m_objectCanvasPainter.begin(m_objectCanvas.getPixmap(0));
        obj->display(m_objectCanvasPainter,
                     slice, buddyPresenter()->objectStyle(), m_sliceAxis);
      }
    }

    if (painted) {
      stack = getObjectMask(1);
      paintObjectBuffer();
    }
  }

  return stack;
}

ZStack* ZStackView::getStrokeMask(uint8_t maskValue)
{
#if 0
  if (m_objectCanvas != NULL){
    updateObjectCanvas();

    int slice = m_depthControl->value();
    if (buddyPresenter()->interactiveContext().isProjectView()) {
      slice = -1;
    }

    foreach (ZStroke2d *obj, buddyDocument()->getStrokeList()) {
      obj->display(m_objectCanvas, m_imageWidget->viewPort().x(), m_imageWidget->viewPort().y(),
                   m_imageWidget->projectSize().width() * 1.0 / m_imageWidget->viewPort().width(),
                   slice, buddyPresenter()->objectStyle());
    }
  }

  ZStack *stack = getObjectMask(maskValue);

  paintObjectBuffer();

  return stack;
#else
  if (!m_objectCanvas.isEmpty()){
      updateObjectCanvas();

      int slice = m_depthControl->value();
      if (buddyPresenter()->interactiveContext().isObjectProjectView()) {
        slice = -slice - 1;
      }

      m_objectCanvasPainter.end();
      m_objectCanvasPainter.begin(m_objectCanvas.getPixmap(0));

      foreach (ZStroke2d *obj, buddyDocument()->getStrokeList()) {
//        ZPainter painter(m_objectCanvas);
//        painter.setZOffset(buddyDocument()->getStackOffset().getZ());
//        painter.setStackOffset(buddyDocument()->getStackOffset());
        obj->display(m_objectCanvasPainter,
                     slice, buddyPresenter()->objectStyle(), m_sliceAxis);
      }
    }

    ZStack *stack = getObjectMask(maskValue);

    paintObjectBuffer();

    return stack;
#endif
}

ZStack* ZStackView::getObjectMask(uint8_t maskValue)
{
  ZStack *stack = NULL;

  if (!m_objectCanvas.isEmpty()) {
    QPixmap *pixmap = m_objectCanvas.getPixmap(0);

    QImage image;
    image = pixmap->toImage();

    stack = new ZStack(GREY, pixmap->width(),
                       pixmap->height(), 1, 1);
    size_t offset = 0;
    uint8_t *array = stack->array8();
    for (int y = 0; y < pixmap->height(); ++y) {
      for (int x = 0; x < pixmap->width(); ++x) {
        QRgb rgb = image.pixel(x, y);
        if (qRed(rgb) > 0 || qGreen(rgb) > 0 || qBlue(rgb) > 0) {
          array[offset] = maskValue;
        } else {
          array[offset] = 0;
        }
        ++offset;
      }
    }
  }

  return stack;
}

ZStack* ZStackView::getObjectMask(neutube::EColor color, uint8_t maskValue)
{
  ZStack *stack = NULL;

  if (!m_objectCanvas.isEmpty()) {
    QPixmap *pixmap = m_objectCanvas.getPixmap(0);
    stack = new ZStack(GREY, pixmap->width(),
                       pixmap->height(), 1, 1);
    size_t offset = 0;
    uint8_t *array = stack->array8();
    QImage image = pixmap->toImage();
    for (int y = 0; y < pixmap->height(); ++y) {
      for (int x = 0; x < pixmap->width(); ++x) {
        QRgb rgb = image.pixel(x, y);
        bool isForeground = false;
        switch (color) {
        case neutube::COLOR_RED:
          if ((qRed(rgb) > qGreen(rgb)) && (qRed(rgb) > qBlue(rgb))) {
            isForeground = true;
          }
          break;
        case neutube::COLOR_GREEN:
          if ((qGreen(rgb) > qRed(rgb)) && (qGreen(rgb) > qBlue(rgb))) {
            isForeground = true;
          }
          break;
        case neutube::COLOR_BLUE:
          if ((qBlue(rgb) > qRed(rgb)) && (qBlue(rgb) > qGreen(rgb))) {
            isForeground = true;
          }
          break;
        default:
          break;
        }

        if (isForeground) {
          array[offset] = maskValue;
        } else {
          array[offset] = 0;
        }
        ++offset;
      }
    }
  }

  return stack;
}

void ZStackView::exportObjectMask(const string &filePath)
{
  if (!m_objectCanvas.isEmpty()) {
    //m_objectCanvas->save(filePath.c_str());
    ZStack *stack = getObjectMask(255);
    if (stack != NULL) {
      stack->save(filePath);
      delete stack;
    }
  }
}

void ZStackView::exportObjectMask(
    neutube::EColor color, const string &filePath)
{
  if (!m_objectCanvas.isEmpty()) {
    //m_objectCanvas->save(filePath.c_str());
    ZStack *stack = getObjectMask(color, 255);
    if (stack != NULL) {
      stack->save(filePath);
      delete stack;
    }
  }
}

void ZStackView::printViewParam() const
{
#ifdef _DEBUG_
    std::cout << "Axis: " << m_sliceAxis << std::endl;
    getViewProj().print();
#endif
}

void ZStackView::zoomTo(const ZIntPoint &pt)
{
  zoomTo(pt.getX(), pt.getY(), pt.getZ());
}

void ZStackView::zoomTo(int x, int y, int z, int w)
{
//  QRect viewPort = getViewPort(NeuTube::COORD_STACK);
//  int width = imin3(w, viewPort.width(), viewPort.height());
  int width = w;
  if (width < 10) {
    width = 200;
  }

  ZGeometry::shiftSliceAxis(x, y, z, getSliceAxis());

  imageWidget()->zoomTo(QPoint(x, y), width);
  updateSliceFromZ(z);
}

void ZStackView::zoomTo(int x, int y, int z)
{
  zoomTo(x, y, z, 800);
}

void ZStackView::increaseZoomRatio()
{
  increaseZoomRatio(0, 0, false);
}

void ZStackView::decreaseZoomRatio()
{
  decreaseZoomRatio(0, 0, false);
}


void ZStackView::increaseZoomRatio(int x, int y, bool usingRef)
{
  if (!isViewPortFronzen()) {
    recordViewParam();
//    setViewPortFrozen(true);
    imageWidget()->blockPaint(true);
    imageWidget()->increaseZoomRatio(x, y, usingRef);
//    reloadCanvas();
    if (buddyPresenter()->interactiveContext().exploreMode() !=
        ZInteractiveContext::EXPLORE_ZOOM_IN_IMAGE) {
      reloadTileCanvas();
      reloadObjectCanvas(true);
      processViewChange(true);
    }


    imageWidget()->blockPaint(false);

    updateImageScreen(UPDATE_QUEUED);
//    imageWidget()->update();
  }
}

void ZStackView::decreaseZoomRatio(int x, int y, bool usingRef)
{
  if (!isViewPortFronzen()) {
//    reloadCanvas();

//    setViewPortFrozen(true);
    imageWidget()->blockPaint(true);
    imageWidget()->decreaseZoomRatio(x, y, usingRef);
//    reloadCanvas();
    if (buddyPresenter()->interactiveContext().exploreMode() !=
        ZInteractiveContext::EXPLORE_ZOOM_OUT_IMAGE) {
      reloadTileCanvas();
      reloadObjectCanvas(true);

      processViewChange(true, false);
    }

//    notifyViewChanged(NeuTube::View::EXPLORE_ZOOM);
    imageWidget()->blockPaint(false);
    imageWidget()->update();

//    notifyViewPortChanged();
  }
}

ZIntPoint ZStackView::getCenter(neutube::ECoordinateSystem coordSys) const
{
  ZIntPoint center;
  center.setZ(getZ(coordSys));

  QRect rect = getViewPort(coordSys);
  center.setX(rect.center().x());
  center.setY(rect.center().y());

  return center;
}

int ZStackView::getZ(neutube::ECoordinateSystem coordSys) const
{
  int z = sliceIndex();
  if (coordSys == neutube::COORD_STACK) {
    z += buddyDocument()->getStackOffset().getSliceCoord(m_sliceAxis);
  }

  return z;
}

QRect ZStackView::getViewPort(neutube::ECoordinateSystem coordSys) const
{
  QRect rect = m_imageWidget->viewPort();
  if (coordSys == neutube::COORD_RAW_STACK) {
    ZIntCuboid box = getViewBoundBox();
    rect.translate(
          QPoint(-box.getFirstCorner().getX(), -box.getLastCorner().getY()));
    /*
    rect.translate(QPoint(-buddyDocument()->getStackOffset().getX(),
                          -buddyDocument()->getStackOffset().getY()));
                          */
  }

  return rect;
}

QRectF ZStackView::getProjRegion() const
{
  return m_imageWidget->projectRegion();
}

ZViewProj ZStackView::getViewProj() const
{
  return m_imageWidget->getViewProj();
}

void ZStackView::setViewProj(int x0, int y0, double zoom)
{
  m_imageWidget->setViewProj(x0, y0, zoom);
  processViewChange(true, false);
}

void ZStackView::setViewProj(const QPoint &pt, double zoom)
{
  setViewProj(pt.x(), pt.y(), zoom);
}

void ZStackView::setViewProj(const ZViewProj &vp)
{
  recordViewParam();
  m_imageWidget->setViewProj(vp);
  processViewChange(true);
}

ZStackViewParam ZStackView::getViewParameter(
    neutube::ECoordinateSystem coordSys, neutube::View::EExploreAction action) const
{
  ZStackViewParam param(coordSys);
  param.setZ(getZ(coordSys));
  param.setViewProj(getViewProj());
//  param.setViewPort(getViewPort(coordSys));
//  param.setProjRect(getProjRegion());
  param.setExploreAction(action);
  param.setSliceAxis(m_sliceAxis);
  //param.setViewPort(imageWidget()->viewPort());

  return param;
}

ZStTransform ZStackView::getViewTransform() const
{
  ZStTransform transform;
  QRectF projRegion = getProjRegion();
  projRegion.moveTopLeft(QPointF(0, 0));

  transform.estimate(getViewPort(neutube::COORD_STACK), projRegion);

  return transform;
}

void ZStackView::setViewPortOffset(int x, int y)
{
  imageWidget()->blockPaint(true);
  imageWidget()->setViewPortOffset(x, y);
  imageWidget()->blockPaint(false);
  processViewChange(false, false);
  redraw(UPDATE_DIRECT);
//  notifyViewChanged(NeuTube::View::EXPLORE_MOVE);
}

void ZStackView::move(const QPoint &src, const QPointF &dst)
{
  imageWidget()->blockPaint(true);
  imageWidget()->moveViewPort(src, dst);
  imageWidget()->blockPaint(false);
  processViewChange(false, false);
  redraw(UPDATE_DIRECT);
}

void ZStackView::moveViewPort(int dx, int dy)
{
  imageWidget()->moveViewPort(dx, dy);
  processViewChange(false, false);
  redraw(UPDATE_DIRECT);
}

void ZStackView::setViewPortCenter(
    const ZIntPoint &center, neutube::EAxisSystem system)
{
  setViewPortCenter(center.getX(), center.getY(), center.getZ(), system);
}

void ZStackView::recordViewParam()
{
  m_oldViewParam = getViewParameter();
}

void ZStackView::updateSliceFromZ(int z)
{
  bool depthChanged = false;
  int slice =
      z - buddyDocument()->getStackOffset().getSliceCoord(getSliceAxis());
  if (slice != m_depthControl->value()) {
    setSliceIndexQuietly(slice);
    depthChanged = true;
  }
//    setSliceIndex(
//          z - buddyDocument()->getStackOffset().getSliceCoord(getSliceAxis()));
  updateImageScreen(ZStackView::UPDATE_QUEUED);
  processViewChange(true, depthChanged);
}

void ZStackView::setViewPortCenter(
    int x, int y, int z, neutube::EAxisSystem system)
{
  switch (system) {
  case neutube::AXIS_NORMAL:
    ZGeometry::shiftSliceAxis(x, y, z, getSliceAxis());
    setViewPortCenter(x, y, z, neutube::AXIS_SHIFTED);
    break;
  case neutube::AXIS_SHIFTED:
  {
    /* Note that cx=x_0+floor((w-1)/2) */
    imageWidget()->setViewPortOffset(
          x - (imageWidget()->viewPort().width() - 1) / 2,
          y - (imageWidget()->viewPort().height() - 1) / 2);

    updateSliceFromZ(z);
  }
    break;
  }
}

ZIntPoint ZStackView::getViewCenter() const
{
  ZIntPoint center;

  QRect viewPort = getViewPort(neutube::COORD_STACK);
  QPoint viewPortCenter = viewPort.center();
  center.set(viewPortCenter.x(), viewPortCenter.y(),
             getZ(neutube::COORD_STACK));

  center.shiftSliceAxisInverse(getSliceAxis());

  return center;
}

void ZStackView::reloadCanvas()
{
  reloadObjectCanvas();
  reloadTileCanvas();
}

void ZStackView::setViewPort(const QRect &rect)
{
  imageWidget()->setViewPort(rect);

  reloadCanvas();
  processViewChange(false, false);
  redraw();
}

void ZStackView::maximizeViewPort()
{
  imageWidget()->maximizeViewPort();

  reloadCanvas();
  processViewChange(false, false);
  redraw();
}

void ZStackView::setView(const ZStackViewParam &param)
{
  if (isViewChanged(param)) {
    ZIntCuboid box = getViewBoundBox();

    bool depthChanged = false;
    int slice = param.getZ();
//    QRect viewPort = param.getViewPort();

    ZViewProj viewProj = param.getViewProj();

    switch (param.getCoordinateSystem()) {
    case neutube::COORD_RAW_STACK:
    {
      viewProj.move(box.getFirstCorner().getX(),
                    box.getFirstCorner().getY());
    }
      break;
    case neutube::COORD_STACK:
    {
//      QRect viewPort = param.getViewPort();
      slice -= box.getFirstCorner().getZ();
//      setSliceIndexQuietly(param.getZ() - box.getFirstCorner().getZ());
    }
      break;
    default:
      break;
    }

    m_imageWidget->setViewProj(viewProj);
//    m_imageWidget->setViewPort(viewPort);

    if (param.fixingZ() == false) {
      if (slice != m_depthControl->value()) {
        depthChanged = true;
        setSliceIndexQuietly(slice);
      }
    }

    reloadCanvas();

    processViewChange(false, depthChanged);

    redraw();
  }
}

void ZStackView::processDepthSliderValueChange()
{
  processDepthSliderValueChange(m_depthControl->value());
}

void ZStackView::processDepthSliderValueChange(int sliceIndex)
{
  ZOUT(LTRACE(), 5)<< "ZStackView::processDepthSliderValueChange" << sliceIndex;
  /*
  bool hasActiveSlice = false;
  QList<ZDvidLabelSlice*> sliceList = buddyDocument()->getDvidLabelSliceList();
  if (buddyPresenter()->isObjectVisible()) {
    foreach (ZDvidLabelSlice *slice, sliceList) {
      if (slice->isVisible()) {
//        slice->setVisible(false);
        hasActiveSlice = true;
        break;
      }
    }
  }
  */

  processViewChange(false, true);
//  ZStackViewParam param = getViewParameter(NeuTube::COORD_STACK);
//  updateViewData(param);
//  notifyViewChanged(param);

//  notifyViewChanged(NeuTube::View::EXPLORE_SLICE);
  redraw(UPDATE_DIRECT);

  /*
  if (hasActiveSlice) {
    foreach (ZDvidLabelSlice *slice, sliceList) {
      slice->setVisible(true);
    }
  }
  */


}

QSet<ZStackObject::ETarget> ZStackView::updateViewData(
    const ZStackViewParam &param)
{
  ZStackDoc::ActiveViewObjectUpdater updater(buddyDocument());
  if (buddyPresenter()->isObjectVisible()) {
//  QSet<ZStackObject::ETarget> targetSet =
    if (buddyPresenter()->interactiveContext().exploreMode() ==
        ZInteractiveContext::EXPLORE_ZOOM_IN_IMAGE ||
        buddyPresenter()->interactiveContext().exploreMode() ==
        ZInteractiveContext::EXPLORE_ZOOM_OUT_IMAGE) {
      updater.exclude(ZStackObject::TYPE_DVID_LABEL_SLICE);
    }
  } else {
    updater.exclude(ZStackObject::TARGET_OBJECT_CANVAS);
    updater.exclude(ZStackObject::TARGET_DYNAMIC_OBJECT_CANVAS);
  }

  updater.update(param);

  return updater.getUpdatedTargetSet();
}

bool ZStackView::isViewChanged(const ZStackViewParam &param) const
{
  ZStackViewParam currentParam = getViewParameter(neutube::COORD_STACK);

  return (currentParam != param);
}

void ZStackView::processViewChange(bool redrawing)
{
  ZStackViewParam param = getViewParameter();

  if (param != m_oldViewParam) {
    processViewChange(redrawing, param.getZ() != m_oldViewParam.getZ());
  }
}

void ZStackView::processViewChange(bool redrawing, bool depthChanged)
{
  if (!isViewChangeEventBlocked()) {
    ZStackViewParam param = getViewParameter(neutube::COORD_STACK);
    QSet<ZStackObject::ETarget> targetSet = updateViewData(param);
    if (redrawing) {

      for (QSet<ZStackObject::ETarget>::const_iterator iter = targetSet.begin();
           iter != targetSet.end(); ++iter) {
        paintObjectBuffer(*iter);
      }

      if (depthChanged) {
        if (!targetSet.contains(ZStackObject::TARGET_OBJECT_CANVAS)) {
          paintObjectBuffer();
        }
      }

      paintDynamicObjectBuffer();

      if (depthChanged) {
        paintStackBuffer();
      }
    }
    notifyViewChanged(param);
  }
}

void ZStackView::setHoverFocus(bool on)
{
  m_imageWidget->setHoverFocus(on);
}

void ZStackView::setSmoothDisplay(bool on)
{
  m_imageWidget->setSmoothDisplay(on);
  updateImageScreen(UPDATE_QUEUED);
}

/*
void ZStackView::notifyViewChanged(NeuTube::View::EExploreAction action)
{
  notifyViewChanged(getViewParameter(NeuTube::COORD_STACK, action));
}
*/

void ZStackView::notifyViewChanged()
{
  notifyViewChanged(getViewParameter(neutube::COORD_STACK));
}

void ZStackView::notifyViewChanged(const ZStackViewParam &param)
{
  updateActiveDecorationCanvas();

#ifdef _DEBUG_2
  std::cout << "Signal: ZStackView::viewChanged" << std::endl;
#endif
  if (!isViewChangeEventBlocked()) {
#ifdef _DEBUG_2
    std::cout << "BEFORE emit ZStackView::viewChanged" << std::endl;
#endif
//    processViewChange(param);

    emit viewChanged(param);
  }
}

/*
void ZStackView::notifyViewPortChanged()
{
  emit viewPortChanged();
}
*/

bool ZStackView::isImageMovable() const
{
  if (imageWidget()->freeMoving()) {
    return true;
  }

  return (imageWidget()->viewPort() != imageWidget()->canvasRegion());
//  return (imageWidget()->viewPort().top() != 0 ||
//      imageWidget()->viewPort().left() != 0 ||
//      imageWidget()->viewPort().bottom() !=
//      imageWidget()->canvasSize().height() - 1 ||
//      imageWidget()->viewPort().right() !=
//      imageWidget()->canvasSize().width() - 1);
}

void ZStackView::customizeWidget()
{
  if (buddyDocument()->getTag() == neutube::Document::FLYEM_MERGE) {
    QPushButton *mergeButton = new QPushButton(this);
    mergeButton->setText("Merge");
    m_secondTopLayout->addWidget(mergeButton);
    connect(mergeButton, SIGNAL(clicked()), this, SLOT(requestMerge()));

    m_splitButton = new ZBodySplitButton(this);
    m_secondTopLayout->addWidget(m_splitButton);

    QPushButton *quickVis3dButton = new QPushButton(this);
    quickVis3dButton->setText("Low-res 3D");
    m_secondTopLayout->addWidget(quickVis3dButton);
    connect(quickVis3dButton, SIGNAL(clicked()),
            this, SLOT(requestQuick3DVis()));

    /*
    QPushButton *vis3dButton = new QPushButton(this);
    vis3dButton->setText("High-res 3D");
    m_secondTopLayout->addWidget(vis3dButton);
    connect(vis3dButton, SIGNAL(clicked()),
            this, SLOT(requestHighresQuick3DVis()));
            */
  } else {
    QPushButton *vis3dButton = new QPushButton(this);
    vis3dButton->setText("3D");
    vis3dButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    m_secondTopLayout->addWidget(vis3dButton);
    connect(vis3dButton, SIGNAL(clicked()), this, SLOT(request3DVis()));

    if (buddyDocument()->getTag() == neutube::Document::NORMAL) {
      QPushButton *settingButton = new QPushButton(this);
      settingButton->setText("Settings");
      settingButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
      m_secondTopLayout->addWidget(settingButton);
      connect(settingButton, SIGNAL(clicked()), this, SLOT(requestSetting()));
    }

    if (GET_APPLICATION_NAME == "Biocytin") {
      if (buddyDocument() != NULL) {
        if (buddyDocument()->getTag() == neutube::Document::BIOCYTIN_STACK) {
          QPushButton *closeChildFrameButton = new QPushButton(this);
          closeChildFrameButton->setText("Close Projection Windows");
          closeChildFrameButton->setSizePolicy(
                QSizePolicy::Maximum, QSizePolicy::Maximum);
          m_secondTopLayout->addWidget(closeChildFrameButton);
          connect(closeChildFrameButton, SIGNAL(clicked()),
                  this, SLOT(closeChildFrame()));
        }
      }
    }
  }
}

void ZStackView::addHorizontalWidget(QWidget *widget)
{
  if (widget != NULL) {
    m_secondTopLayout->addWidget(widget);
  }
}

void ZStackView::addHorizontalWidget(QSpacerItem *spacer)
{
  if (spacer != NULL) {
    m_secondTopLayout->addSpacerItem(spacer);
  }
}

void ZStackView::closeChildFrame()
{
  emit closingChildFrame();
}

void ZStackView::request3DVis()
{
  if (m_messageManager != NULL) {
    ZMessage message(this);
    if (buddyDocument()->getTag() == neutube::Document::FLYEM_SPLIT) {
      ZMessageFactory::MakeFlyEmSplit3DVisMessage(message);
    } else {
      ZMessageFactory::Make3DVisMessage(message);
    }
    //message.setBodyEntry("title", "3dvis");
    //message.setBodyEntry("body", "message test");

    m_messageManager->processMessage(&message, true);
  }
}

void ZStackView::requestSetting()
{
  emit changingSetting();
}

void ZStackView::requestQuick3DVis()
{
  if (m_messageManager != NULL) {
    ZMessage message(this);
    if (buddyDocument()->getTag() == neutube::Document::FLYEM_MERGE) {
      ZMessageFactory::MakeQuick3DVisMessage(message, 1);
    }
    m_messageManager->processMessage(&message, true);
  }
}

void ZStackView::requestHighresQuick3DVis()
{
  if (m_messageManager != NULL) {
    ZMessage message(this);
    if (buddyDocument()->getTag() == neutube::Document::FLYEM_MERGE) {
      ZMessageFactory::MakeQuick3DVisMessage(message, 0);
    }
    m_messageManager->processMessage(&message, true);
  }
}

void ZStackView::requestMerge()
{
  if (m_messageManager != NULL) {
    /*
    ZMessage message;
    message.setType(ZMessage::TYPE_FLYEM_MERGE);
    message.setOriginalSource(this);
    message.setCurrentSource(this);
    */

    ZMessage message(this);
    message.setType(ZMessage::TYPE_FLYEM_MERGE);

    m_messageManager->processMessage(&message, true);
  }
}

void ZStackView::MessageProcessor::processMessage(
    ZMessage * /*message*/, QWidget * /*host*/) const
{
#ifdef _DEBUG_
  std::cout << "ZStackView::MessageProcessor::processMessage" << std::endl;
#endif
}

ZStackFrame* ZStackView::getParentFrame() const
{
  return qobject_cast<ZStackFrame*>(parent());
}

ZStackMvc* ZStackView::getParentMvc() const
{
  return qobject_cast<ZStackMvc*>(parent());
}

ZSharedPointer<ZStackDoc> ZStackView::buddyDocument() const
{
  if (getParentFrame() != NULL) {
    return getParentFrame()->document();
  } else if (getParentMvc() != NULL) {
    return getParentMvc()->getDocument();
  }
  return ZSharedPointer<ZStackDoc>();
}

ZStackPresenter* ZStackView::buddyPresenter() const
{
  if (getParentFrame() != NULL) {
    return getParentFrame()->presenter();
  } else if (getParentMvc() != NULL) {
    return getParentMvc()->getPresenter();
  }
  return NULL;
}

bool ZStackView::isViewPortFronzen() const
{
  return m_viewPortFrozen;
}

void ZStackView::setViewPortFrozen(bool state)
{
  m_viewPortFrozen = state;
}

bool ZStackView::isDepthFronzen() const
{
  return m_depthFrozen;
}

bool ZStackView::isViewChangeEventBlocked() const
{
  return m_viewChangeEventBlocked;
}

void ZStackView::setDepthFrozen(bool state)
{
  m_depthFrozen = state;
}

void ZStackView::blockViewChangeEvent(bool state)
{
  m_viewChangeEventBlocked = state;
}

void ZStackView::setCanvasVisible(ZStackObject::ETarget target, bool visible)
{
  switch (target) {
  case ZStackObject::TARGET_OBJECT_CANVAS:
    m_objectCanvas.setVisible(visible);
    break;
  case ZStackObject::TARGET_TILE_CANVAS:
    m_tileCanvas.setVisible(visible);
    /*
    if (m_tileCanvas != NULL) {
      m_tileCanvas->setVisible(visible);
    }
    */
    break;
  case ZStackObject::TARGET_DYNAMIC_OBJECT_CANVAS:
    m_dynamicObjectCanvas->setVisible(true);
    break;
  default:
    break;
  }
}

ZPixmap* ZStackView::getCanvas(ZStackObject::ETarget target)
{
  switch (target) {
  case ZStackObject::TARGET_OBJECT_CANVAS:
    return imageWidget()->getObjectCanvas();
  case ZStackObject::TARGET_TILE_CANVAS:
    return imageWidget()->getTileCanvas();
  case ZStackObject::TARGET_DYNAMIC_OBJECT_CANVAS:
    return imageWidget()->getDynamicObjectCanvas();
  default:
    break;
  }

  return NULL;
}

ZPainter* ZStackView::getPainter(ZStackObject::ETarget target)
{
  switch (target) {
  case ZStackObject::TARGET_OBJECT_CANVAS:
    updateObjectCanvas();
    if (m_objectCanvas.isEmpty()) {
      return NULL;
    }

    if (!m_objectCanvasPainter.isActive()) {
      return NULL;
    }

    return &m_objectCanvasPainter;
  case ZStackObject::TARGET_TILE_CANVAS:
    updateTileCanvas();
    if (m_tileCanvas.isEmpty()) {
      return NULL;
    }
    if (!m_tileCanvasPainter.isActive()) {
      return NULL;
    }

    return &m_tileCanvasPainter;
#if 0
  case ZStackObject::TARGET_DYNAMIC_OBJECT_CANVAS:
    updateDynamicObjectCanvas();
    if (!m_dynamicObjectCanvasPainter.isActive()) {
      return NULL;
    }

    return &m_dynamicObjectCanvasPainter;
#endif
  default:
    break;
  }

  return NULL;
}

void ZStackView::paintObjectBuffer(ZStackObject::ETarget target)
{
  ZPainter *painter = getPainter(target);
  if (painter != NULL) {
    paintObjectBuffer(*painter, target);
  } else {
    if (target == ZStackObject::TARGET_DYNAMIC_OBJECT_CANVAS) {
      paintDynamicObjectBuffer();
    }
  }
}

void ZStackView::paintObject(ZStackObject::ETarget target)
{
  ZPainter *painter = getPainter(target);
  if (painter != NULL) {
    paintObjectBuffer(*painter, target);
//    if (painter->isPainted()) {
      updateImageScreen(UPDATE_QUEUED);
//    }
  } else if (target == ZStackObject::TARGET_WIDGET) {
    updateImageScreen(UPDATE_QUEUED);
  } else if (target == ZStackObject::TARGET_DYNAMIC_OBJECT_CANVAS) {
    paintDynamicObjectBuffer();
    updateImageScreen(UPDATE_QUEUED);
  }
}

void ZStackView::paintObject(const QSet<ZStackObject::ETarget> &targetSet)
{
  bool isPainted = false;
  for (QSet<ZStackObject::ETarget>::const_iterator iter = targetSet.begin();
       iter != targetSet.end(); ++iter) {
    ZStackObject::ETarget target = *iter;
    ZPainter *painter = getPainter(target);
    if (painter != NULL) {
      paintObjectBuffer(*painter, target);
//      isPainted = isPainted || painter->isPainted();
      if (painter->isPainted()) {
        setCanvasVisible(target, true);
      }
      isPainted = true;
    } else if (target == ZStackObject::TARGET_WIDGET) {
      isPainted = true;
    } else if (target == ZStackObject::TARGET_DYNAMIC_OBJECT_CANVAS) {
       paintDynamicObjectBuffer();
       isPainted = true;
    }
  }

  if (isPainted) {
    updateImageScreen(UPDATE_QUEUED);
  }
}

void ZStackView::dump(const QString &msg)
{
  m_msgLabel->setText(msg);
}

void ZStackView::highlightPosition(const ZIntPoint &pt)
{
  highlightPosition(pt.getX(), pt.getY(), pt.getZ());
}

void ZStackView::highlightPosition(int x, int y, int z)
{
  ZStackBall *ball = new ZStackBall(x, y, z, 5.0);
  ball->setColor(255, 0, 0);
  ball->addVisualEffect(neutube::display::Sphere::VE_GRADIENT_FILL);
//  ball->display(m_objectCanvasPainter, sliceIndex(), ZStackObject::SOLID);

  buddyPresenter()->setHighlight(true);
  buddyPresenter()->highlight(x, y, z);
//  buddyPresenter()->addDecoration(ball);

  updateImageScreen(UPDATE_QUEUED);
}
