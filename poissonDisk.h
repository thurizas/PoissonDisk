#ifndef _poissonDisk_h_
#define _poissonDisk_h_

#include "common.h"

//#include <map>
#include <vector>
#include <random>
#include <optional>

typedef struct _gridEntry
{
  bool        isOccupied = false;
  float_t     Xcoord;
  float_t     Ycoord;
} gridEntryT, *pgridEntryT;


class poissonDisk
{
public:
  poissonDisk() : m_minRadius(1.0) {}
  poissonDisk(ctxT);
  ~poissonDisk();

  uint32_t getPRNGSeed() { return m_seed; }
  void restart();

  std::vector<std::pair<float_t, float_t>> getPtSet() { return m_ptSet; }

  bool calcNextPt(uint32_t);

private:
  void      initSet();
  uint32_t  genIndex(float_t, float_t);

  uint32_t                                 m_cntPts;     // number of points to generate
  uint32_t                                 m_maxCol;     // number of columns in a row
  uint32_t                                 m_maxRow;
  uint32_t                                 m_seed;       // PRNG seed
  float_t                                  m_minRadius;  // minimum separation between points
  std::mt19937*                            m_pGen;       // generator for various distributions
  std::uniform_real_distribution<float_t>* m_pXDist;
  std::uniform_real_distribution<float_t>* m_pYDist;
  std::pair<float_t, float_t>              m_Xrange;     // range of x coordinate
  std::pair<float_t, float_t>              m_Yrange;     // range of y coordinate
  std::pair<float_t, float_t>              m_cellDim;    // cell size

  std::vector<std::pair<float_t, float_t>>     m_ptSet;
  std::vector<std::pair<float_t, float_t>>     m_workingSet;
  std::vector<gridEntryT>                      m_grid;
};

#endif

