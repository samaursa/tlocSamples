#include "simpleGrid.h"

using namespace tloc;

SimpleGrid::SimpleGrid(tloc::tl_size a_sizeX, tloc::tl_size a_sizeY)
{
  m_grid.resize(a_sizeX);

  for (tl_size i = 0; i < a_sizeX; ++i)
  {
    m_grid[i].resize(a_sizeY);
  }

}

Cell SimpleGrid::GetCell(tloc::tl_size a_x, tloc::tl_size a_y)
{
  return m_grid[a_x][a_y];
}