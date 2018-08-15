#include "zsparsestack.h"

#include <fstream>
#include <iostream>
#include "zstack.hxx"
#include "neutubeconfig.h"
#include "misc/miscutility.h"
#include "zhdf5reader.h"
#include "zhdf5writer.h"
#include "zobject3dscan.h"

//#define MAX_STACK_VOLUME 1847483647
#define MAX_STACK_VOLUME 923741823

//#define MAX_STACK_VOLUME 1000

ZSparseStack::ZSparseStack() :
  m_objectMask(NULL), m_stack(NULL), m_baseValue(1)
{
}

ZSparseStack::~ZSparseStack()
{
  deprecate(ALL_COMPONET);
}

void ZSparseStack::deprecateDependent(EComponent component)
{
  switch (component) {
  case GREY_SCALE:
  case OBJECT_MASK:
    deprecate(STACK);
    break;
  default:
    break;
  }
}

void ZSparseStack::deprecate(EComponent component)
{
  deprecateDependent(component);

  switch (component) {
  case STACK:
    delete m_stack;
    m_stack = NULL;
    break;
  case GREY_SCALE:
    for (ZStackBlockGrid *grid : m_stackGrid) {
      delete grid;
    }
    m_stackGrid.clear();
//    delete m_stackGrid;
//    m_stackGrid = NULL;
    break;
  case OBJECT_MASK:
    delete m_objectMask;
    m_objectMask = NULL;
    break;
  case ALL_COMPONET:
    deprecate(GREY_SCALE);
    deprecate(OBJECT_MASK);
    break;
  }
}

bool ZSparseStack::isDeprecated(EComponent component) const
{
  switch (component) {
  case STACK:
    return m_stack == NULL;
  default:
    break;
  }

  return false;
}

void ZSparseStack::setBaseValue(int baseValue)
{
  if (m_baseValue != baseValue) {
    deprecate(STACK);
    m_baseValue = baseValue;
  }
}

void ZSparseStack::getLineValue(int x,int y,int z,int cnt,double* buffer) const
{
  double *dst=buffer;
  double base_value=m_baseValue;
  memset(dst,0,sizeof(double)*cnt);
  int num=0;
  if(getStackGrid()){
    int width = getStackGrid()->getBlockSize().getX();

    while(num<cnt){
      ZStackBlockGrid::Location location = getStackGrid()->getLocation(x+num, y, z);
      int ofx=location.getLocalPosition().getX();
      ZStack *stack = getStackGrid()->getStack(location.getBlockIndex());
      if(stack!=NULL){
        int ofy=location.getLocalPosition().getY();
        int ofz=location.getLocalPosition().getZ();
        for(int j=0;j<width-ofx && num <cnt;++j,++num){
          *(dst+num)=(double)stack->getIntValueLocal(ofx+j,ofy,ofz)+base_value;
        }
      }
      else{
        num+=width-ofx;
      }
    }
  }
}

double ZSparseStack::getValue(int x,int y,int z) const
{
  double base_value=m_baseValue;

  if(getStackGrid()){
    return getStackGrid()->getValue(x,y,z)+base_value;
  }
  return 0.0;
}

void ZSparseStack::assignStackValue(
    ZStack *stack, const ZObject3dScan &obj, const ZStackBlockGrid &stackGrid,
    const int baseValue)
{
  if (stackGrid.isEmpty() || stackGrid.getStackArray().empty()) {
    for (size_t i = 0; i < obj.getStripeNumber(); ++i) {
      const ZObject3dStripe &stripe = obj.getStripe(i);
      int y = stripe.getY();
      int z = stripe.getZ();
      for (int j = 0; j < stripe.getSegmentNumber(); ++j) {
        int x0 = stripe.getSegmentStart(j);
        int x1 = stripe.getSegmentEnd(j);

        for (int x = x0; x <= x1; ++x) {
          stack->setIntValue(x, y, z, 0, 255);
        }
      }
    }
  } else {
    for (size_t i = 0; i < obj.getStripeNumber(); ++i) {
      const ZObject3dStripe &stripe = obj.getStripe(i);
      int y = stripe.getY();
      int z = stripe.getZ();
      for (int j = 0; j < stripe.getSegmentNumber(); ++j) {
        int x0 = stripe.getSegmentStart(j);
        int x1 = stripe.getSegmentEnd(j);

        for (int x = x0; x <= x1; ++x) {
          int v = stackGrid.getValue(x, y, z) + baseValue;
          stack->setIntValue(x, y, z, 0, v);
        }
      }
    }
  }
}

