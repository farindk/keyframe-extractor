/*
 * Extractor video cutting application.
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

#include "decoder.hh"
#include <iostream>
#include <algorithm>
#include <assert.h>

const bool D = false;


Decoder::Decoder()
{
  av_register_all();

  //mState = STATE_CLOSED;
  mFormatCtx = NULL;

  mCurrentFrame = NULL;
  mCurrentFrameNumber = -1;
}


Decoder::~Decoder()
{
  freeCurrentFrame();
}

void Decoder::freeCurrentFrame()
{
  if (mCurrentFrame) {
    av_frame_unref(mCurrentFrame);
    av_free(mCurrentFrame);

    mCurrentFrame = NULL;
  }
}

int64_t Decoder::getVideoDuration() const
{
  assert(mFormatCtx);
  return mFormatCtx->duration;
}


int Decoder::loadMovie(const char* filename)
{
  freeCurrentFrame();
  mFrameInfos.clear();
  mCurrentFrameNumber = -1;

  int err;
  if ((err=avformat_open_input(&mFormatCtx, filename, NULL, NULL)) < 0) {
    return err;
  }


  if ((err=avformat_find_stream_info(mFormatCtx, NULL)) <0 ) {
    return err;
  }

  bool videoStreamFound=false;
  mVDecoder.mDecoder = NULL;

  if (D) std::cout << "total duration: " << mFormatCtx->duration / double(AV_TIME_BASE) << "\n";

  for (int i=0;i<mFormatCtx->nb_streams;i++) {

    AVStream* stream = mFormatCtx->streams[i];

    if (stream->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
      AVCodec* codec = avcodec_find_decoder(stream->codec->codec_id);
      if (codec != NULL) {
        videoStreamFound=true;
        mVDecoder.mDecoder = codec;
        mVDecoder.mDecoderContext = stream->codec;
	mVDecoder.mStream = stream;
        mVDecoder.mStreamIdx = i;

        stream->codec->refcounted_frames = 1;

        if ((err = avcodec_open2(stream->codec, codec, NULL))<0) {
          return err;
        }

        if (D) {
        std::cout << i << ": " << codec->long_name << "\n";
	std::cout << "   time-base: " << stream->time_base.num << "/" << stream->time_base.den << "\n";
	std::cout << "   duration:  " << stream->duration << " = "
		  << stream->duration * stream->time_base.num / double(stream->time_base.den) << "\n";
	std::cout << "   start-time:  " << stream->start_time << " = "
		  << stream->start_time * stream->time_base.num / double(stream->time_base.den) << "\n";
        }
      }
    }

    if (D) std::cout << i << ":\n";
  }


  if (videoStreamFound==false) {
    return 1; // error: no video stream/decoder found
  }

  scanStream();

  mInputFileName = filename;


  // load first frame

  loadNextFrame();

  return 0;
}


void Decoder::scanStream()
{
  AVPacket packet;

  int i=0;
  int err;
  while ((err=av_read_frame(mFormatCtx, &packet))==0)  // while OK
    {
      if (packet.stream_index == mVDecoder.mStreamIdx) {


        // Extract packet PTS. (If this does not exist, use DTS instead.)

	frameinfo fi;
	if (packet.pts!=AV_NOPTS_VALUE) { fi.pts = packet.pts; }
	else { fi.pts = packet.dts; }

        fi.dts = packet.dts;

	fi.key = !!(packet.flags & AV_PKT_FLAG_KEY);

        if (D) printf("%d: pts:%ld dts:%ld %s\n",i++,
               packet.pts,packet.dts, fi.key ? "KEY":"");


        // ignore all frames before stream start_time (probably not decodable)

        if (fi.pts >= mVDecoder.mStream->start_time) {
          mFrameInfos.push_back(fi);
        }
      }

      av_free_packet(&packet);
    }


  // sort images in temporal order (PTS)

  std::sort(mFrameInfos.begin(), mFrameInfos.end(),
            [](const Decoder::frameinfo& a,const Decoder::frameinfo& b) { return a.pts<b.pts; });


  // seek to beginning

  err = av_seek_frame(mFormatCtx, mVDecoder.mStreamIdx,
		      mVDecoder.mStream->start_time, AVSEEK_FLAG_BACKWARD);

  avcodec_flush_buffers(mVDecoder.mDecoderContext);
}


AVFrame* Decoder::getVideoFrame()
{
  assert(mCurrentFrame);
  return mCurrentFrame;
}


int Decoder::loadNextFrame()
{
  if (mCurrentFrame) {
    freeCurrentFrame();
  }


  AVFrame* frame = av_frame_alloc();
  int got_picture = 0;

  int err = read_video_frame(frame, &got_picture);

  if (got_picture) {

    mCurrentFrame = frame;


    // advance current frame number

    if (mCurrentFrameNumber<0) {
      mCurrentFrameNumber=0;
    }
    else {
      mCurrentFrameNumber++;
    }


    // check that the decoded frame PTS matches the stored PTS

    int64_t currentPTS = frame->pkt_pts;
    if (D) printf("expected PTS[%d]=%ld, actual PTS:%ld\n",mCurrentFrameNumber,mFrameInfos[mCurrentFrameNumber].pts,currentPTS);


    // we did not return the expected frame -> adjust current frame number

    if (mFrameInfos[mCurrentFrameNumber].pts != currentPTS) {
      if (currentPTS > mFrameInfos[mCurrentFrameNumber].pts) // we skipped forward (should never happen. Maybe broken video file.)
	{
	}

      for (int f=mCurrentFrameNumber+1 ; f<mFrameInfos.size() ; f++) {
        if (mFrameInfos[f].pts == currentPTS) {
          mCurrentFrameNumber = f;
          break;
        }
      }

      // assert(currentPTS == mFrameInfos[mCurrentFrameNumber].pts); // should be correct now (but video file can be broken)
    }
  }
  else {
    av_free(frame);
  }

  return err;
}


int Decoder::read_video_frame(AVFrame* frame, int* got_picture)
{
  AVPacket packet;
  int err;


  // get packet from container format and try to decode image

  while ((err=av_read_frame(mFormatCtx, &packet))==0)  // while OK
    {
      if (packet.stream_index == mVDecoder.mStreamIdx) {

        int ret = avcodec_decode_video2(mVDecoder.mDecoderContext,
                                        frame, got_picture, &packet);

        if (D) std::cout << " got:" << *got_picture << " " << frame->pkt_pts << "\n";
      }

      av_free_packet(&packet);

      if (*got_picture)
        break;
    }


  // if we are at the end of the stream, push an empty dummy packet to get
  // the remaining pictures out of the decoder

  if (err!=0) {
    AVPacket emptyPacket;
    av_init_packet(&emptyPacket);
    emptyPacket.data = NULL;
    emptyPacket.size = 0;
    emptyPacket.stream_index = mVDecoder.mStreamIdx;
    int ret=avcodec_decode_video2(mVDecoder.mDecoderContext,
                                  frame, got_picture, &emptyPacket);
  }

  return 0;
}


int Decoder::seekToNextVideoFrame()
{
  return loadNextFrame();
}

int Decoder::seekToPrevVideoFrame()
{
  assert(mCurrentFrame>0);
  return seekToFrame(mCurrentFrameNumber-1, Backwards);
}


int Decoder::seekToFrame(int64_t frameNr, Direction direction)
{
  if (D) printf("seekToFrame(%ld)\n",frameNr);

  assert(frameNr>=0);
  assert(frameNr<mFrameInfos.size());


  int nReadForward = frameNr - mCurrentFrameNumber;

  // case 1: no seek required

  if (nReadForward==0) { return 0; }

  // case 2: seek forward, only a few frames -> decode without skip

  const int nReadForwardThreshold = 20;

  if (nReadForward>0 && nReadForward<nReadForwardThreshold) {
    int err;

    // do not read fixed amount of frames (nReadForward), because there may be skipped frames
    while (mCurrentFrameNumber < frameNr) {
      err = loadNextFrame();
      if (err != 0) return err;
    }
    return 0;
  }


  // case 3: skip and read until we get a frame with matching PTS

  freeCurrentFrame();

#if 0
  int64_t targetPTS = mFrameInfos[frameNr].pts;

  if (D) printf("targetPTS=%ld\n",targetPTS);


  /* Skip to requested PTS (actually, this often skips to a given DTS
   */

  if (0) {
    int err = avformat_seek_file(mFormatCtx, mVDecoder.mStreamIdx,
                                 0, 1260000, 1260000,
                                 0); // AVSEEK_FLAG_FRAME);
  }

  int err;
  const int seekPTSOffset = 0; // 1000

  int64_t seekPTS = targetPTS - seekPTSOffset;
  seekPTS = std::max(seekPTS, mVDecoder.mStream->start_time);

  err = av_seek_frame(mFormatCtx, mVDecoder.mStreamIdx,
		      seekPTS, AVSEEK_FLAG_BACKWARD);
