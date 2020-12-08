
#include "sandbox.h"
#include <iostream>

#if ! __has_feature(objc_arc)
#error "ARC is off"
#endif


int main(int argc, char *args[]) {
  std::cout<<args[0]<<std::endl;
  Sandbox::SandboxApplication sand;
  sand.run();
  return 0;
}