void ZSparseStack::assignStackValue(
    ZStack *stack, const ZObject3dScan &obj, const ZObject3dScan &border,
    const ZStackBlockGrid &stackGrid,
    const int baseValue)
{
  if (stackGrid.isEmpty() || stackGrid.getStackArray().empty()) {
    for (size_t i = 0; i < obj.getStripeNumber(); ++i) {
      const ZObject3dStripe &stripe = obj.getStripe(i);
      int y = stripe.getY();
      int z = stripe.getZ();
      for (int j = 0; j < stripe.getSegmentNumber(); ++j) {
        int x0 = stripe.getSegmentStart(j);
        int x1 = stripe.getSegmentEnd(j);

        for (int x = x0; x <= x1; ++x) {
          stack->setIntValue(x, y, z, 0, 255);
        }
      }
    }
  } else {
    for (size_t i = 0; i < obj.getStripeNumber(); ++i) {
      const ZObject3dStripe &stripe = obj.getStripe(i);
      int y = stripe.getY();
      int z = stripe.getZ();
      for (int j = 0; j < stripe.getSegmentNumber(); ++j) {
        int x0 = stripe.getSegmentStart(j);
        int x1 = stripe.getSegmentEnd(j);

        for (int x = x0; x <= x1; ++x) {
          int v = stackGrid.getValue(x, y, z) + baseValue;
          stack->setIntValue(x, y, z, 0, v);
        }
      }
    }

    for (size_t i = 0; i < border.getStripeNumber(); ++i) {
      const ZObject3dStripe &stripe = border.getStripe(i);
      int y = stripe.getY();
      int z = stripe.getZ();
      for (int j = 0; j < stripe.getSegmentNumber(); ++j) {
        int x0 = stripe.getSegmentStart(j);
        int x1 = stripe.getSegmentEnd(j);

        for (int x = x0; x <= x1; ++x) {
          if (stack->getIntValue(x, y, z, 0) > baseValue) {
            stack->setIntValue(x, y, z, 0, baseValue);
          }
        }
      }
    }
  }
}

size_t ZSparseStack::GetMaxStackVolume()
{
  return MAX_STACK_VOLUME;
}

bool ZSparseStack::DownsampleRequired(const ZIntCuboid &box)
{
  return box.getVolume() > MAX_STACK_VOLUME;
}

bool ZSparseStack::downsampleRequired() const
{
  if (m_objectMask != NULL) {
    return DownsampleRequired(m_objectMask->getBoundBox());
  }

  return false;
}