#endif

#if 1
  /* Find the corresponding DTS for the frame and skip to that timestamp.
     (Skipping usually uses the DTS. This is also safer than the PTS, since
     the DTS is always smaller/equal.)
   */

  int64_t targetPTS = mFrameInfos[frameNr].pts;
  int64_t targetDTS = mFrameInfos[frameNr].dts;

  if (D) printf("targetPTS=%ld direction=%s\n",targetPTS,(direction==Forwards) ? "Forwards" : (direction==Backwards) ? "Backwards" : "Exact");


  if (0) {
    int err = avformat_seek_file(mFormatCtx, mVDecoder.mStreamIdx,
                                 0, 1260000, 1260000,
                                 0); // AVSEEK_FLAG_FRAME);
  }

  int err;

  err = av_seek_frame(mFormatCtx, mVDecoder.mStreamIdx,
		      targetDTS, AVSEEK_FLAG_BACKWARD);
#endif

  if (err<0) {
    if (D) printf("av_seek_frame error: %d\n",err);
    return err;
  }

  avcodec_flush_buffers(mVDecoder.mDecoderContext);



  // --- read forward until we reach exactly the requested frame ---

  AVFrame* frame = av_frame_alloc();

  int64_t last_pts_decoded = AV_NOPTS_VALUE;

  for (;;) {
    // read next frame

    int got_picture = 0;
    err = read_video_frame(frame, &got_picture);

    if (D) printf("seek: got pts=%ld, want pts=%ld\n",frame->pkt_pts,targetPTS);
    //if (frame->pkt_pts == AV_NOPTS_VALUE) { return -1; }


    // case 1: target frame reached

    if (frame->pkt_pts == targetPTS) {
      mCurrentFrame = frame;
      mCurrentFrameNumber = frameNr;
      break;
    }

    // case 2: target frame missed (did not seek back enough or skipped frame)

    else if (frame->pkt_pts > targetPTS) {
      //av_frame_unref(frame);
      // assert(0); // TODO: seek further to the beginning

      mCurrentFrame = frame;

      mCurrentFrameNumber = -1;

      switch (direction) {
      case Exact:
        assert(0); // there should not be any skipped frames
        break;

      case Forwards:
        // find current frame number
        for (int f=frameNr+1 ; f<mFrameInfos.size() ; f++) {
          if (mFrameInfos[f].pts == frame->pkt_pts) {
            mCurrentFrameNumber = f;
            break;
          }
        }

        assert(mCurrentFrameNumber >= 0);
        break;

      case Backwards:
        assert(last_pts_decoded != AV_NOPTS_VALUE);

        av_frame_unref(frame);
        return seekToFrame( getFrameNrWithPTS(last_pts_decoded), Exact);
      }

      break;
    }

    // case 3: frame PTS before target PTS -> simply skip frame

    else {
      last_pts_decoded = frame->pkt_pts;
      av_frame_unref(frame);
    }


    assert(frame->pkt_pts < targetPTS);
  }

  //assert(mCurrentFrame->pkt_pts == targetPTS);

  return err;
}


int64_t Decoder::getKeyframeBeforeFrameNr(int64_t frameNr) const
{
  for (int f=frameNr-1; f>=0; f--) {
    if (mFrameInfos[f].key)
      return f;
  }

  return -1;
}


int64_t Decoder::getFrameNrWithPTS(int64_t pts) const
{
  for (int64_t f=0;f<mFrameInfos.size();f++) {
    if (mFrameInfos[f].pts == pts) {
      return f;
    }
  }

  assert(false);
  return -1;
}
