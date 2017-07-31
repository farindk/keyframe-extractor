/********************************************************************************
   Copyright (C) 2003 Dirk Farin.    Unauthorized redistribution is prohibited.
 ********************************************************************************/

#include "libcvalgo/histogram.hh"

namespace cvalgo {

  Histogram::Histogram()
  {
    d_hist=NULL;
  }


  Histogram::Histogram(const Histogram& h)
    : d_hist(NULL)
  {
    *this = h;
  }


  Histogram::~Histogram()
  {
    if (d_hist) delete[] d_hist;
  }


  void Histogram::Create(int lowval,int highval)
  {
    if (d_hist) delete[] d_hist;
    d_hist = new double[highval-lowval+1];

    d_base=lowval;
    d_maxval=highval;

    Reset();
  }


  const Histogram& Histogram::operator=(const Histogram& h)
  {
    if (h.d_hist==NULL) {
      if (d_hist) delete[] d_hist;
      d_hist=NULL;
    }
    else {
      Create(h.d_base,h.d_maxval);

      for (int i=d_base;i<=d_maxval;i++)
        d_hist[i-d_base] = h.d_hist[i-d_base];

      d_total=h.d_total;
    }

    return *this;
  }


  void Histogram::Divide(double n)
  {
    assert(n != 0.0);

    for (int i=d_base;i<=d_maxval;i++)
      d_hist[i-d_base] /= n;

    d_total /= n;
  }


  void Histogram::Reset()
  {
    for (int i=d_base;i<=d_maxval;i++)
      d_hist[i-d_base]=0;

    d_total=0.0;
  }

}
