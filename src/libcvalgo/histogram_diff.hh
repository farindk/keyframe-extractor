/********************************************************************************
  $Header: /home/farin/cvs/libcvalgo/libcvalgo/histogram_diff.hh,v 1.3 2004/09/29 08:54:41 farin Exp $

    Simple histogram difference measures.
 ********************************************************************************
    Copyright (C) 2001 Dirk Farin.    Unauthorized redistribution is prohibited.
 ********************************************************************************/

#ifndef LIBCVALGO_FEATURES_HISTOGRAM_DIFF_HH
#define LIBCVALGO_FEATURES_HISTOGRAM_DIFF_HH

#include "libcvalgo/histogram.hh"

namespace cvalgo {
  using namespace videogfx;

  class HistogramDiff
  {
  public:
    virtual ~HistogramDiff() { }

    virtual double Diff(const Histogram&,const Histogram&) = 0;
    virtual const char* Name() const = 0;
    virtual double MinError() const { return 0.0; }
    virtual double MaxError() const = 0;
  };


  class HistogramDiff_SquaredError : public HistogramDiff
  {
  public:
    double Diff(const Histogram&,const Histogram&);
    const char* Name() const { return "squared error"; }
    double MinError() const { return 0.0; }
    double MaxError() const { return 1.0; }
  };


  class HistogramDiff_AbsoluteError : public HistogramDiff
  {
  public:
    double Diff(const Histogram&,const Histogram&);
    const char* Name() const { return "absolute error\n"; }
    double MinError() const { return 0.0; }
    double MaxError() const { return 1.0; }
  };


  class HistogramDiff_ChiSquare : public HistogramDiff
  {
  public:
    double Diff(const Histogram&,const Histogram&);
    const char* Name() const { return "chi square\n"; }
    double MinError() const { return 0.0; }
    double MaxError() const { return 1.0; }
  };


  class HistogramDiff_KolmogorovSmirnov : public HistogramDiff
  {
  public:
    double Diff(const Histogram&,const Histogram&);
    const char* Name() const { return "Kolmogorov-Smirnov"; }
    double MinError() const { return 0.0; }
    double MaxError() const { return 1.0; }
  };


  class HistogramDiff_EarthmoverDistance : public HistogramDiff
  {
  public:
    double Diff(const Histogram&,const Histogram&);
    const char* Name() const { return "earth-mover distance"; }
    double MinError() const { return 0.0; }
    double MaxError() const { return 1.0; }
  };

}

#endif
