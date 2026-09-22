#include "SFML/Graphics.hpp"

#include "common.h"
#include "XGetopt.h"
#include "logger.h"

#include <iostream>
#include <algorithm>
#include <optional>
#include <vector>
#include <charconv>              // needed for std::from_chars
#include <string_view>
#include <fstream>

#include "poissonDisk.h"

static void showVersion(const char*);
static void showHelp(const char*);
static bool isInteger(const char*);
static bool isDouble(std::string_view);

extern const uint8_t colorCmdOut = 0;


int main(int argc, char** argv)
{
  bool     fontAvailable = true;
  int8_t   choice = 1;
  uint16_t winWidth = defWidth;
  uint16_t winHeight = defHeight;
  uint32_t cntPoints = defCntPoints;
  uint32_t trials = defTrials;
  std::optional<int> seed;
  float_t  minRadius = defMinRadius;
  uint8_t  state = STATE_RUN | STATE_GRID;
  int debugLvl = CLogger::level::WARNING;
  float_t timeStep = 0.5f;                   // 500 msec per simulation step

  sf::Color lightGray(43, 43, 43);

  while (-1 != (choice = getopt(argc, argv, "w:h:r:n:t:s:dvH")))
  {
    switch (choice)
    {
    case 'w':
      if (isInteger(optarg))
      {
        winWidth = atoi(optarg);
        if (winWidth <= 0) { std::cout << "width can not be zero or less, using default width" << std::endl; winWidth = defWidth; }
        if (winWidth > 65535) { std::cout << "width can not be greater than 65,535 - using default height" << std::endl; winWidth = defWidth; }
      }
      else
      {
        std::cout << "Argument to -w needs to be an integer, using default value" << std::endl;
      }
      break;

    case 'h':
      if (isInteger(optarg))
      {
        winHeight = atoi(optarg);
        if (winHeight <= 0) { std::cout << "height can not be zero or less, using default width" << std::endl; winHeight = defWidth; }
        if (winHeight > 65535) { std::cout << "height can not be greater than 65,535 - using default height" << std::endl; winHeight = defWidth; }
      }
      else
      {
        std::cout << "Argument to -h needs to be an integer, using default value" << std::endl;
      }
      break;

    case 'r':
      if (isDouble(std::string_view(optarg)))
      {
        minRadius = static_cast<float_t>(atof(optarg));
        if (minRadius < 0) { std::cout << "minimum radius can not be zero or less, using default minimum radius" << std::endl; minRadius = defMinRadius; }
      }
      else
      {
        std::cout << "argument ot -r must be a non-negative real number" << std::endl;
      }
      break;

    case 'n':
      if (isInteger(optarg))
      {
        cntPoints = atoi(optarg);
        if (cntPoints <= 0) { std::cout << "number of points can be zero or less, using default number of points" << std::endl; cntPoints = defCntPoints; }
      }
      break;

    case 't':
      if (isInteger(optarg))
      {
        trials = atoi(optarg);
      }
      else
      {
        std::cout << "argument to -t must be an integer, using default value of 15" << std::endl;
      }
      break;

    case 's':
      if (isInteger(optarg))
      {
        seed.emplace(atoi(optarg));
      }
      else
      {
        std::cout << "argument to -s must be an integer, using random value for the PRNG seed" << std::endl;
      }

    case 'd':
      debugLvl--;  
      if (debugLvl < 0) debugLvl = 0;
      break;

    case 'v':
      showVersion(argv[0]);
      exit(0);

    case '?':
      std::cout << "Unknown command line option: " << argv[optind] << std::endl;
      [[fallthrough]];
    case 'H':
      showHelp(argv[0]);
      exit(0);
    }
  }

  CLogger* pLogger = CLogger::getInstance();
  pLogger->regOutDevice(colorCmdOut, cmdColorOut);
  pLogger->setLevel(debugLvl);
  pLogger->outMsg(colorCmdOut, CLogger::level::NOTICE, "logging engine started.");

  // create a grid that fits in the window dimensions, grid cell is minRadius/sqrt(2)
  float_t cellSize = static_cast<float_t>(minRadius / sqrt(2));
  
  uint32_t nbrCellHorizontal = static_cast<uint32_t>(floor(static_cast<double_t>(winWidth) / cellSize));
  uint32_t nbrCellVertical = static_cast<uint32_t>(floor(static_cast<double_t>(winHeight) / cellSize));
  
  // point set size not given by user, set to one point per cell
  if(cntPoints == defCntPoints) cntPoints = nbrCellHorizontal * nbrCellVertical;      

  float_t gridWidth = nbrCellHorizontal * cellSize;
  float_t gridHeight = nbrCellVertical * cellSize;

  float_t xsWidth = 0;   // winWidth - gridWidth;                         // NOTE: uncomment the calculation too have grid centered
  float_t xsHeight = 0;  // winHeight - gridHeight;

  // construct simulation context...
  ctxT ctx{ .minDist = minRadius, .xrange = rangeT(xsWidth / 2, xsWidth / 2 + gridWidth), .yrange = rangeT(xsHeight / 2, xsHeight / 2 + gridHeight),
            .cellSize = rangeT(cellSize, cellSize), .mCol = nbrCellHorizontal, .mRow = nbrCellVertical, .cnt = cntPoints, .seed = seed };

  pLogger->outMsg(colorCmdOut, CLogger::level::DEBUG, "Cell size is %4f.", cellSize);
  pLogger->outMsg(colorCmdOut, CLogger::level::DEBUG, "Grid size is %d rows and %d columns", nbrCellVertical, nbrCellHorizontal);
  pLogger->outMsg(colorCmdOut, CLogger::level::DEBUG, "top-left point is at (%4.f, %4.f)", xsWidth / 2, xsHeight / 2);
  pLogger->outMsg(colorCmdOut, CLogger::level::DEBUG, "bottom-right point is at (%4.f,%4.f)", xsWidth / 2 + gridWidth, xsHeight / 2 + gridHeight);

  std::vector<sf::Vertex> lines;
  std::vector<sf::Vertex> centers;

  // construct verticle lines.
  for (uint32_t ndx = 0; ndx <= nbrCellHorizontal; ndx++)
  {
    lines.push_back(sf::Vertex(sf::Vector2f((xsWidth / 2) + ndx * cellSize, (xsHeight / 2)), lightGray));
    lines.push_back(sf::Vertex(sf::Vector2f((xsWidth / 2) + ndx * cellSize, (xsHeight / 2) + gridHeight), lightGray));
  }

  // construct horizontal lines.
  for (uint32_t ndx = 0; ndx <= nbrCellVertical; ndx++)
  {
    lines.push_back(sf::Vertex(sf::Vector2f(xsWidth / 2, (xsHeight / 2) + ndx * cellSize), lightGray));
    lines.push_back(sf::Vertex(sf::Vector2f((xsWidth / 2) + gridWidth, (xsHeight / 2) + ndx * cellSize), lightGray));
  }

  // calculate center of cells
  for (float_t center_y = cellSize / 2; center_y < gridHeight; center_y += cellSize)
  {
    for (float_t center_x = cellSize / 2; center_x < gridWidth; center_x += cellSize)
    {
      centers.push_back(sf::Vertex(sf::Vector2(center_x, center_y), sf::Color::Red));
    }
  }


  try
  {
    poissonDisk  disk(ctx);

    {
      std::cout << "*****************************************************************************************************" << std::endl;
      std::cout << "*** Starting Poisson Disk Sampling example" << std::endl;
      std::cout << "***\n*** Generating " << cntPoints << " with a minimum separation of " << minRadius << std::endl;
      std::cout << "*** using a " << cellSize << "x " << cellSize << " in auxillary gird" << std::endl;
      std::cout << "*** auxillary grid is " << nbrCellVertical << " rows by " << nbrCellHorizontal << " columns" << std::endl;
      std::cout << "*** using " << disk.getPRNGSeed() << " as seed for the random number generator" << std::endl;
      std::cout << "*****************************************************************************************************" << std::endl;
    }

    sf::Clock   deltaClock;
    sf::Time    lastUpdate = sf::Time::Zero;

    // create font for use in rendering text 
    sf::Font font;
    if (!font.loadFromFile(R"(C:\Windows\Fonts\arial.ttf)")) 
    {
      pLogger->outMsg(colorCmdOut, CLogger::level::ERR, "SFML internal loader failed entirely (Check Debug/Release libs)");
      fontAvailable = false;
    }

    sf::RenderWindow window(sf::VideoMode({ winWidth,winHeight }), "Poisson Disk Example");
    if (window.isOpen())
    {
      while (window.isOpen() && ((state & STATE_RUN) || (state & STATE_PAUSE) || state & STATE_DONE))
      {
        sf::Event event;
        while (window.pollEvent(event))
        {
          if (event.type == sf::Event::Closed)
          {
            window.close();
          }

          if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
          {
            window.close();                                // _esc_ -- terminate the application
          }
          if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::G)
          {
            state ^= STATE_GRID;                          // _g_rid -- toggle grid on/off
          }
          if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Equal)
          {
            timeStep /= 2.0;
            if (timeStep < 0.001) timeStep = 0.001f;      // _+_ -- speed up simulation
          }
          if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Dash)
          {
            timeStep *= 2.0;
            if (2 < timeStep) timeStep = 2.f;            // ___ -- slow down the simulation
          }
          if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::R)
          {
            state |= STATE_RUN;                               // set the run bit
            if (state & STATE_PAUSE) state &= ~STATE_PAUSE;   // clear the pause bit, if set
            if (state & STATE_STOP) state &= ~STATE_STOP;     // clear the stop bit, if set
          }
          if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::S)
          {
            state &= STATE_STOP;                              // clear run bit if set
          }
          if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::P)
          {
            state |= STATE_PAUSE;
            if (state & STATE_RUN) state &= ~STATE_RUN;       // clear the run bit, if set
          }
          if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::C)
          {
            disk.restart();                             // clear/restart the simulation
          }
        }

        const sf::Time TIME_PER_STEP = sf::seconds(timeStep);            // about 500 msec per iteration
        sf::Time dt = deltaClock.restart();
        if (dt > sf::seconds(0.25f))
        {
          dt = sf::seconds(0.25f);
        }

        lastUpdate += dt;

        // render scene...
        if ((state & STATE_RUN) == STATE_RUN)
        {
          while (lastUpdate >= TIME_PER_STEP)
          {
            lastUpdate -= TIME_PER_STEP;
            if (!disk.calcNextPt(trials))
            {
              state = STATE_DONE;
            }
          }

          window.clear(sf::Color::Black);
          if (state & STATE_GRID)
          {
            window.draw(&lines[0], lines.size(), sf::PrimitiveType::Lines);

            for (uint32_t ndx = 0; ndx < centers.size(); ndx++)
            {
              window.draw(&centers.at(ndx), 1, sf::PrimitiveType::Points);
              if (fontAvailable == true)
              {
                char  msg[5];
                memset((void*)msg, '\0', 5);
                snprintf(msg, 4, "%3d", ndx);
                sf::Text text(msg, font, 10);
                text.setFillColor(sf::Color::Red);
                text.setPosition(centers.at(ndx).position);  // set upper left corner
                window.draw(text);
              }

            }

          }

          for (auto pt : disk.getPtSet())
          {
            sf::Vertex point({ pt.first, pt.second }, sf::Color::Green);
            window.draw(&point, 1, sf::PrimitiveType::Points);
          }
          window.display();
        }
        else if ((state & STATE_DONE) == STATE_DONE)
        {
          static bool once = true;

          if (once)
          {
            std::cout << "Generation is complete, the point set is: " << std::endl;
            std::vector<std::pair<float_t, float_t>> ptSet = disk.getPtSet();
            uint32_t cnt = 1;
            for (std::pair<float_t, float_t>& pt : ptSet)
            {
              std::cout << "(" << pt.first << ", " << pt.second << ")";
              if ((cnt != 0) && (cnt % 8 == 0))
                std::cout << std::endl;
              else
                std::cout << ", ";

              cnt++;
            }
            once = false;
          }
        }
      } 
    }
    else
    {
      pLogger->outMsg(colorCmdOut, CLogger::level::FATAL, "SFML window creation failed");
    }
  }
  catch ([[maybe_unused]] std::bad_alloc& exc)
  {
    std::cerr << "Out of memory, fatal error - exiting" << std::endl;
  }


  pLogger->outMsg(colorCmdOut, CLogger::level::NOTICE, "logging engine shuting down");
  pLogger->delInstance();
  return 0;
}



