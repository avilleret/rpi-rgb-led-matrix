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

#include <ctime>
#include <fstream>
#include <sstream>
#include "Magick++.h"

using namespace rgb_matrix;
using namespace std;

static int usage(const char *progname) {
  fprintf(stderr, "usage: %s [options]\n", progname);
  fprintf(stderr, "Reads text, speed and color from file and displays it. "
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
          "\t-S            : 'Scrambled' 32x16 display with 2 chains on each panel,\n"
          "\t-L <loop>     : either 0 or 1 to enable continuous text looping, 1 by default\n");

  return 1;
}

static bool parseColor(Color *c, const char *str) {
  return sscanf(str, "%hhu,%hhu,%hhu", &c->r, &c->g, &c->b) == 3;
}

Color color(255, 255, 255);
int speed = 100;
std::string text, textOpen, textClosed;
int horo[7][3];

void readConfigFile(){
  std::ifstream infile("/home/borne/rpi-rgb-led-matrix/text.txt"); // contain line by line : text, color and speed
  std::string sline;
  int i = 0;

  while (std::getline(infile, sline))
  {
      std::istringstream iss(sline);
      i++;
      switch (i) {
        case 1:
          textOpen = sline;
          break;
        case 2:
          textClosed = sline;
          break;
        case 3:
          if (!(iss >> speed)) { break; }
          break;
        default:
          int h1,m1,h2,m2;
          if (!(iss >> h1  >> m1 >> h2 >> m2)) { break; } // error
          if(h1<h2){
            horo[i-4][0] = h1*100 + m1;
            horo[i-4][1] = h2*100 + m2;
            horo[i-4][2] = 1;
          } else {
            horo[i-4][1] = h1*100 + m1;
            horo[i-4][0] = h2*100 + m2;
            horo[i-4][2] = 0;
          }
          break;
      }
  }
  printf("text to display when opened : '%s'\n", textOpen.c_str());
  printf("text to display when closed : '%s'\n", textClosed.c_str());
  printf("speed : %d\n", speed);
  printf("horaires :\n");
  for (int i=0; i<7; i++){
    printf("jour %d : %d to %d\n",i,horo[i][0], horo[i][1]);
  }
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
  bool loop = true;
  int rotation = 0;

  for (int i=0; i<7; i++){
    horo[i][0]=0;
    horo[i][1]=0;
  }
  Magick::InitializeMagick(NULL);

  int opt;
  while ((opt = getopt(argc, argv, "r:P:c:x:y:f:C:R:S:L")) != -1) {
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
    case 'L':
      loop = atoi(optarg);
      break;
    default:
      return usage(argv[0]);
    }
  }

  if (bdf_font_file == NULL) {
    bdf_font_file = "fonts/10x20_BDCASH.bdf";
  }

  if (bmp_font_file == NULL) {
    bmp_font_file = "/home/borne/rpi-rgb-led-matrix/fonts/10x20_BDCASH.bmp";
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

  canvas->SetBrightness(100);
  canvas->set_luminance_correct(false);

  static int fontWidth = 11;
  static int fontHeight = 15;

  Magick::Image fontImage;
  fontImage.read(bmp_font_file);
  while(1){
    /*
     * Read file to get text, color and speed
     */
    readConfigFile();

    time_t rawtime;
    tm * timeinfo;
    time(&rawtime);
    timeinfo=localtime(&rawtime);
    int day = (timeinfo->tm_wday+6)%7;
    printf("today is %d, we are %s from %d to %d\n", day, horo[day][2] ? "open" : "closed" , horo[day][0], horo[day][1]);
    int currTime = timeinfo->tm_hour*100 + timeinfo->tm_min;
    printf("it's %02d:%02d \t%d\n", timeinfo->tm_hour, timeinfo->tm_min, currTime);
    bool state = false;
    if ( currTime > horo[day][0] && currTime < horo[day][1] ){
      state = (bool) horo[day][2];
    } else {
      state = !(bool) horo[day][2];
    }

    printf("now we are %s\n", state ? "open" : "close") ;
    if (state) text = textOpen;
    else text = textClosed;

    printf("display  : '%s'\n", text.c_str());

    // Optimize CPU usage when using full color by disabling PWM
    bool all_extreme_colors = true;
    all_extreme_colors &= color.r == 0 || color.r == 255;
    all_extreme_colors &= color.g == 0 || color.g == 255;
    all_extreme_colors &= color.b == 0 || color.b == 255;
    if (all_extreme_colors)
      canvas->SetPWMBits(1);

    // create array of bool array, this is our image containing the whole text
    int bufHeight(text.length()*fontHeight);
    int bufWidth(fontWidth);
    bool **bufImage;
    bufImage = new bool*[bufWidth];
    for ( int i=0; i<bufWidth; i++){
      bufImage[i] = new bool[bufHeight];
    }

    // fill array with black
    for ( int i=0; i<bufWidth; i++){
      for ( int j=0; j<bufHeight; j++){
        bufImage[i][j] = false;
      }
    }

    // fill our image with text
    for (uint i = 0; i<text.length(); i++){ // iterate char
      for (int k=0; k<fontWidth; k++){ // iterate columns
        int idx = k+fontWidth*(text[i]-32); // the bitmap font image start with ASCII character 32 (space)
        if ( idx < 0 || idx > 990 ) continue; // 990 is the width of bitmap font minus the width of one char

        for (int j=0; j<fontHeight-1; j++){ // iterate rows
          const Magick::Color &c = fontImage.pixelColor(idx,j);
          bufImage[k][j+i*fontHeight] = c.redQuantum() > 0;
        }
      }
    }


    for ( int n=-canvas->width(); n<bufHeight; n++){
      for (int j=0; j<min(canvas->width(),bufHeight); j++){
        for (int i=0; i<min(canvas->height()/parallel,bufWidth); i++){
          if ( loop ){
            int k = (j+n+(canvas->width()/bufHeight+1)*bufHeight) % bufHeight;
            for (int p=1; p<=parallel; p++){
                canvas->SetPixel(j,canvas->height()/parallel*p-i-4,
                            static_cast<int>( bufImage[i][k] ) * color.r,
                            static_cast<int>( bufImage[i][k] ) * color.g,
                            static_cast<int>( bufImage[i][k] ) * color.b);
            }
          } else {
            if (j+n < bufHeight && j+n >= 0){
              for (int p=1; p<=parallel; p++){
                canvas->SetPixel(j,canvas->height()/parallel*p-i-4,
                            static_cast<int>( bufImage[i][j+n] ) * color.r,
                            static_cast<int>( bufImage[i][j+n] ) * color.g,
                            static_cast<int>( bufImage[i][j+n] ) * color.b);
              }
            }
          }
        }
      }
      usleep(static_cast<useconds_t>(1000000/speed));
    }

   for (int i=0; i<bufWidth; i++){
     if(bufImage[i]) delete[] bufImage[i];
   }
   delete[] bufImage;
  }

  // Finished. Shut down the RGB matrix.
  canvas->Clear();
  delete canvas;

  return 0;
}
