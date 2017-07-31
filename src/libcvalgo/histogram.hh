/********************************************************************************
  $Header: /home/farin/cvs/libcvalgo/libcvalgo/histogram.hh,v 1.4 2004/09/29 08:54:41 farin Exp $

    Simple histogram data structure.
 ********************************************************************************
    Copyright (C) 2001 Dirk Farin.    Unauthorized redistribution is prohibited.
 ********************************************************************************/

#ifndef LIBCVALGO_FEATURES_HISTOGRAM_HH
#define LIBCVALGO_FEATURES_HISTOGRAM_HH

#include <libvideogfx/types.hh>
#include <libvideogfx/graphics/datatypes/bitmap.hh>
#include <assert.h>

namespace cvalgo {
  using namespace videogfx;

  class Histogram
  {
  public:
    Histogram();
    Histogram(const Histogram&);
    virtual ~Histogram();

    void Create(int lowval,int highval);
    void Count(int val,double incr = 1.0)
    {
      assert(val >= d_base);
      assert(val <= d_maxval);

      d_hist[val-d_base]+=incr;
      d_total+=incr;
    }

    int LowVal() const { return d_base; }
    int HighVal() const { return d_maxval; }

    double operator[](int val) const
    {
      assert(val >= d_base);
      assert(val <= d_maxval);

      return d_hist[val-d_base];
    }

    void Divide(double n);

    const Histogram& operator=(const Histogram&);

    double TotalSum() const { return d_total; }

    void Reset();

  private:
    double* d_hist;
    double  d_total;
    int  d_base;
    int  d_maxval;
  };

}

#endif