static void showVersion(const char* name)
{
  std::cout << name << " a poisson disk sampling example " << std::endl;
  std::cout << "version: " << MAJOR << "." << MINOR << "." << PATCH << std::endl;
}



static void showHelp(const char* name)
{
  std::cout << name << " a poisson disk sampling example " << std::endl;
  std::cout << "usage: " << name << " options" << std::endl;
  std::cout << "\nOptions:                                                         " << std::endl;
  std::cout << " w num                     use number as the width of the window (default is 800)" << std::endl;
  std::cout << " h num                     use number as the height of the window (default is 600)" << std::endl;
  std::cout << " r num                     use number as the minimum distance between points (default is 10)" << std::endl;
  std::cout << " t num                     number of failed trials before removing a point from working set " << std::endl;
  std::cout << " n num                     use number as the number of points to generate (default is 100)  " << std::endl;
  std::cout << " d                         increase the verbosity of the logging message                    " << std::endl;
  std::cout << " v                         display program version                                          " << std::endl;
  std::cout << " H                         displays this help screen                                        " << std::endl;
  std::cout << "\n\nKeyboard commands while the simulation is running:\n                                    " << std::endl;
  std::cout << "escape key                 exits the simulation                                             " << std::endl;
  std::cout << "g                          toggles grid on/off                                              " << std::endl;
  std::cout << "+                          increase the speed of the simulation                             " << std::endl;
  std::cout << "-                          decrease the speed of the simulation                             " << std::endl;
  std::cout << "r                          run the simulation                                               " << std::endl;
  std::cout << "s                          stop the simulation                                              " << std::endl;
  std::cout << "p                          pause the simulation                                             " << std::endl;
  std::cout << "c                          clears the simulation data and restart the simulation            " << std::endl;
}

bool isInteger(const char* val)
{
  bool res = false;
  const char* ep = val + strlen(val);

  res = std::find_if(val, ep, [](const char ch)->bool { return !isdigit(ch); });

  return res;
}

bool isDouble(std::string_view val)
{
  bool res = false;

  if (!val.empty())
  {
    double value;
    auto [ptr, ec] = std::from_chars(val.data(), val.data() + val.size(), value);

    res = (ec == std::errc{} && ptr == val.data() + val.size());
  }

  return res;
}
