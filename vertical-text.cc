// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
// Small example how write text.
//
// This code is public domain
// (but note, that the led-matrix library this depends on is GPL v2)

#include "led-matrix.h"
#include "graphics.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fstream>
#include <sstream>
#include "Magick++.h"

using namespace rgb_matrix;
using namespace std;

static int usage(const char *progname) {
  fprintf(stderr, "usage: %s [options]\n", progname);
  fprintf(stderr, "Reads text from stdin and displays it. "
          "Empty string: clear screen\n");
  fprintf(stderr, "Options:\n"
          "\t-f <font-file>: Use given font.\n"
          "\t-r <rows>     : Display rows. 16 for 16x32, 32 for 32x32. "
          "Default: 32\n"
          "\t-P <parallel> : For Plus-models or RPi2: parallel chains. 1..3. "
          "Default: 1\n"
          "\t-c <chained>  : Daisy-chained boards. Default: 1.\n"
          "\t-x <x-origin> : X-Origin of displaying text (Default: 0)\n"
          "\t-y <y-origin> : Y-Origin of displaying text (Default: 0)\n"
          "\t-C <r,g,b>    : Color. Default 255,255,0\n"
          "\t-R <rotation> : Sets the rotation of matrix. Allowed: 0, 90, 180, 270. Default: 0.\n"
          "\t-S            : 'Scrambled' 32x16 display with 2 chains on each panel,\n");

  return 1;
}

static bool parseColor(Color *c, const char *str) {
  return sscanf(str, "%hhu,%hhu,%hhu", &c->r, &c->g, &c->b) == 3;
}

Color color(255, 255, 0);
int speed = 100;
std::string text;

void readConfigFile(){
  std::ifstream infile("text.txt"); // contain line by line : text, color and speed
  std::string sline;
  int i = 0;

  while (std::getline(infile, sline))
  {
      std::istringstream iss(sline);
      i++;
      switch (i) {
        case 1:
          text = sline;
          break;
        case 2:
          int r,g,b;
          if (!(iss >> r >> g >> b)) { break; } // error
          color.r = r; color.g = g; color.b = b;
          break;
        case 3:
          if (!(iss >> speed)) { break; }
          break;
      }
  }
  printf("text to display : '%s'\n", text.c_str());
  printf("color : %d,%d,%d\n", color.r, color.g, color.b);
  printf("speed : %d\n", speed);
  return;
}

