#ifndef ZMOUSEEVENTMAPPER_H
#define ZMOUSEEVENTMAPPER_H

#include <stdlib.h>
#include <map>
#include "zintpoint.h"
#include "zmouseevent.h"
#include "zmouseeventrecorder.h"

class ZInteractiveContext;
class QMouseEvent;
class ZMouseEvent;
class ZStackDoc;
class ZStackOperator;
class ZMouseEventRecorder;

class ZMouseEventMapper
{
public:
  ZMouseEventMapper(ZInteractiveContext *context = NULL,
                    ZStackDoc *doc = NULL);

  /*
  enum EButton {
    LEFT_BUTTON, RIGHT_BUTTON
  };

  enum EAction {
    BUTTON_PRESS, BUTTON_RELEASE
  };
*/
  //virtual EOperation getOperation(QMouseEvent *event);
  virtual ZStackOperator getOperation(const ZMouseEvent &event) const;

  ZStackOperator initOperation() const;
  //virtual EOperation getOperation()

  inline void setContext(ZInteractiveContext *context) {
    m_context = context;
  }

  inline void setDocument(ZStackDoc *doc) {
    m_doc = doc;
  }

  inline void setRecorder(ZMouseEventRecorder *recorder) {
    m_eventRecorder = recorder;
  }

  inline void set(ZInteractiveContext *context, ZStackDoc *doc,
                  ZMouseEventRecorder *recorder)
  {
    setContext(context);
    setDocument(doc);
    setRecorder(recorder);
  }

  void setPosition(int x, int y, int z, Qt::MouseButton button,
                   ZMouseEvent::EAction action);
  ZIntPoint getPosition(Qt::MouseButton button,
                        ZMouseEvent::EAction action) const;

  typedef std::map<Qt::MouseButton, std::map<ZMouseEvent::EAction, ZIntPoint> >
  TMousePosition;

  void process(QMouseEvent *event, int z);

  inline const ZStackDoc* getDocument() const {
    return m_doc;
  }

protected:
  ZInteractiveContext *m_context;
  mutable ZStackDoc *m_doc;
  ZMouseEventRecorder *m_eventRecorder;
  TMousePosition m_position;
};

////////////////////////////////////
class ZMouseEventRightButtonReleaseMapper : public ZMouseEventMapper
{
public:
  ZStackOperator getOperation(const ZMouseEvent &event) const;
};

////////////////////////////////////
class ZMouseEventLeftButtonReleaseMapper : public ZMouseEventMapper
{
public:
  ZStackOperator getOperation(const ZMouseEvent &event) const;
};

////////////////////////////////////
class ZMouseEventLeftButtonPressMapper : public ZMouseEventMapper
{
public:
  ZStackOperator getOperation(const ZMouseEvent &event) const;
};

////////////////////////////////////
class ZMouseEventLeftButtonDoubleClickMapper : public ZMouseEventMapper
{
public:
  ZStackOperator getOperation(const ZMouseEvent &event) const;
};

////////////////////////////////////
class ZMouseEventMoveMapper : public ZMouseEventMapper
{
public:
  ZStackOperator getOperation(const ZMouseEvent &event) const;
};

#endif // ZMOUSEEVENTMAPPER_H
