#pragma once
void drawPhotoPage() {
  gfx->fillScreen(C_BLACK);
  if (photoValid && photoPixels) {
    uint16_t *out=gfx->getFramebuffer();
    const int offset=(LCD_WIDTH-int(PhotoProtocol::Pixels)*2)/2;
    for (unsigned y=0;y<PhotoProtocol::Pixels;y++) {
      uint16_t *row=out+(offset+y*2)*LCD_WIDTH+offset;
      for(unsigned x=0;x<PhotoProtocol::Pixels;x++) row[x*2]=row[x*2+1]=photoPixels[y*PhotoProtocol::Pixels+x];
      memcpy(row+LCD_WIDTH,row,PhotoProtocol::Pixels*4);
    }
  } else {
    printCentered("PHOTO",CX,180,4,C_WHITE);
    printCentered("SEND A PHOTO FROM IPHONE",CX,250,2,C_GREY);
  }
  onWaitScreen=false; frameIsMap=false; needFlush=true; photoDirty=false;
}
