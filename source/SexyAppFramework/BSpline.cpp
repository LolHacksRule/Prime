#include "BSpline.h"

using namespace Sexy;

BSpline::BSpline() //15-16
{
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void BSpline::Reset() //21-27
{
    mXPoints.clear();
    mYPoints.clear();
    mArcLengths.clear();
    mYCoef.clear();
    mYCoef.clear();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void BSpline::AddPoint(float x, float y) //32-35
{
    mXPoints.push_back(x);
    mYPoints.push_back(y);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void BSpline::CalcArcLengths() //40-55
{
    mArcLengths.clear();
    int numCurves = mXPoints.size() - 1;
    for (int i = 0; i < numCurves; i++)
    {
        float x1 = mXPoints[i];
        float y1 = mYPoints[i];
        float x2 = mXPoints[i + 1];
        float y2 = mYPoints[i + 1];
        float item = sqrtf((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
        mArcLengths.push_back(item);
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool BSpline::GetNextPoint(float& x, float& y, float& t) //One var in H5 | 60-93
{
    int anIndex = floorf(t);
    if (anIndex < 0 || anIndex >= (mXPoints.size() | 0) - 1)
    {
        x = GetXPoint(t);
        y = GetYPoint(t);
        return false;
    }
    float aLength = 1.0 / (mArcLengths[anIndex] * 100.0);
    float ox = GetXPoint(t);
    float oy = GetYPoint(t);
    float nt = t;
    float nx;
    float ny;
    float dist;
    do
    {
        nt += aLength;
        nx = GetXPoint(nt);
        ny = GetYPoint(nt);
        dist = (nx - ox) * (nx - ox) + (ny - oy) * (ny - oy);
    } while (!(dist >= 1.0) && !(nt > (mXPoints.size() - 1)));
    x = nx;
    y = ny;
    t = nt;
    return true;
}

void EquationSystem::SetCoefficient(int theCol, float theValue) { SetCoefficient(mCurRow, theCol, theValue); } //At row in XNA | 115
void EquationSystem::SetConstantTerm(float theValue) { SetConstantTerm(mCurRow, theValue); } //At row in XNA | 116
void EquationSystem::NextEquation() { ++mCurRow; } //117

EquationSystem::EquationSystem(int theNumVariables) //Part of BSpline according to H5 and XNA | 126-131
{
    mRowSize = theNumVariables + 1;
    mCurRow = 0;
    eqs.resize(mRowSize * theNumVariables);
    sol.resize(theNumVariables);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void EquationSystem::SetCoefficient(int theRow, int theCol, float theValue) //136-140
{
    int anIndex = mRowSize * theRow + theCol;

    eqs[anIndex] = theValue;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void EquationSystem::SetConstantTerm(int theRow, float theValue) //145-149
{
    int anIndex = mRowSize * theRow + mRowSize - 1;

    eqs[anIndex] = theValue;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void EquationSystem::Solve() //154-196
{
    int i;
    int j;
    int k;
    int max;
    int r;
    int N;
    float temp;
    r = mRowSize;
    N = mRowSize - 1;
    for (i = 0; i < N; i++)
    {
        max = i;
        for (j = i + 1; j < N; j++)
        {
            if (fabs(eqs[j * r + i]) > fabs(eqs[max * r + i]))
                max = j;
        }
        for (k = 0; k < N + 1; k++)
            std::swap(eqs[k + r * max], eqs[k + r + i]);
        for (j = i + 1; j < N; j++)
        {
            float mult = eqs[j * r + i] / eqs[i * r + i];
            if (mult != 0.0)
            {
                for (k = N; k >= i; k--)
                    eqs[j * r + k] = eqs[j * r + k] - eqs[i * r + k] * mult;
            }
        }
    }
    for (j = N - 1; j >= 0; j--)
    {
        temp = 0.0;
        for (k = j + 1; k < N; k++)
            temp += eqs[j * r + k] * sol[k];
        sol[j] = (eqs[j * r + N] - temp) / eqs[j * r + j];
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void BSpline::CalculateSplinePrv(FloatVector& thePoints, FloatVector& theCoef) //? | 201-251
{
    if (thePoints.size() < 2)
        return;

    int numCurves = (thePoints.size() | 0) - 1;
    int numVariables = numCurves * 4;
    int i;
    EquationSystem aSystem = EquationSystem(numVariables);
    aSystem.SetCoefficient(2, 1.0);
    aSystem.NextEquation();
    int c = 0;
    for (i = 0; i < numCurves; i++, c += 4)
    {
        aSystem.SetCoefficient(c + 3, 1);
        aSystem.SetConstantTerm((thePoints[i]));
        aSystem.NextEquation();
        aSystem.SetCoefficient(c, 1);
        aSystem.SetCoefficient(c + 1, 1);
        aSystem.SetCoefficient(c + 2, 1);
        aSystem.SetConstantTerm((thePoints[i + 1]) - (thePoints[i]));
        aSystem.NextEquation();
        aSystem.SetCoefficient(c, 3);
        aSystem.SetCoefficient(c + 1, 2);
        aSystem.SetCoefficient(c + 2, 1);
        if (i < numCurves - 1)
            aSystem.SetCoefficient(c + 6, -1);
        aSystem.NextEquation();
        if (i < numCurves - 1)
        {
            aSystem.SetCoefficient(c, 6);
            aSystem.SetCoefficient(c + 1, 2);
            aSystem.SetCoefficient(c + 5, -2);
            aSystem.NextEquation();
        }
    }
    aSystem.Solve();
    theCoef = aSystem.sol;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void BSpline::CalculateSplinePrvSemiLinear(FloatVector& thePoints, FloatVector& theCoef) //? | 256-325
{
    if (thePoints.size() < 2)
        return;

    int numCurves = (thePoints.size() | 0) - 1;
    int i;
    FloatVector aNewPoints;
    float p1, p2;

    for (int i = 0; i < numCurves; ++i)
    {
        float mix = mArcLengths[i];
        if (mix <= 100)
            mix = 1;
        else
            mix = 100 / mix;
        mix = 0.3;
        p1 = thePoints[i];
        p2 = thePoints[i + 1];
        if (i > 0)
            aNewPoints.push_back(mix * p2 + (1.0 - mix) * p1);
        else
            aNewPoints.push_back(p1);
        if (i > numCurves - 1)
            aNewPoints.push_back(mix * p1 + (1.0 - mix) * p2);
        else
            aNewPoints.push_back(p2);
    }
    thePoints = aNewPoints;
    numCurves = (aNewPoints.size() | 0) - 1;
    theCoef.resize(4 * numCurves); //PC?
    for (i = 0; i < numCurves; ++i)
    {
        p1 = aNewPoints[i];
        p2 = aNewPoints[i + 1];
        int c = i * 4;
        if (((i & 1) != 0) && (i < numCurves - 1))
        {
            float p0 = aNewPoints[i - 1];
            float p3 = aNewPoints[i + 2];
            float A;
            float B;
            float C;
            float D;
            D = p1;
            C = p1 - p0;
            A = -2.0 * (p2 - 2.0 * p1 + p0) - C + (p3 - p2);
            B = -A + p2 - 2.0 * p1 + p0;
            //Different in XNA?
            theCoef[c] = A;
            theCoef[c + 1] = B;
            theCoef[c + 2] = C;
            theCoef[c + 3] = D;
            /*theCoef.push_back(A);
            theCoef.push_back(B);
            theCoef.push_back(C);
            theCoef.push_back(D);*/
        }
        else
        {
            theCoef[c] = 0.0;
            theCoef[c + 1] = 0.0;
            theCoef[c + 2] = p2 - p1;
            theCoef[c + 3] = p1;
            /*theCoef.push_back(0.0);
            theCoef.push_back(0.0);
            theCoef.push_back(p2 - p1);
            theCoef.push_back(p1);*/
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void BSpline::CalculateSplinePrvLinear(FloatVector& thePoints, FloatVector& theCoef) //330-349
{
    if (thePoints.size() < 2)
        return;
    int numCurves = (thePoints.size() | 0) - 1;
    for (int i = 0; i < numCurves; i++)
    {
        int c = i * 4;
        float p1 = (thePoints[i]);
        float p2 = (thePoints[i + 1]);
        theCoef[c] = 0.0;
        theCoef[c + 1] = 0.0;
        theCoef[c + 2] = p2 - p1;
        theCoef[c + 3] = p1;
        /*theCoef.push_back(0.0);
        theCoef.push_back(0.0);
        theCoef.push_back(p2 - p1);
        theCoef.push_back(p1);*/
    }
}

void BSpline::CalculateSpline(bool linear) //353-366
{
    CalcArcLengths();
    if (linear)
    {
        CalculateSplinePrvLinear(mXPoints, mXCoef);
        CalculateSplinePrvLinear(mYPoints, mYCoef);
    }
    else
    {
        CalculateSplinePrv(mXPoints, mXCoef);
        CalculateSplinePrv(mYPoints, mYCoef);
    }
    CalcArcLengths();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
float BSpline::GetPoint(float t, FloatVector& theCoef) //371-395
{
    int anIndex = floorf(t);
    if (anIndex < 0)
    {
        anIndex = 0;
        t = 0;
    }
    else if (anIndex >= (mXPoints.size() | 0) - 1)
    {
        anIndex = (mXPoints.size() | 0) - 2;
        t = anIndex + 1;
    }
    float s = t - anIndex;
    anIndex *= 4;
    float A = (theCoef[anIndex]);
    float B = (theCoef[anIndex + 1]);
    float C = (theCoef[anIndex + 2]);
    float D = (theCoef[anIndex + 3]);
    float s2 = s * s;
    float s3 = s2 * s;
    //return (A * s3 + B * s2 + C * s + theCoef[anIndex + 3]); //?
    return A * s3 + B * s2 + C * s + D;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
float BSpline::GetXPoint(float t) //400-402
{
    return GetPoint(t, mXCoef);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
float BSpline::GetYPoint(float t) //407-409
{
    return GetPoint(t, mYCoef);
}