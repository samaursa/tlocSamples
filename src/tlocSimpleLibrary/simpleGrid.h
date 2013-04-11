#ifndef _TLOC_SIMPLE_GRID_H_
#define _TLOC_SIMPLE_GRID_H_

#include <tlocCore/tloc_core.h>

class Cell
{
public:
  TLOC_DECL_AND_DEF_SETTER(tloc::tl_int, SetData, m_data);
  TLOC_DECL_AND_DEF_GETTER(tloc::tl_int, GetData, m_data);

private:
  tloc::tl_int m_data;
};

class SimpleGrid
{
public:
  SimpleGrid(tloc::tl_size a_sizeX, tloc::tl_size a_sizeY);

  Cell GetCell(tloc::tl_size a_x, tloc::tl_size a_y);

private:
  tloc::core_conts::Array<
    tloc::core_conts::Array<Cell> > m_grid;
};

#endif