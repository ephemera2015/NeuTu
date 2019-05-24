#include "flyemdatawriter.h"

#include "geometry/zintpoint.h"
#include "zobject3dscan.h"
#include "zjsonobject.h"
#include "zfiletype.h"

#include "neutubeconfig.h"

#include "dvid/zdvidwriter.h"
#include "dvid/zdvidurl.h"

#include "zflyembodyannotation.h"
#include "flyemdatareader.h"
#include "flyemdataconfig.h"

FlyEmDataWriter::FlyEmDataWriter()
{

}

void FlyEmDataWriter::UpdateBodyStatus(
    ZDvidWriter &writer, const ZIntPoint &pos, const std::string &newStatus)
{
  uint64_t bodyId = writer.getDvidReader().readBodyIdAt(pos);
  ZFlyEmBodyAnnotation annot =
      FlyEmDataReader::ReadBodyAnnotation(writer.getDvidReader(), bodyId);
#ifdef _DEBUG_
  std::cout << "Old annotation:" << std::endl;
  annot.print();
#endif
  annot.setStatus(newStatus);
  writer.writeBodyAnntation(annot);

#ifdef _DEBUG_
  ZFlyEmBodyAnnotation newAnnot =
      FlyEmDataReader::ReadBodyAnnotation(writer.getDvidReader(), bodyId);
  std::cout << "New annotation:" << std::endl;
  newAnnot.print();
#endif
}

void FlyEmDataWriter::RewriteBody(ZDvidWriter &writer, uint64_t bodyId)
{
  uint64_t newBodyId = 0;

  if (writer.good()) {
    ZObject3dScan obj;
    writer.getDvidReader().readBody(bodyId, false, &obj);

    if (!obj.isEmpty()) {
      newBodyId = writer.writeSplit(obj, bodyId, 0);
      //      std::cout << newBodyId << std::endl;

      if (newBodyId > 0) {
        ZFlyEmBodyAnnotation annotation =
            FlyEmDataReader::ReadBodyAnnotation(writer.getDvidReader(), bodyId);

        if (!annotation.isEmpty()) {
          writer.deleteBodyAnnotation(bodyId);
          annotation.setBodyId(newBodyId);
          writer.writeBodyAnntation(annotation);
        }
      }
    }
  }
}

void FlyEmDataWriter::UploadUserDataConfig(
    ZDvidWriter &writer, const FlyEmDataConfig &config)
{
  ZJsonObject obj;
  ZJsonObject contrastProtocol = config.getContrastProtocol().toJsonObject();
  obj.setEntry(FlyEmDataConfig::KEY_CONTRAST, contrastProtocol);
  std::string userName = NeutubeConfig::GetUserName();
  writer.writeJson(ZDvidData::GetName(ZDvidData::ERole::NEUTU_CONFIG),
                   "user_" + userName, obj);
}

void FlyEmDataWriter::UploadRoi(
    ZDvidWriter &writer, const std::string &name, const std::string &roiFile,
    const std::string &meshFile)
{
  if (!name.empty()) {
    if (!roiFile.empty()) {
      if (ZFileType::FileType(roiFile) == ZFileType::EFileType::OBJECT_SCAN) {
        ZObject3dScan roi;
        roi.load(roiFile);
        if (!writer.getDvidReader().hasData(name)) {
          writer.createData("roi", name);
        } else {
          writer.deleteData("roi", name);
        }
        std::cout << "Writing " << name << std::endl;
        writer.writeRoi(roi, name);
      } else {
        std::cout << "WARNING: Unexpected file roi file: " << roiFile << std::endl;
      }
    }

    if (!meshFile.empty()) {
      if (ZFileType::FileType(meshFile) == ZFileType::EFileType::MESH) {
        std::cout << "Writing mesh for " << name << std::endl;
        writer.uploadRoiMesh(meshFile, name);
      } else {
        std::cout << "WARNING: Unexpected file mesh file: " << meshFile << std::endl;
      }
    }
  }
}
