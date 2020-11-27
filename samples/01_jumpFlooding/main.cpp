
#include "app.h"

#if ! __has_feature(objc_arc)
#error "ARC is off"
#endif


int main(int argc, char *args[]) {
  JumpFloodApplication sand;
  sand.run();
  return 0;
}

