#include <cassert>
#include <cstring>
#include "../boards/21/MotoNav/CompanionFace.h"
int main() {
  using namespace CompanionFace;
  Expression expression = Calm;
  for (int i=0;i<Count;i++) {
    auto p=pose(expression,false,false);
    for (int eye=0;eye<2;eye++) {
      assert(p.eye[eye].x>=0 && p.eye[eye].y>=0);
      assert(p.eye[eye].x+p.eye[eye].w<=160);
      assert(p.eye[eye].y+p.eye[eye].h<=100);
    }
    auto blinking=pose(expression,true,true);
    for (auto &eye: blinking.eye) {
      assert(eye.x+eye.w<=160);
      assert(eye.h==5);
    }
    if (i) assert(std::strcmp(p.name,pose(Calm,false,false).name));
    expression=next(expression);
  }
  assert(expression==Calm);
  auto wink=pose(Wink,false,false);
  assert(wink.eye[0].h < wink.eye[1].h);
  auto curious=pose(Curious,false,false);
  assert(curious.eye[0].h != curious.eye[1].h);
  auto sleep=pose(Sleepy,false,false);
  assert(sleep.eye[0].h==5 && sleep.eye[1].h==5);
}
