

#include "tstrokeoutline.h"
#include "tcurves.h"

//-----------------------------------------------------------------------------

TStrokeOutline::TStrokeOutline(const TStrokeOutline &stroke)
    : m_outline(stroke.m_outline) {}

//-----------------------------------------------------------------------------

TStrokeOutline &TStrokeOutline::operator=(const TStrokeOutline &stroke) {
  TStrokeOutline tmp(stroke);
  tmp.m_outline.swap(m_outline);
  return *this;
}

//-----------------------------------------------------------------------------

void TStrokeOutline::addOutlinePoint(const TOutlinePoint &p) {
  m_outline.push_back(p);
}

//-----------------------------------------------------------------------------

std::vector<TQuadratic> getOutlineWithQuadratic(const TStroke &s) {
  return std::vector<TQuadratic>();
}

//-----------------------------------------------------------------------------
//  End Of File
//-----------------------------------------------------------------------------