int main(int argc, char *argv[]) {
  const char *bdf_font_file = NULL;
  const char *bmp_font_file = NULL;
  int rows = 8;
  int chain = 16;
  int parallel = 1;
  int x_orig = 0;
  int y_orig = 0;
  bool scrambled_display = true;
  int rotation = 0;

  Magick::InitializeMagick(NULL);

  int opt;
  while ((opt = getopt(argc, argv, "r:P:c:x:y:f:C:R:S")) != -1) {
    switch (opt) {
    case 'r': rows = atoi(optarg); break;
    case 'P': parallel = atoi(optarg); break;
    case 'c': chain = atoi(optarg); break;
    case 'x': x_orig = atoi(optarg); break;
    case 'y': y_orig = atoi(optarg); break;
    case 'f': bdf_font_file = strdup(optarg); break;
    case 'C':
      if (!parseColor(&color, optarg)) {
        fprintf(stderr, "Invalid color spec.\n");
        return usage(argv[0]);
      }
      break;
    case 'R':
      rotation = atoi(optarg);
      break;
    case 'S':
      scrambled_display = true;
      break;
    default:
      return usage(argv[0]);
    }
  }

  if (bdf_font_file == NULL) {
    bdf_font_file = "fonts/10x20_BDCASH.bdf";
  }

  if (bmp_font_file == NULL) {
    bmp_font_file = "fonts/10x20_BDCASH.bmp";
  }


  if (bdf_font_file == NULL) {
    fprintf(stderr, "Need to specify BDF font-file with -f\n");
    return usage(argv[0]);
  }

  /*
   * Load font. This needs to be a filename with a bdf bitmap font.
   */
  /*
  rgb_matrix::Font font;

  if (!font.LoadFont(bdf_font_file)) {
    fprintf(stderr, "Couldn't load font '%s'\n", bdf_font_file);
    return usage(argv[0]);
  }
  */

  if (rows!= 8 && rows != 16 && rows != 32) {
    fprintf(stderr, "Rows can either be 8, 16 or 32\n");
    return 1;
  }

  if (chain < 1) {
    fprintf(stderr, "Chain outside usable range\n");
    return 1;
  }
  if (chain > 8) {
    fprintf(stderr, "That is a long chain. Expect some flicker.\n");
  }
  if (parallel < 1 || parallel > 3) {
    fprintf(stderr, "Parallel outside usable range.\n");
    return 1;
  }

  if (rotation % 90 != 0) {
    fprintf(stderr, "Rotation %d not allowed! Only 0, 90, 180 and 270 are possible.\n", rotation);
    return 1;
  }

  /*
   * Set up GPIO pins. This fails when not running as root.
   */
  GPIO io;
  if (!io.Init())
    return 1;

  /*
   * Set up the RGBMatrix. It implements a 'Canvas' interface.
   */
  RGBMatrix *canvas = new RGBMatrix(&io, rows, chain, parallel);

  LinkedTransformer *transformer = new LinkedTransformer();
  canvas->SetTransformer(transformer);

  if (scrambled_display) {
    // Mapping the coordinates of a scrambled 32x16 display with 2 chains per panel
    transformer->AddTransformer(new Scrambled32x16Transformer());
  }

/*
 * Optimize CPU usage when using full color by disabling PWM
  bool all_extreme_colors = true;
  all_extreme_colors &= color.r == 0 || color.r == 255;
  all_extreme_colors &= color.g == 0 || color.g == 255;
  all_extreme_colors &= color.b == 0 || color.b == 255;
  if (all_extreme_colors)
    canvas->SetPWMBits(1);
*/

  while(1){
    /*
     * Read file to get text, color and speed
     */
    readConfigFile();
    //printf("text : %s\n", text.c_str());


    int fontWidth = 11;
    int fontHeight = 15;

    int bufHeight(text.length()*fontHeight);
    int bufWidth(fontWidth);
    Magick::Image fontImage;
    bool **bufImage;
    bufImage = new bool*[bufWidth];
    for ( int i=0; i<fontWidth; i++){
      bufImage[i] = new bool[bufHeight];
    }
    fontImage.read(bmp_font_file);

    for (uint i = 0; i<text.length(); i++){ // iterate char
      //printf("draw : %d %c\n",text[i],text[i]);
      //printf("px :\n");
      for (int k=0; k<fontWidth; k++){ // iterate columns
        int idx = k+fontWidth*(text[i]-32);
        if ( idx < 0 || idx > 990 ) continue;

        int dstId = i*fontHeight ;

        for (int j=0; j<fontHeight-1; j++){ // iterate rows
          const Magick::Color &c = fontImage.pixelColor(idx,j);
          bufImage[k][j+i*fontHeight] = c.redQuantum() > 0;
          //printf("%d\t",bufImage[k][j+i*fontHeight]);
        }
        //printf("\n");
      }
    }

    //printf("reso : %dx%d\n", canvas->width(), canvas->height());
    //printf("color : %d,%d,%d\n", color.r, color.g,color.b);
    for ( int n=-canvas->width(); n<bufHeight; n++){
      canvas->Clear();
      for (int j=0; j< min(canvas->width(),bufHeight); j++){
        for (int i=0; i<min(canvas->height(),bufWidth); i++){
          if (j+n < bufHeight && j+n >= 0){
            canvas->SetPixel(j,canvas->height()-i-4,
                            static_cast<int>( bufImage[i][j+n] ) * color.r,
                            static_cast<int>( bufImage[i][j+n] ) * color.g,
                            static_cast<int>( bufImage[i][j+n] ) * color.b);
          }
        }
      }
      usleep(static_cast<useconds_t>(1000000/speed));
    }
  }

  // Finished. Shut down the RGB matrix.
  canvas->Clear();
  delete canvas;

  return 0;
}
