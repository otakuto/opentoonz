

#ifdef _DEBUG
#define _STLP_DEBUG 1
#endif
#include "DeformationSelector.h"

using namespace ToonzExt;

//-----------------------------------------------------------------------------

DeformationSelector::DeformationSelector() {}

//-----------------------------------------------------------------------------

DeformationSelector::~DeformationSelector() {}

//-----------------------------------------------------------------------------

DeformationSelector *DeformationSelector::instance() {
  static DeformationSelector singleton;
  return &singleton;
}

//-----------------------------------------------------------------------------

void DeformationSelector::add(StrokeDeformationImpl *deformation,
                              int priority) {
  assert(deformation && "Not deformation available!!!");
  if (!deformation) return;
  Deformation val(deformation, priority);
  ref_.push_back(val);
}

//-----------------------------------------------------------------------------

StrokeDeformationImpl *DeformationSelector::getDeformation(
    const ContextStatus *status) {
  assert(status && "Not status available!!!");
  if (!status) return 0;

  Deformation candidate((StrokeDeformationImpl *)0, -1);

  // by default lower deformator is selected
  Deformation default_def((StrokeDeformationImpl *)0, -1);
  if (!ref_.empty()) default_def = ref_[0];

  for (auto const &e : ref_) {
    StrokeDeformationImpl *tmp = e.first;

    if (tmp->check(status)) {
      if (e.second > candidate.second) {
        candidate = e;
        if (e.second < default_def.second) {
          default_def = e;
        }
      }
    }
    if (tmp->getShortcutKey() == status->key_event_) {
      return tmp;
    }
  }

  // if there is a good candidate return it
  if (candidate.first) return candidate.first;

  // else select a lower deformator (smooth)
  return default_def.first;
}

//-----------------------------------------------------------------------------
//  End Of File
//-----------------------------------------------------------------------------
