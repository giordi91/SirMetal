#define CATCH_CONFIG_RUNNER
#include <catch/catch.h>

int main(int argc, char *argv[]) {
  // global setup...


  int result = Catch::Session().run(argc, argv);

  // global clean-up...

  return result;
}
