#ifndef _TLOC_SIMPLE_GRID_H_
#define _TLOC_SIMPLE_GRID_H_

#include <tlocCore/tloc_core.h>

class Cell
{
public:
  TLOC_DECL_AND_DEF_SETTER(tl_int, SetData, m_data);
  TLOC_DECL_AND_DEF_GETTER(tl_int, GetData, m_data);

private:
  tl_int m_data;
};

class SimpleGrid
{
public:
  SimpleGrid(tl_size a_sizeX, tl_size a_sizeY);

  Cell GetCell(tl_size a_x, tl_size a_y);

private:
  tl_core_conts::Array<tl_core_conts::Array<Cell> > m_grid;
};

#endif