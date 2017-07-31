/********************************************************************************
   Copyright (C) 2003 Dirk Farin.    Unauthorized redistribution is prohibited.
 ********************************************************************************/

#include "libcvalgo/histogram_diff.hh"

namespace cvalgo {

#define SameHistogramRange \
  assert(a.TotalSum() == b.TotalSum()); \
  assert(a.LowVal() == b.LowVal()); \
  assert(a.HighVal() == b.HighVal())

  double HistogramDiff_SquaredError::Diff(const Histogram& a,const Histogram& b)
  {
    SameHistogramRange;

    double total_inv = 1.0/a.TotalSum();

    double err=0;
    for (int i=a.LowVal() ; i<=a.HighVal() ; i++)
      {
	double diff = (a[i]-b[i]); diff *= total_inv;
	err += diff*diff;
      }

    return err/2.0;
  }


  double HistogramDiff_AbsoluteError::Diff(const Histogram& a,const Histogram& b)
  {
    SameHistogramRange;

    double err=0;
    for (int i=a.LowVal() ; i<=a.HighVal() ; i++)
      {
	err += fabs(a[i]-b[i]);
      }

    return err/(2.0*a.TotalSum());
  }


  double HistogramDiff_ChiSquare::Diff(const Histogram& a,const Histogram& b)
  {
    SameHistogramRange;

    double err=0;
    for (int i=a.LowVal() ; i<=a.HighVal() ; i++)
      {
	if (a[i]+b[i])
	  {
	    double diff = a[i]-b[i];
	    double sum  = a[i]+b[i];
	    err += diff*diff/(sum*sum);
	  }
      }

    return err/(a.HighVal()-a.LowVal()+1);
  }


  double HistogramDiff_KolmogorovSmirnov::Diff(const Histogram& a ,const Histogram& b)
  {
    SameHistogramRange;

    double err=0;
    double sum_a=0;
    double sum_b=0;
    for (int i=a.LowVal() ; i<=a.HighVal() ; i++)
      {
	sum_a += a[i];
	sum_b += b[i];

	double diff = fabs(sum_a-sum_b);
	if (diff>err) err=diff;
      }

    return err/a.TotalSum();
  }


  double HistogramDiff_EarthmoverDistance::Diff(const Histogram& a ,const Histogram& b)
  {
    SameHistogramRange;

    double d=0;

    double tomove = 0;
    for (int i=a.LowVal();i<a.HighVal();i++)
      {
	tomove += a[i]-b[i];
	d+=fabs(tomove);
      }

    //cout << tomove << endl;
    //assert(tomove<=1.0);
    return d/((a.HighVal()-a.LowVal()+1)*a.TotalSum());
  }

}