ZStack* ZSparseStack::makeDsStack(
    int xintv, int yintv, int zintv, bool preservingGap)
{
  ZStack *out = NULL;

  //Determine the right zoom for making ds stack
//  int zoom = 0;
//  int scale = 1;
//  for (int i = (int) m_stackGrid.size(); i >= 0; --i) {
//    if (m_stackGrid[i]) {
//      scale = zgeom::GetZoomScale(i);
//      if (((xintv + 1) % scale == 0) &&
//          ((yintv + 1) % scale == 0) &&
//          ((zintv + 1) % scale == 0)) {
//        zoom = i;
//        break;
//      }
//    }
//  }

//  const ZStackBlockGrid *stackGrid = getStackGrid(zoom);

  if (m_objectMask != NULL) {
    ZIntCuboid cuboid = m_objectMask->getBoundBox();

    if (!m_objectMask->isEmpty() && !cuboid.isEmpty()) {
      ZObject3dScan *obj = m_objectMask->subobject(cuboid, NULL, NULL);
      ZIntPoint dsIntv(xintv, yintv, zintv);

      if (!dsIntv.isZero()) {
        ZObject3dScan border;
        if (preservingGap) {
          border = obj->getComplementObject();
          border.downsampleMax(xintv, yintv, zintv);
        }
        obj->downsampleMax(dsIntv.getX(), dsIntv.getY(), dsIntv.getZ());
        if (preservingGap) {
          border.intersect(*obj);
        }

        ZStackBlockGrid *dsGrid = makeDownsampleGrid(xintv, yintv, zintv);
//            stackGrid->makeDownsample((dsIntv + 1) / scale - 1);
        out =  new ZStack(GREY, obj->getBoundBox(), 1);
        out->setZero();
        assignStackValue(out, *obj, *dsGrid, m_baseValue);
        out->setDsIntv(dsIntv);
        delete dsGrid;
        delete obj;
      } else {
        out = new ZStack(GREY, cuboid, 1);
        out->setZero();
        assignStackValue(out, *obj, *getStackGrid(), m_baseValue);
        delete obj;
      }
    }
  }

  return out;
}

ZStack* ZSparseStack::makeIsoDsStack(size_t maxVolume, bool preservingGap)
{
  ZStack *out = NULL;
  if (m_objectMask != NULL && getStackGrid() != NULL) {
    ZIntCuboid cuboid = m_objectMask->getBoundBox();

    if (!m_objectMask->isEmpty() && !cuboid.isEmpty()) {
//      ZObject3dScan *obj = m_objectMask->subobject(cuboid, NULL, NULL);
      int dsIntv = misc::getIsoDsIntvFor3DVolume(cuboid, maxVolume, true);

#ifdef _DEBUG_
      std::cout << "Downsampling: " << dsIntv << std::endl;
#endif

      out = makeDsStack(dsIntv, dsIntv, dsIntv, preservingGap);
#if 0
      if (dsIntv > 0) {
        ZObject3dScan border;
        if (preservingGap) {
          border = obj->getComplementObject();
          border.downsampleMax(dsIntv, dsIntv, dsIntv);
        }
        obj->downsampleMax(dsIntv, dsIntv, dsIntv);
        if (preservingGap) {
          border.intersect(*obj);
        }

        ZStackBlockGrid *dsGrid =
            m_stackGrid->makeDownsample(dsIntv, dsIntv, dsIntv);
        out =  new ZStack(GREY, obj->getBoundBox(), 1);
        out->setZero();
        assignStackValue(out, *obj, border, *dsGrid, m_baseValue);
        out->setDsIntv(ZIntPoint(dsIntv, dsIntv, dsIntv));
        delete dsGrid;
        delete obj;
      } else {
        out = new ZStack(GREY, cuboid, 1);
        out->setZero();
        assignStackValue(out, *obj, *m_stackGrid, m_baseValue);
        delete obj;
      }
#endif
    }
  }

  return out;
}

ZStackBlockGrid* ZSparseStack::makeDownsampleGrid(
    int xintv, int yintv, int zintv) const
{
  int zoom = 0;
  int scale = 1;
  for (int i = (int) m_stackGrid.size(); i >= 0; --i) {
    if (m_stackGrid[i]) {
      scale = zgeom::GetZoomScale(i);
      if (((xintv + 1) % scale == 0) &&
          ((yintv + 1) % scale == 0) &&
          ((zintv + 1) % scale == 0)) {
        zoom = i;
        break;
      }
    }
  }

  const ZStackBlockGrid *stackGrid = getStackGrid(zoom);
  ZStackBlockGrid *dsGrid = NULL;

  if (stackGrid) {
    stackGrid->makeDownsample((ZIntPoint(xintv, yintv, zintv) + 1) / scale - 1);
  }

  return dsGrid;
}

