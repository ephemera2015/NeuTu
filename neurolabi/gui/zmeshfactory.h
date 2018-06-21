#ifndef ZMESHFACTORY_H
#define ZMESHFACTORY_H

class ZMesh;
class ZObject3dScan;
class ZIntPoint;
class ZObject3dScanArray;

class ZMeshFactory
{
public:
  ZMeshFactory();

  ZMesh* makeMesh(const ZObject3dScan &obj);

  void setDsIntv(int dsIntv);
  void setSmooth(int smooth);

  static ZMesh* MakeMesh(const ZObject3dScan &obj);
  static ZMesh* MakeMesh(const ZObject3dScan &obj, int dsIntv, int smooth);
  static ZMesh* MakeMesh(const ZObject3dScanArray &objArray);
//  static ZMesh* MakeMesh(const ZObject3dScan &obj, const ZIntPoint &dsIntv, int smooth);

private:
  int m_dsIntv = 0;
  int m_smooth = 3;
};

#endif // ZMESHFACTORY_H
