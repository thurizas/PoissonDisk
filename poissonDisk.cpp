#include "poissonDisk.h"
#include "logger.h"

#include <random>
#include <optional>

extern const uint8_t colorCmdOut;

auto inRange = [](float_t val, rangeT range)->bool {if ((range.first < val) && (val < range.second)) return true; else return false; };

poissonDisk::poissonDisk(ctxT ctx) : m_cntPts(ctx.cnt), m_maxCol(ctx.mCol), m_maxRow(ctx.mRow), m_minRadius(ctx.minDist), m_Xrange(ctx.xrange), m_Yrange(ctx.yrange), m_cellDim(ctx.cellSize)
{
  try
  {
    std::random_device rd;

    // set seed for mersenne twister
    if (ctx.seed.has_value()) { m_seed = ctx.seed.value(); }
    else { m_seed = rd(); }
    m_pGen = new std::mt19937(m_seed);

    m_pXDist = new std::uniform_real_distribution<float_t>(0.0, m_Xrange.second - m_Xrange.first);  // random real in range [0, maxX)  
    m_pYDist = new std::uniform_real_distribution<float_t>(0.0, m_Yrange.second - m_Yrange.first);  // random real in range [0, maxY)

    m_grid.reserve(m_maxRow * m_maxCol);
    for (uint32_t ndx = 0; ndx < m_maxRow * m_maxCol; ndx++)
      m_grid.push_back(gridEntryT{.isOccupied = false, .Xcoord=0, .Ycoord = 0});
  }
  catch (std::bad_alloc& exc)
  {
    CLogger::getInstance()->outMsg(colorCmdOut, CLogger::level::ERR, "failed to allocate memory for distibutions");
    throw exc;
  }

  initSet();
}

poissonDisk::~poissonDisk()
{
  if (m_pXDist != nullptr) delete m_pXDist;
  if (m_pYDist != nullptr) delete m_pYDist;
  if (m_pGen != nullptr) delete m_pGen;
}

void poissonDisk::initSet()
{
  bool    member;
  float_t x;
  float_t y;
  
  do                                          // generate initial point
  {
    x = (*m_pXDist)(*m_pGen); 
    y = (*m_pYDist)(*m_pGen);

    member = inRange(x, m_Xrange);
    member &= inRange(y, m_Yrange);
  } while (!member);


  uint32_t gridNdx = genIndex(x, y);

  CLogger::getInstance()->outMsg(colorCmdOut, CLogger::level::DEBUG, "initial point is at (%4.f, %4.f), index is: %d", x, y, gridNdx);
  m_ptSet.push_back(std::pair<float_t, float_t>(x, y));
  m_workingSet.push_back(std::pair<float_t, float_t>(x, y));
  m_grid.at(gridNdx) = gridEntryT{ .isOccupied = true, .Xcoord = x, .Ycoord = y };
}

