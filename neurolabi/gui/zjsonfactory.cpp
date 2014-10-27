#include "zjsonfactory.h"
#include "zjsonarray.h"
#include "zjsonobject.h"
#include "zobject3dscan.h"

ZJsonFactory::ZJsonFactory()
{
}

ZJsonArray ZJsonFactory::makeJsonArray(
    const ZObject3dScan &obj, ZJsonFactory::EObjectForm form)
{
  ZJsonArray array;

  ZObject3dScan::ConstSegmentIterator iterator(&obj);

  while (iterator.hasNext()) {
    const ZObject3dScan::Segment &seg = iterator.next();

    switch (form) {
    case ZJsonFactory::OBJECT_SPARSE:
    {
      ZJsonArray segmentObj;
      segmentObj.append(seg.getZ());
      segmentObj.append(seg.getY());
      segmentObj.append(seg.getStart());
      segmentObj.append(seg.getEnd());
      array.append(segmentObj);
    }
      break;
    case ZJsonFactory::OBJECT_DENSE:
    {
      for (int x = seg.getStart(); x <= seg.getEnd(); ++x) {
        ZJsonArray pointObj;
        pointObj.append(x);
        pointObj.append(seg.getY());
        pointObj.append(seg.getZ());
        array.append(pointObj);
      }
    }
      break;
    }
  }

  return array;
}