ZStack* ZSparseStack::makeStack(
    const ZIntCuboid &box, size_t maxVolume, bool preservingGap)
{
  ZStack *out = NULL;
  if (m_objectMask != NULL && getStackGrid() != NULL) {
    ZIntCuboid cuboid = m_objectMask->getBoundBox();
    if (!box.isEmpty()) {
      cuboid.intersect(box);
    }
    if (!m_objectMask->isEmpty() && !cuboid.isEmpty()) {
      size_t volume = cuboid.getVolume();
      double dsRatio = (double) volume / maxVolume;
      ZObject3dScan *obj = m_objectMask->subobject(cuboid, NULL, NULL);

      if (dsRatio > 1.0) {
        ZIntPoint tmpDsIntv = misc::getDsIntvFor3DVolume(dsRatio);

        ZObject3dScan border;
        if (preservingGap) {
          border = obj->getComplementObject();
          border.downsampleMax(tmpDsIntv);
        }

        obj->downsampleMax(tmpDsIntv.getX(), tmpDsIntv.getY(), tmpDsIntv.getZ());

        if (preservingGap) {
          border.intersect(*obj);
        }

        ZStackBlockGrid *dsGrid = makeDownsampleGrid(
              tmpDsIntv.getX(), tmpDsIntv.getY(), tmpDsIntv.getZ());
//            m_stackGrid->makeDownsample(
//              tmpDsIntv.getX(), tmpDsIntv.getY(), tmpDsIntv.getZ());
#ifdef _DEBUG_2
  return NULL;
#endif
        out =  new ZStack(GREY, obj->getBoundBox(), 1);
        out->setZero();
        assignStackValue(out, *obj, border, *dsGrid, m_baseValue);
        out->setDsIntv(tmpDsIntv);
        out->pushDsIntv(getDsIntv());
        delete dsGrid;
        delete obj;
      } else {
        out = new ZStack(GREY, cuboid, 1);
        out->setZero();
        assignStackValue(out, *obj, *getStackGrid(), m_baseValue);
        out->pushDsIntv(getDsIntv());
        delete obj;
      }
    } else {
      ZOUT(LTRACE(), 5) << "Invalid range. No stack created.";
    }
  } else {
    ZOUT(LTRACE(), 5) << "Object mask or grayscale data missing. No stack created.";
  }

  return out;
}

ZStack* ZSparseStack::makeStack(const ZIntCuboid &box, bool preservingGap)
{
  return makeStack(box, MAX_STACK_VOLUME, preservingGap);
}

ZIntPoint ZSparseStack::getDenseDsIntv() const
{
  if (m_stack != NULL) {
    return m_stack->getDsIntv();
  }

  return ZIntPoint(0, 0, 0);
}

void ZSparseStack::pushDsIntv(int x, int y, int z)
{
  m_dsIntv.setX((m_dsIntv.getX() + 1) * (x + 1) - 1);
  m_dsIntv.setY((m_dsIntv.getY() + 1) * (y + 1) - 1);
  m_dsIntv.setZ((m_dsIntv.getZ() + 1) * (z + 1) - 1);
}

void ZSparseStack::pushDsIntv(const ZIntPoint &dsIntv)
{
  pushDsIntv(dsIntv.getX(), dsIntv.getY(), dsIntv.getZ());
}

