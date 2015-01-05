#ifndef ZFLYEMBODYSPLITPROJECT_H
#define ZFLYEMBODYSPLITPROJECT_H

#include <QObject>
#include <set>
#include "dvid/zdvidtarget.h"
#include "flyem/zflyembookmarklistmodel.h"

class ZStackFrame;
class Z3DWindow;
class ZStackObject;
class ZSwcTree;
class ZObject3dScan;
class ZFlyEmNeuron;

class ZFlyEmBodySplitProject : public QObject
{
  Q_OBJECT

public:
  explicit ZFlyEmBodySplitProject(QObject *parent = 0);
  virtual ~ZFlyEmBodySplitProject();


  void clear();

  void setDvidTarget(const ZDvidTarget &target);
  inline void setBodyId(int bodyId) {
    m_bodyId = bodyId;
  }

  inline int getBodyId() const { return m_bodyId; }
  inline const ZDvidTarget& getDvidTarget() const { return m_dvidTarget; }

  ZFlyEmNeuron getFlyEmNeuron() const;

  bool hasDataFrame() const;
  void setDataFrame(ZStackFrame *frame);
  inline ZStackFrame* getDataFrame() const {
    return m_dataFrame;
  }

  void loadBookmark(const QString &filePath);
  std::set<int> getBookmarkBodySet() const;

  const ZFlyEmBookmarkArray& getBookmarkArray() const {
    return m_bookmarkArray;
  }

  void locateBookmark(const ZFlyEmBookmark &bookmark);

  void clearBookmarkDecoration();
  void addBookmarkDecoration(const ZFlyEmBookmarkArray &bookmarkArray);
  void updateBookDecoration();

  void showSkeleton(ZSwcTree *tree);
  void quickView();

  ZObject3dScan* readBody(ZObject3dScan *out) const;

  void saveSeed();
  void downloadSeed();

  void commitResult();

  void viewPreviousSlice();
  void viewNextSlice();
  void viewFullGrayscale();
  void updateBodyMask();

  void setShowingBodyMask(bool state){
    m_showingBodyMask = state;
  }

signals:
  void messageGenerated(QString);

public slots:
  void showDataFrame() const;
  void showDataFrame3d();
  void showResult3d();
  void showBookmark(bool visible);

  /*!
   * \brief Clear the project without deleting the associated widgets
   *
   * This is usually used when the widgets are destroyed outside.
   */
  void shallowClear();
  void shallowClearDataFrame();
  void shallowClearResultWindow();
  void shallowClearQuickViewWindow();

private:
  bool showingBodyMask() const { return m_showingBodyMask; }
  std::string getSeedKey(int bodyId) const;

private:
  ZDvidTarget m_dvidTarget;
  int m_bodyId;
  ZStackFrame *m_dataFrame;
  Z3DWindow *m_resultWindow;
  Z3DWindow *m_quickViewWindow;
  ZFlyEmBookmarkArray m_bookmarkArray;
  std::vector<ZStackObject*> m_bookmarkDecoration;
  bool m_isBookmarkVisible;
  bool m_showingBodyMask;
};

#endif // ZFLYEMBODYSPLITPROJECT_H
