#ifndef __BSPLINE_H__
#define __BSPLINE_H__

#include "Common.h"

typedef std::vector<float> FloatVector;

class EquationSystem //XNA allowed this to be recovered
{
public:
	FloatVector eqs;
	FloatVector sol;
	int mRowSize;
	int mCurRow;

	EquationSystem(int theNumVariables);
	~EquationSystem();
	void SetCoefficient(int theCol, float theValue);
	void SetCoefficient(int theRow, int theCol, float theValue);
	void SetConstantTerm(float theValue);
	void SetConstantTerm(int theRow, float theValue);
	void NextEquation();
	void Solve();
	void CheckRanges(); //Not a function
};

namespace Sexy
{
	class BSpline
	{
	protected:
		FloatVector			mXPoints;
		FloatVector			mYPoints;
		FloatVector			mArcLengths;
		FloatVector			mXCoef;
		FloatVector			mYCoef;
		float				GetPoint(float t, FloatVector& theCoef);
		void				CalculateSplinePrv(FloatVector& thePoints, FloatVector& theCoef);
		void				CalculateSplinePrvLinear(FloatVector& thePoints, FloatVector& theCoef);
		void				CalculateSplinePrvSemiLinear(FloatVector& thePoints, FloatVector& theCoef);
		void				CalcArcLengths();
	public:
		BSpline();
		~BSpline();
		void 				Reset();
		void 				AddPoint(float x, float y);
		void 				CalculateSpline(bool linear = false);
		float 				GetXPoint(float t);
		float 				GetYPoint(float t);
		bool				GetNextPoint(float& x, float& y, float& t);
		const FloatVector&	GetXPoints() const;
		const FloatVector&	GetYPoints() const;
		int					GetMaxT();
	};
}
#endif //__BSPLINE_H__