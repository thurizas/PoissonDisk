#ifndef _common_h_
#define _common_h_

#include <cmath>
#include <cstdint>
#include <map>
#include <optional>

static const float_t   PI = 3.14159265358979f;

// default values for user entered parameters....
static const uint16_t defWidth = 800;
static const uint16_t defHeight = 600;
static const double_t defMinRadius = 21;
static const int32_t  defCntPoints = -1;
static const uint32_t defTrials = 15;

// values for version information....
static const uint16_t MAJOR = 1;
static const uint16_t MINOR = 0;
static const uint16_t PATCH = 0;

// state machine values
static const uint16_t STATE_RUN   = 0b0000'0000'0000'0010;       // _r_un
static const uint16_t STATE_STOP  = 0b0000'0000'1111'1101;       // _s_top, so state &= STATE_STOP unsets run bit
static const uint16_t STATE_PAUSE = 0b0000'0000'0000'0100;       // _p_ause
static const uint16_t STATE_CLEAR = 0b0000'0000'0000'1000;       // _c_lear
static const uint16_t STATE_INC   = 0b0000'0000'0001'0000;       // _+_: increase speed of simulation
static const uint16_t STATE_DEC   = 0b0000'0000'0010'0000;       // _-_: decrease speed of simulation
static const uint16_t STATE_GRID  = 0b0000'0000'0100'0000;       // _g_rid toggle
static const uint16_t STATE_CIRCLE= 0b0000'0001'0000'0000;       // _b_ounding circle toggle
static const uint16_t STATE_DONE  = 0b0000'0000'1000'0000;       // generation is complete

// typedef's and custom structures

typedef std::pair<float_t, float_t> rangeT;

typedef struct _ctx
{
  float_t            minDist;         // minimum distance between points
  rangeT             xrange;          // x coordinate range
  rangeT             yrange;          // y coordinate range
  rangeT             cellSize;        // cell size (width, height) - normally a square cell
  uint32_t           mCol;            // maximum number of columns
  uint32_t           mRow;            // maximum number of row
  int32_t            cnt;             // number of points to generate
  std::optional<int> seed;            // optional value of the PRNG seed
} ctxT, *pctxT;

#endif
