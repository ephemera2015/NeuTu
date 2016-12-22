#ifndef ZCOMMANDLINE_H
#define ZCOMMANDLINE_H

#include <string>
#include <vector>
#include <set>

#include "zjsonobject.h"
#include "zmessagereporter.h"

class ZSwcTree;
class ZSwcTreeMatcher;
class ZStack;
class ZWeightedPoint;
class ZDvidReader;

class ZCommandLine
{
public:
  ZCommandLine();

  enum ECommand {
    OBJECT_MARKER, BOUNDARY_ORPHAN, OBJECT_OVERLAP,
    SYNAPSE_OBJECT, CLASS_LIST, FLYEM_NEURON_FEATURE,
    SKELETONIZE, SEPARATE_IMAGE, TRACE_NEURON, TEST_SELF,
    COMPARE_SWC, COMPUTE_SEED,
    UNKNOWN_COMMAND
  };

  int run(int argc, char *argv[]);


  friend class ZTest;

private:
  void init();

  static ECommand getCommand(const char *cmd);
  int runObjectMarker();
  int runBoundaryOrphan();
  int runObjectOverlap();
  int runSynapseObjectList();
  int runOutputClassList();
  int runComputeFlyEmNeuronFeature();
  int runSkeletonize();
  int runCompareSwc();
  int runImageSeparation();
  int runTraceNeuron();
  int runComputeSeed();
  int runTest();

  std::set<uint64_t> loadBodySet(const std::string &input) const;

  void loadConfig(const std::string &filePath);
  void expandConfig(const std::string &configFilePath, const std::string &key);
  std::string extractIncludePath(
      const std::string &configFilePath, const std::string &key);

  double compareSwc(
      ZSwcTree *tree1, ZSwcTree *tree2, ZSwcTreeMatcher &matcher) const;

private:
  ZStack* readDvidStack(const ZJsonObject &dvidConfig);
  void loadTraceConfig();
  static bool ExportPointArray(const std::vector<ZWeightedPoint> &ptArray,
                               const std::string &outFilePath);
  int skeletonizeDvid();
  int skeletonizeFile();
  std::vector<uint64_t> getSkeletonBodyList(ZDvidReader &reader) const;
  ZJsonObject getSkeletonizeConfig(ZDvidReader &reader);

private:
  std::vector<std::string> m_input;
  std::string m_output;
  std::string m_blockFile;
  std::string m_referenceBlockFile;
  std::string m_synapseFile;
  ZJsonObject m_configJson;
  std::string m_configDir;
  int m_ravelerHeight;
  int m_zStart;
  int m_intv[3];
  int m_blockOffset[3];
  int m_position[3];
  int m_size[3];
  int m_level;
  double m_scale;
  bool m_fullOverlapScreen;
  bool m_isVerbose;
  bool m_forceUpdate;
  bool m_namedOnly;
  std::vector<uint64_t> m_bodyIdArray;
  ZMessageReporter m_reporter;
};

#endif // ZCOMMANDLINE_H