ZStack* ZSparseStack::getStack()
{
  if (m_objectMask == NULL) {
    return NULL;
  }

  if (isDeprecated(STACK)) {
    ZIntCuboid cuboid = m_objectMask->getBoundBox();
    if (!m_objectMask->isEmpty()) {
      size_t volume = cuboid.getVolume();
      double dsRatio = (double) volume / MAX_STACK_VOLUME;
      if (dsRatio > 1.0) {
        ZObject3dScan obj = *m_objectMask;
        ZIntPoint dsIntv = misc::getDsIntvFor3DVolume(dsRatio);
//        int intv = misc::getIsoDsIntvFor3DVolume(dsRatio);
//        m_dsIntv.set(intv, intv, intv);

#ifdef _DEBUG_
        std::cout << "Downsampling:" << dsIntv.toString() << std::endl;
#endif

        obj.downsampleMax(dsIntv.getX(), dsIntv.getY(), dsIntv.getZ());

        ZStackBlockGrid *dsGrid = makeDownsampleGrid(
              dsIntv.getX(), dsIntv.getY(), dsIntv.getZ());
#ifdef _DEBUG_2
        return NULL;
#endif
        m_stack =  new ZStack(GREY, obj.getBoundBox(), 1);
        m_stack->setZero();
        assignStackValue(m_stack, obj, *dsGrid, m_baseValue);
        m_stack->setDsIntv(getDsIntv());
        m_stack->pushDsIntv(dsIntv);
        delete dsGrid;
      } else {
        m_stack = new ZStack(GREY, cuboid, 1);
        m_stack->setZero();
        assignStackValue(m_stack, *m_objectMask, *getStackGrid(), m_baseValue);
        m_stack->setDsIntv(getDsIntv());
//        m_dsIntv.set(0, 0, 0);
      }
    }
  }

#ifdef _DEBUG_2
  m_stack->save(GET_TEST_DATA_DIR + "/test.tif");
#endif

  return m_stack;
}

const ZStack* ZSparseStack::getStack() const
{
  return dynamic_cast<const ZStack*>(
        const_cast<ZSparseStack*>(this)->getStack());
}

size_t ZSparseStack::getObjectVolume() const
{
  if (m_objectMask == NULL) {
    return 0;
  }

  return m_objectMask->getVoxelNumber();
}

void ZSparseStack::shakeOff()
{
  if (m_objectMask != NULL) {
    std::vector<ZObject3dScan> objArray =
        m_objectMask->getConnectedComponent(ZObject3dScan::ACTION_NONE);
    if (objArray.size() > 1) {
      size_t maxV = objArray[0].getVoxelNumber();
      size_t selected = 0;
      for (size_t i = 1; i < objArray.size(); ++i) {
        const ZObject3dScan &obj = objArray[i];
        size_t v = obj.getVoxelNumber();
        if (v > maxV) {
          maxV = v;
          selected = i;
        }
      }
      *m_objectMask = objArray[selected];
    }
  }
}

ZStack* ZSparseStack::getSlice(int z) const
{
  ZObject3dScan slice = m_objectMask->getSlice(z);
  ZIntCuboid box = slice.getBoundBox();
  ZStack *stack = new ZStack(GREY, box, 1);
  stack->setZero();
  assignStackValue(stack, slice, *getStackGrid(), 1);

  return stack;
}

ZStack* ZSparseStack::getMip() const
{
  ZStack *stack = NULL;
  if (m_objectMask != NULL) {
    ZIntCuboid box = m_objectMask->getBoundBox();
    box.setFirstZ(0);
    box.setLastZ(0);
    stack = new ZStack(GREY, box, 1);
    ZObject3dScan::ConstSegmentIterator iterator(m_objectMask);
    while (iterator.hasNext()) {
      const ZObject3dScan::Segment &seg = iterator.next();
      int y = seg.getY();
//      int z = seg.getZ();
      for (int x = seg.getStart(); x <= seg.getEnd(); ++x) {
        stack->setIntValue(x, y, 0, 0, 164);
      }
    }

  }

  return stack;
}

void ZSparseStack::setGreyScale(ZStackBlockGrid *stackGrid)
{
  if (m_stackGrid.empty()) {
    m_stackGrid.push_back(stackGrid);
  } else if (m_stackGrid[0] != stackGrid) {
    delete m_stackGrid[0];
    deprecateDependent(GREY_SCALE);
    m_stackGrid[0] = stackGrid;
  }
}

