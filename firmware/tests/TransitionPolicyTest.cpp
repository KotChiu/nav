#include <cassert>
#include "../boards/21/MotoNav/ui/TransitionPolicy.h"

int main() {
  for (int from = 0; from < 6; ++from) {
    for (int to = 0; to < 6; ++to) {
      assert(motonavNativeSwipeAllowed(from, to, false, true) == (from != to));
#if MOTONAV_NATIVE_NAV_SLIDES == 0
      assert(!motonavNativeSwipeAllowed(from, to, true, true));
#else
      assert(motonavNativeSwipeAllowed(from, to, true, true) == (from != to));
#endif
    }
  }
  assert(!motonavNativeSwipeAllowed(-1, 4, false, true));
  assert(!motonavNativeSwipeAllowed(3, 6, false, true));
  assert(!motonavNativeSwipeAllowed(1, 2, false, false));
}
