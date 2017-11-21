#ifndef ZOBJECT3DFACTORY_H
#define ZOBJECT3DFACTORY_H

#include <vector>

#include "tz_cdefs.h"
#include "neutube_def.h"

class ZStack;
class ZObject3d;
class ZObject3dArray;
class ZObject3dScan;
class ZObject3dScanArray;
class ZClosedCurve;
class ZArray;
class ZIntCuboid;
class ZStroke2d;

class ZObject3dFactory
{
public:
  ZObject3dFactory();

public:
  enum EOutputForm {
    OUTPUT_COMPACT, OUTPUT_SPARSE
  };

  static ZObject3dArray* MakeRegionBoundary(
      const ZStack &stack, EOutputForm option);
  static ZObject3dScan MakeObject3dScan(const ZStack &stack);
  static ZObject3dScan* MakeObject3dScan(
      const ZStack &stack, ZObject3dScan *out);
  static ZObject3dScanArray* MakeObject3dScanArray(const ZStack &stack,
                                                   int yStep = 1);
  static ZObject3dScanArray* MakeObject3dScanArray(
      const ZStack &stack, NeuTube::EAxis sliceAxis);

  static std::vector<ZObject3dScan*> MakeObject3dScanPointerArray(
      const ZStack &stack, int yStep = 1, bool boundaryOnly = true);

  /*!
   * \brief Extract objects from a stack
   *
   * \param stack Input stack.
   * \param axis Scanning axis.
   * \param foreground Exclude background object if it is true.
   * \param out Output pointer if it is not NULL.
   * \return An array of objects.
   */
  static ZObject3dScanArray* MakeObject3dScanArray(
      const ZStack &stack, NeuTube::EAxis axis, bool foreground,
      ZObject3dScanArray *out);

  static ZObject3dScanArray* MakeObject3dScanArray(
      const ZArray &array, int yStep, ZObject3dScanArray *out,
      bool foreground);

  static ZObject3dScanArray* MakeObject3dScanArray(
      const ZArray &array, NeuTube::EAxis axis, bool foreground,
      ZObject3dScanArray *out);

  static ZObject3dScan* MakeFilledMask(const ZClosedCurve &curve, int z,
                                       ZObject3dScan *result = NULL);

  static ZObject3dScan MakeObject3dScan(const ZIntCuboid &box);

  static ZStack* MakeBoundaryStack(const ZStack &stack);
  /*!
   * \brief Extracts objects from a stack.
   *
   * \return an array of objects, each representing a region of one value.
   */
  static std::vector<ZObject3d*> MakeObject3dArray(const ZStack &stack);

  static ZObject3dScan MakeRandomObject3dScan(const ZIntCuboid &box);

  static ZObject3dScan MakeObject3dScan(const ZStroke2d &stroke);
};

#endif // ZOBJECT3DFACTORY_H