void ZSparseStack::setGreyScale(int zoom, ZStackBlockGrid *stackGrid)
{
  if (int(m_stackGrid.size()) <= zoom) {
    m_stackGrid.resize(zoom + 1);
  }

  delete m_stackGrid[zoom];
  m_stackGrid[zoom] = stackGrid;
  deprecateDependent(GREY_SCALE);
}

const ZStackBlockGrid* ZSparseStack::getStackGrid() const
{
  if (m_stackGrid.empty()) {
    return nullptr;
  }

  return m_stackGrid.at(0);
}

ZStackBlockGrid* ZSparseStack::getStackGrid()
{
  return const_cast<ZStackBlockGrid*>(
        static_cast<const ZSparseStack&>(*this).getStackGrid());
}

const ZStackBlockGrid* ZSparseStack::getStackGrid(int zoom) const
{
  if (int(m_stackGrid.size()) <= zoom) {
    return nullptr;
  }

  return m_stackGrid.at(zoom);
}

ZStackBlockGrid* ZSparseStack::getStackGrid(int zoom)
{
  return const_cast<ZStackBlockGrid*>(
        static_cast<const ZSparseStack&>(*this).getStackGrid(zoom));
}

void ZSparseStack::setObjectMask(ZObject3dScan *obj)
{
  if (m_objectMask != obj) {
    deprecate(OBJECT_MASK);
    m_objectMask = obj;
    /*
    if (obj != NULL) {
      obj->setColor(255, 255, 255, 255);
    }
    */
  }
}

void ZSparseStack::merge(ZSparseStack &sparseStack)
{
  if (!sparseStack.isEmpty()) {
    m_objectMask->unify(*(sparseStack.getObjectMask()));
    getStackGrid()->consume(sparseStack.m_stackGrid[0]);
    deprecateDependent(GREY_SCALE);
    sparseStack.deprecateDependent(GREY_SCALE);
  }
}

ZIntCuboid ZSparseStack::getBoundBox() const
{
  ZIntCuboid box;
  if (m_objectMask != NULL) {
    box = m_objectMask->getBoundBox();
  }

  return box;
}

bool ZSparseStack::isEmpty() const
{
  if (m_objectMask != NULL) {
    return m_objectMask->isEmpty();
  }

  return true;
}

void ZSparseStack::read(std::istream &stream)
{
  deprecate(ALL_COMPONET);
  m_objectMask = new ZObject3dScan;
  m_objectMask->read(stream);
  ZStackBlockGrid *stackGrid = new ZStackBlockGrid;
  stackGrid->read(stream);
  setGreyScale(stackGrid);
}

void ZSparseStack::write(std::ostream &stream) const
{
  if (m_objectMask != NULL) {
    if (stream.good()) {
      m_objectMask->write(stream);
      if (getStackGrid()) {
        getStackGrid()->write(stream);
      }
    }
  }
}

bool ZSparseStack::save(const std::string &filePath) const
{
  std::ofstream stream(filePath.c_str(), std::ios_base::binary);
  if (stream.good()) {
    write(stream);
    return true;
  }

  return false;
}

bool ZSparseStack::load(const std::string &filePath)
{
  std::ifstream stream(filePath.c_str(), std::ios_base::binary);
  if (stream.good()) {
    read(stream);
    return true;
  }

  return false;
}

ZSparseStack* ZSparseStack::downsample(int xintv, int yintv, int zintv)
{
  ZSparseStack *stack = new ZSparseStack;
  stack->m_objectMask = new ZObject3dScan(*m_objectMask);
  stack->m_objectMask->downsampleMax(xintv, yintv, zintv);
  stack->setGreyScale(makeDownsampleGrid(xintv, yintv, zintv));
  stack->setDsIntv(getDsIntv());
  stack->pushDsIntv(xintv, yintv, zintv);

  return stack;
}

void ZSparseStack::printInfo() const
{
  if (isEmpty()) {
    std::cout << "Empty sparse stack." << std::endl;
  } else {
    if (m_objectMask != NULL) {
      m_objectMask->printInfo();
    }
  }
}
