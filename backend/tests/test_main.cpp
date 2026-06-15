#include "test_harness.h"

int main() {
    return crossbow::test::TestHarness::instance().run_all();
}
