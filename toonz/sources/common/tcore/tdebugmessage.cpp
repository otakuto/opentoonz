

#include "tdebugmessage.h"
#include <iostream>

namespace {

TDebugMessage::Manager *debugManagerInstance = 0;
}

void TDebugMessage::setManager(Manager *manager) {
  debugManagerInstance = manager;
}

std::ostream &TDebugMessage::getStream() {
  if (debugManagerInstance)
    return debugManagerInstance->getStream();
  else
    return std::cout;
}

void TDebugMessage::flush(int code) {
  if (debugManagerInstance)
    debugManagerInstance->flush(code);
  else
    std::cout << std::endl;
}
