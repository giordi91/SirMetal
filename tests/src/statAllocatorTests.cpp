#include "SirMetal/core/memory/cpu/stackAllocator.h"
#include "catch/catch.h"

TEST_CASE("StackAllocator simple allocation", "[memory]") {

  SirMetal::StackAllocator alloc;
  alloc.initialize(256);
  void *mem = alloc.allocate(16);
  REQUIRE(mem == alloc.getStartPtr());
  REQUIRE(alloc.getStackPtr() == (static_cast<char *>(mem) + 16));
}

TEST_CASE("StackAllocator simple multiple allocation", "[memory]") {

  SirMetal::StackAllocator alloc;
  alloc.initialize(256);
  for (int i = 0; i < 10; ++i) {
    void *mem = alloc.allocate(16);
    REQUIRE(mem == (static_cast<char *>(alloc.getStartPtr()) + 16 * i));
    REQUIRE(alloc.getStackPtr() == (static_cast<char *>(mem) + 16));
    REQUIRE(alloc.getStackPtr() ==
            (static_cast<char *>( alloc.getStartPtr()) + (16 * (i + 1))));
  }
}

TEST_CASE("StackAllocator simple free", "[memory]") {
  SirMetal::StackAllocator alloc;
  alloc.initialize(256);
  alloc.allocate(16);
  void* mem = alloc.free(16);
  REQUIRE(mem == alloc.getStartPtr());
}

TEST_CASE("StackAllocator  multiple alloc/free", "[memory]") {
  SirMetal::StackAllocator alloc;
  alloc.initialize(256);
  alloc.allocate(16);
  alloc.allocate(8);
  void* mem = alloc.free(16);
  REQUIRE(mem == (static_cast<char*>(alloc.getStartPtr())+8));
  REQUIRE(mem == (static_cast<char*>(alloc.getStartPtr())+8));
  mem = alloc.free(8);
  REQUIRE(mem == alloc.getStartPtr());
}
