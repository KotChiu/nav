#include <cassert>
#include "../boards/21/MotoNav/ui/NavigationCueModel.h"
int main() {
  assert(motonavCueKind("S")==MotoNavCueKind::Straight);
  assert(motonavCueKind("L")==MotoNavCueKind::Left);
  assert(motonavCueKind("R")==MotoNavCueKind::Right);
  assert(motonavCueKind("SL")==MotoNavCueKind::SlightLeft);
  assert(motonavCueKind("SR")==MotoNavCueKind::SlightRight);
  assert(motonavCueKind("U")==MotoNavCueKind::UTurn);
  assert(motonavCueKind("A")==MotoNavCueKind::Arrive);
  assert(motonavCueKind("unknown")==MotoNavCueKind::Unknown);
  assert(!motonavCueSupported(motonavCueKind(nullptr)));
  assert(!motonavCueSupported(motonavCueKind("")));
  assert(motonavCueDefaultAngle(MotoNavCueKind::Left)==-90.f);
  assert(motonavCueDefaultAngle(MotoNavCueKind::Right)==90.f);
  assert(motonavCueDefaultAngle(MotoNavCueKind::SlightRight)==45.f);
}
