/*
 * Clutch video cutting application.
 * Copyright (c) 2014-2015 Dirk Farin <dirk.farin@gmail.com>
 *
 * This file is part of Clutch, a simple video cutting application.
 *
 * Clutch is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Clutch is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Clutch.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "decoder.hh"
#include <vector>
#include <algorithm>
#include <libvideogfx.hh>
#include <libcvalgo/features/histogram_diff.hh>
#include "cmdline.h"

using namespace videogfx;

struct gengetopt_args_info args_info;


Image<Pixel> convertToImage(const AVFrame* avframe)
{
  Image<Pixel> img;
  img.Create(avframe->width,
             avframe->height,
             Colorspace_YUV,
             Chroma_420);

  for (int y=0;y<avframe->height;y++)
    memcpy(img.AskFrameY()[y], avframe->data[0] + avframe->linesize[0]*y, avframe->width);

  for (int y=0;y<avframe->height/2;y++)
    memcpy(img.AskFrameU()[y], avframe->data[1] + avframe->linesize[1]*y, avframe->width/2);

  for (int y=0;y<avframe->height/2;y++)
    memcpy(img.AskFrameV()[y], avframe->data[2] + avframe->linesize[2]*y, avframe->width/2);

  return img;
}

double calcEntropy(const cvalgo::Histogram& p)
{
  double entropy = 0.0;
  for (int i=0;i<256;i++)
    if (p[i]!=0)
      {
        entropy += p[i] * -log2(p[i]);
        //printf("p:%f e:%f\n",p[i],entropy);
      }

  return entropy;
}

cvalgo::Histogram calcHistogram(const Image<Pixel>& img, int channel)
{
  cvalgo::Histogram histogram;
  histogram.Create(0,255);

  Bitmap<Pixel> bm = img.AskBitmap((BitmapChannel)channel);
  int w = bm.AskWidth();
  int h = bm.AskHeight();

  assert(w!=0 && h!=0);

  for (int y=0;y<h;y++)
    for (int x=0;x<w;x++) {
      histogram.Count( bm.AskFrame()[y][x] );
    }

  histogram.Divide(histogram.TotalSum());

  return histogram;
}


struct Candidate
{
  int64_t frameNr;
  Image<Pixel> image;
  cvalgo::Histogram histogram;

  double entropy;
  double min_histogram_distance;

  double score;
};


void initRandomFrames(std::vector<Candidate>& candidates, int nFrames, int nCandidates)
{
  std::vector<int64_t> frames;
  for (int i=0;i<nFrames;i++) frames.push_back(i);

  while (candidates.size() < nCandidates) {
    int i = rand() % frames.size();
    int64_t f = frames[i];

    Candidate c;
    c.frameNr = f;
    candidates.push_back(c);

    frames[i] = frames.back();
    frames.pop_back();

    const int minDistance = nFrames / (nCandidates*4);
    for (int i=0;i<frames.size();i++) {
      if (abs(frames[i] - f) < minDistance) {
        frames[i] = frames.back();
        frames.pop_back();
        i--;
      }
    }
  }
}


int Sum(const Image<Pixel>& image, int x0,int y0, int x1,int y1)
{
  int sum=0;

  for (int y=y0;y<y1;y++)
    for (int x=x0;x<x1;x++) {
      sum += image.AskFrameY()[y][x];
    }

  return sum;
}

float maxVBorderPercent = 0.25;
float maxHBorderPercent = 0.15;
int   aspect_mean_threshold = 50;

void CropBordersV(std::vector<Candidate>& keyframes)
{
  int w = keyframes[0].image.AskWidth();
  int h = keyframes[0].image.AskHeight();

  int maxBorderWidth = h * maxVBorderPercent;
  int borderWidth = 0;

  for (int i=1;i<=maxBorderWidth;i++) {
    int mean = 0;
    for (auto c : keyframes) {
      mean += Sum(c.image, 0,i-1, w-1,i) + Sum(c.image, 0,h-i, w-1,h-i+1);
    }

    mean /= 2*w*keyframes.size();

    //int var2 = Var2(image, 0,0, i,h-1,mean) + Var2(image, w-maxBorderWidth,0, w,h-1,mean);
    //var2 /= 2*i*h;

    if (mean < aspect_mean_threshold)
      borderWidth = i;
    else
      break;
  }


  if (borderWidth > 0) {
    for (auto& c : keyframes) {
      Image<Pixel> cropped_img;
      cropped_img.Create(w,h-2*borderWidth, Colorspace_YUV);
      Crop(cropped_img, c.image, 0,0,borderWidth,borderWidth);
      c.image = cropped_img;
    }
  }
}


void CropBordersH(std::vector<Candidate>& keyframes)
{
  int w = keyframes[0].image.AskWidth();
  int h = keyframes[0].image.AskHeight();

  int maxBorderWidth = w * maxHBorderPercent;
  int borderWidth = 0;

  for (int i=1;i<=maxBorderWidth;i++) {
    int mean = 0;
    for (auto c : keyframes) {
      mean += Sum(c.image, i-1,0, i,h-1) + Sum(c.image, w-i,0, w-i+1,h-1);
    }

    mean /= 2*h*keyframes.size();

    //int var2 = Var2(image, 0,0, i,h-1,mean) + Var2(image, w-maxBorderWidth,0, w,h-1,mean);
    //var2 /= 2*i*h;

    if (mean < aspect_mean_threshold)
      borderWidth = i;
    else
      break;
  }


  if (borderWidth > 0) {
    for (auto& c : keyframes) {
      Image<Pixel> cropped_img;
      cropped_img.Create(w-2*borderWidth,h, Colorspace_YUV);
      Crop(cropped_img, c.image, borderWidth,borderWidth,0,0);
      c.image = cropped_img;
    }
  }
}


bool AspectCrop(Image<Pixel>& image)
{
  int aspect_h;
  int aspect_v;

  std::string aspect = args_info.aspect_crop_arg;
  size_t pos = aspect.find(':');
  if (pos == std::string::npos) {
    fprintf(stderr,"aspect option has wrong format\n");
    return false;
  }

  std::string aspect_h_str = aspect.substr(0,pos);
  std::string aspect_v_str = aspect.substr(pos+1);

  aspect_h = atoi(aspect_h_str.c_str());
  aspect_v = atoi(aspect_v_str.c_str());

  if (aspect_h<=0 || aspect_v<=0) {
    fprintf(stderr,"aspects must be > 0\n");
    return false;
  }


  int crop_width  = image.AskWidth();
  int crop_height = image.AskHeight();

  // --- try horizontal fit ---

  int hfit_height = image.AskWidth()*aspect_v/aspect_h;
  if (hfit_height <= image.AskHeight()) {
    crop_height = hfit_height;
  }
  else {
    // --- try vertical fit ---

    int vfit_width = image.AskHeight()*aspect_h/aspect_v;
    assert(vfit_width <= image.AskWidth());
    crop_width = vfit_width;
  }

  Image<Pixel> cropped_img;
  cropped_img.Create(crop_width, crop_height, Colorspace_YUV);
  Crop(cropped_img, image,
       (image.AskWidth() - crop_width) / 2,
       image.AskWidth()-crop_width - (image.AskWidth() - crop_width) / 2,
       (image.AskHeight() - crop_height) / 2,
       image.AskHeight()-crop_height - (image.AskHeight() - crop_height) / 2);

  image = cropped_img;

  return true;
}


int main(int argc, char **argv)
{
  srand(time(NULL));

  cmdline_parser(argc,argv,&args_info);

  if (args_info.inputs_num != 1) {
    cmdline_parser_print_help();
    exit(0);
  }

  // --- init video decoder ---

  Decoder decoder;
  decoder.loadMovie(args_info.inputs[0]);


  // --- initial set of candidates ---

  std::vector<Candidate> candidates;
  int nCandidates = args_info.candidates_arg;
  const int CANDIDATES_REDUNDANCY = 2;
  if (nCandidates==0) { nCandidates=args_info.number_arg * CANDIDATES_REDUNDANCY; }

  int64_t nFrames = decoder.getNFrames();

  if (args_info.random_given) {
    initRandomFrames(candidates, nFrames, nCandidates);
  }
  else {
    for (int i=0;i<nCandidates;i++) {
      Candidate c;
      c.frameNr = (i+1)*nFrames/(nCandidates+1);
      candidates.push_back(c);
    }
  }

#if 1
  // --- load video frames ---

  std::sort(candidates.begin(),
            candidates.end(),
            [](const Candidate& a, const Candidate& b) { return a.frameNr < b.frameNr; });

  int frameNr = -1;

  for (Candidate& c : candidates) {
    if (args_info.verbose_given) { printf("loading candidate frame %ld\n", c.frameNr); }

    AVFrame* frame = NULL;
    if (!args_info.noseek_given) {
      decoder.seekToFrame(c.frameNr);
      frame = decoder.getVideoFrame();
    }
    else {
      while (frameNr < c.frameNr) {
        frame = decoder.getVideoFrame();
        frameNr++;
        decoder.seekToNextVideoFrame();
      }
    }

    Image<Pixel> img = convertToImage(frame);
    c.image = img;

    c.histogram = calcHistogram(img,0);
    c.entropy = calcEntropy(c.histogram);
  }



  // --- save best images ---

  std::vector<Candidate> keyframes;

  while (!candidates.empty()) {

    // --- compute min. distance to selected keyframes ---

    for (Candidate& c : candidates) {
      cvalgo::HistogramDiff_AbsoluteError histDiff;

      double minDist = 1.0;
      for (Candidate& k : keyframes) {
        double dist = histDiff.Diff(k.histogram, c.histogram);
        if (dist<minDist) minDist=dist;
      }

      c.min_histogram_distance = minDist;
    }


    // --- compute score ---

    for (Candidate& c : candidates) {
      c.score = c.entropy + c.min_histogram_distance;
    }

    std::sort(candidates.begin(),
              candidates.end(),
              [](const Candidate& a, const Candidate& b) { return a.score < b.score; });


    Candidate c = candidates.back();
    keyframes.push_back(c);
    candidates.pop_back();
  }


  if (args_info.border_crop_v_given) {
    CropBordersV(keyframes);
  }

  if (args_info.border_crop_h_given) {
    CropBordersH(keyframes);
  }


  int cnt=1;
  for (auto c : keyframes) {
    bool save = (cnt <= args_info.number_arg);

    if (args_info.verbose_given) {
      printf("%2d%c: #=%5ld E=%f hd=%f\n",cnt, save ? '*':' ', c.frameNr, c.entropy, c.min_histogram_distance);
    }

    if (save) {
      if (args_info.aspect_crop_given) {
        if (!AspectCrop(c.image)) {
          return 0;
        }
      }

      char name[100];
      sprintf(name, args_info.output_arg ,cnt);
      WriteImage_JPEG(name, c.image);
    }

    cnt++;
  }
#endif

  return 0;
}
