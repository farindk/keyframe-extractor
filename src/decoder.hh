/*
 * Extractor
 * Copyright (c) 2014-2015 Dirk Farin <dirk.farin@gmail.com>
 *
 * This file is part of Extractor, a simple video key-frame extration tool.
 *
 * Extractor is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Extractor is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Extractor.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DECODER_HH
#define DECODER_HH

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <vector>
#include <string>

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include <libswscale/swscale.h>
}


class Decoder
{
public:
  Decoder();
  ~Decoder();

  int loadMovie(const char* filename);

  /*
    PLAYBACK state:
    get next video frame (preferably buffered)
    get next audio frame (preferably buffered)

    PAUSED state:
    seek to PTS
    get next/previous video frame
  */

  void playback() { }
  void pause() { }

  // --- static info ---

  int64_t getVideoDuration() const; // div by AV_TIME_BASE gives seconds
  int64_t getNFrames() const { return mFrameInfos.size(); }

  const AVStream* getVideoStream() const { return mVDecoder.mStream; }

  int64_t getFramePTS(int frame) const { return mFrameInfos[frame].pts; }
  int64_t getFrameNrWithPTS(int64_t pts) const;
  const std::string& getInputFileName() const { return mInputFileName; }

  double PTS2Time(int64_t pts) const { return double(pts) * mVDecoder.mStream->time_base.num /
      mVDecoder.mStream->time_base.den; }

  // --- decoding ---

  AVFrame* getVideoFrame(); // get current frame (do not advance)
  AVFrame* getAudioFrame() { return NULL; } // TODO

  //int64_t getCurrentVideoPTS() const { return mCurrentPTS; }

  //int64_t getNextVideoPTS() const { return -1; }
  //int64_t getNextAudioPTS(int stream) const { return -1; }


  // --- seeking ---

  int64_t getCurrentFrameNr() const { return mCurrentFrameNumber; }
  int64_t getKeyframeBeforeFrameNr(int64_t frameNr) const;

  enum Direction { Backwards, Forwards, Exact };

  // Direction specifies in which direction we should seek further when the requested
  // frame could not be decoded:
  // - Forwards: if frame cannot be decoded, return a frame after the requested frame
  // - Backwards: if frame cannot be decoded, return a frame before the requested frame
  // - Exact: we do not accept any other frame than the requested
  //          (we will fail hard if it cannot be decoded).
  int seekToFrame(int64_t frameNr, enum Direction = Forwards);
  int seekToNextVideoFrame();
  int seekToPrevVideoFrame();


  // --- playback ---

  //void    startPlayback(bool forward=true);
  //void    stopPlayback();

  //void     setPlaybackPTS(int64_t pts);
  //AVFrame* getCurrentFrame(int streamIdx) const;

  //int64_t getPrevPTS(int streamIdx) const { return -1; }
  //int64_t getNextPTS(int streamIdx) const { return -1; }


private:
  std::string mInputFileName;

  //enum DecoderState { STATE_CLOSED, STATE_PLAYBACK, STATE_PAUSED, STATE_ERROR } mState;

  AVFormatContext* mFormatCtx;

  struct stream_decoder
  {
    AVCodec* mDecoder;
    AVCodecContext* mDecoderContext;
    AVStream* mStream;
    int      mStreamIdx;
  };

  stream_decoder mVDecoder;

  //std::vector<stream_decoder> mDecoders;


  struct frameinfo
  {
    int64_t pts;
    int64_t dts;
    bool    key;
  };

  std::vector<frameinfo> mFrameInfos;

  AVFrame* mCurrentFrame;
  int64_t  mCurrentFrameNumber;


  void scanStream();

  int  loadNextFrame();
  int  read_video_frame(AVFrame* frame, int* got_picture);

  void freeCurrentFrame();
};

#endif