bool poissonDisk::calcNextPt(uint32_t attempts)
{    
  
  std::uniform_real_distribution<float_t> radiusDist(m_minRadius, 2 * m_minRadius);  // distribution for radius [minR, 2*minR)
  std::uniform_real_distribution<float_t> angleDist(0, 2 * PI);                      // distribution for angle [0, 2\pi)
  bool     ptAcceptable = false;
  int32_t  trials = attempts;
  uint32_t rootPtNdx;
  uint32_t newNdx;
  float_t  x;
  float_t  y;

  if((m_ptSet.size() < m_cntPts) && (!m_workingSet.empty()))           // check to see if we generated the correct number of points
  { 
    uint32_t max = static_cast<uint32_t>(m_workingSet.size());         // pick a random point from the working set
    std::uniform_int_distribution<uint32_t> dist(0, max - 1);

    rootPtNdx = dist(*m_pGen);
    std::pair<float_t, float_t> rootPt = m_workingSet.at(rootPtNdx);

    do
    {
      do
      {
        x = rootPt.first + radiusDist(*m_pGen) * cos(angleDist(*m_pGen));                // candidate point coordinates 
        y = rootPt.second + radiusDist(*m_pGen) * sin(angleDist(*m_pGen));

        ptAcceptable = inRange(x, m_Xrange);
        ptAcceptable &= inRange(y, m_Yrange);
      } while (!ptAcceptable);

      newNdx = genIndex(x, y);                                  // determine grid cell candidate point is in
      CLogger::getInstance()->outMsg(colorCmdOut, CLogger::level::INFO, "candidate cell as (%.4f, %.4f), index: %d", x, y, newNdx);
      if (!m_grid.at(newNdx).isOccupied)                        // only one point per cell
      {
        int32_t col = static_cast<int32_t>(floor(x / m_cellDim.second));
        int32_t row = static_cast<int32_t>(floor(y / m_cellDim.first));
        ptAcceptable = true;
        
        for (int32_t r = row - 2; r <= row + 2; r++)
        {
          if ((r < 0) || (r > m_maxRow-1))                 // insure we did not step outside of grid
            continue;
          for (int32_t c = col - 2; c <= col + 2; c++)
          {
            
            if ((c < 0) || c > m_maxCol-1)                 // insure we did not step outside of grid
              continue;

            uint32_t testNdx = r * m_maxCol + c;
            CLogger::getInstance()->outMsg(colorCmdOut, CLogger::level::INFO, "checking cell at r: %d, c: %d, ndx: %d", r, c, testNdx);
            
            if (newNdx == testNdx)                           // looking at the cell the candidate point is in
            {
              CLogger::getInstance()->outMsg(colorCmdOut, CLogger::level::INFO, ".... cell with new point");
              continue;
            }
            else if (!m_grid.at(testNdx).isOccupied)         // grid cell is empty, nothing to do
            {
              CLogger::getInstance()->outMsg(colorCmdOut, CLogger::level::INFO, ".... cell is empty");
              continue;
            }
            else                                             // grid cell is occupied
            {
              float_t d = sqrt((m_grid.at(testNdx).Xcoord - x) * (m_grid.at(testNdx).Xcoord - x) +
                               (m_grid.at(testNdx).Ycoord - y) * (m_grid.at(testNdx).Ycoord - y));
              if (d < m_minRadius) ptAcceptable = false;
              CLogger::getInstance()->outMsg(colorCmdOut, CLogger::level::INFO, ".... cell occupied, distance: %.4f", d);
            }
          }
        }
          
        if(ptAcceptable) break;                              // we have a good point, let go
      }
      else                                                   // candidate point would be going into an occupied cell
      {
        continue;
      }

      trials--;
    } while (trials >= 0);

    if (trials >= 0)                                         // found a valid point
    {
      m_ptSet.push_back(std::pair<float_t, float_t>(x, y));
      m_workingSet.push_back(std::pair<float_t, float_t>(x, y));
      m_grid.at(newNdx) = gridEntryT{ .isOccupied = true, .Xcoord = x, .Ycoord = y };
      CLogger::getInstance()->outMsg(colorCmdOut, CLogger::level::DEBUG, "next point is at (%4.f, %4.f), index is: %d", x, y, newNdx);
    }
    else
    {
      CLogger::getInstance()->outMsg(colorCmdOut, CLogger::level::DEBUG, "removing point from working set, %d points left", m_workingSet.size());
      m_workingSet[rootPtNdx] = m_workingSet.back();
      m_workingSet.pop_back();
    }
  }
  else
  {
    CLogger::getInstance()->outMsg(colorCmdOut, CLogger::level::DEBUG, "finished, generated %d points", m_ptSet.size());
    if ((m_workingSet.size() == 0) && (m_ptSet.size() != m_cntPts))
    {
      CLogger::getInstance()->outMsg(colorCmdOut, CLogger::level::WARNING, "finished prior to generating requested points");
      CLogger::getInstance()->outMsg(colorCmdOut, CLogger::level::WARNING, "try increasing number of trials per point");
    }
    return false;
  }

  return true;
}

uint32_t poissonDisk::genIndex(float_t x, float_t y)
{
  float_t col = floor(x / m_cellDim.second);
  float_t row = floor(y / m_cellDim.first);

  return static_cast<uint32_t>(row * m_maxCol + col);
}

void poissonDisk::restart()
{
  // TODO : delete m_ptSet
  // TODO : delete m_workingSet
  // TODO : delete grid

  initSet();
}
