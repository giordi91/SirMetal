#include "SirMetal/core/memory/cpu/sparseMemoryPool.h"
#include "catch/catch.h"

struct DummyAlloc {
  uint32_t value = 0xBADDCAFE;
  uint32_t value2 = 0;
};
TEST_CASE("MemoryPool allocation", "[memory]") {

  SirMetal::SparseMemoryPool<DummyAlloc> pool(20);
  for (uint32_t i = 0; i < 10; ++i) {
    uint32_t idx;
    pool.getFreeMemoryData(idx);
    REQUIRE(idx == i);
    REQUIRE(pool.getAllocatedCount() == i + 1);
  }
}

TEST_CASE("MemoryPool deletion", "[memory]") {

  SirMetal::SparseMemoryPool<DummyAlloc> pool(20);
  for (uint32_t i = 0; i < 10; ++i) {
    uint32_t idx;
    pool.getFreeMemoryData(idx);
    REQUIRE(idx == i);
    REQUIRE(pool.getAllocatedCount() == i + 1);
  }
  for (uint32_t i = 0; i < 10; ++i) {
    pool.free(i);
    REQUIRE(pool.getAllocatedCount() == 10 - i - 1);
  }
  for (uint32_t i = 0; i < 10; ++i) {
    uint32_t idx;
    pool.getFreeMemoryData(idx);
    REQUIRE(pool.getAllocatedCount() == i+1);
  }
}

TEST_CASE("MemoryPool mixed deletion and addition", "[memory]") {

  SirMetal::SparseMemoryPool<DummyAlloc> pool(20);
  uint32_t indices[5]{3, 5, 1, 0, 7};
  for (uint32_t i = 0; i < 10; ++i) {
    uint32_t idx;
    pool.getFreeMemoryData(idx);
    REQUIRE(idx == i);
    REQUIRE(pool.getAllocatedCount() == i + 1);
  }
  for (uint32_t i = 0; i < 5; ++i) {
    pool.free(indices[i]);
    REQUIRE(pool.getAllocatedCount() == 10 - i - 1);
  }
  for (uint32_t i = 0; i < 5; ++i) {
    uint32_t idx;
    pool.getFreeMemoryData(idx);
    REQUIRE(idx == indices[5 - i - 1]);
  }
}
