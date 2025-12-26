#include "PolyDivide.h"
#include <stdio.h>

using namespace Sexy; //Heavily based off https://wiki.preterhuman.net/Polytri.c

/* A triangle made up of three vertices */
typedef FPoint triangle;

#define V_MAX 100	/* Maximum number of vertices allowed (arbitrary) */

#define BIG 1.0e30	/* A number bigger than we expect to find here */

#define COUNTER_CLOCKWISE 0
#define CLOCKWISE 1

/*
 * orientation
 *
 * Return either clockwise or counter_clockwise for the orientation
 * of the polygon.
 */

static int orientation(int n, FPoint* v) //24-39
{
	float area;
	int i;

	/* Do the wrap-around first */
	area = v[n - 1].mX * v[0].mY - v[0].mX * v[n - 1].mY;

	/* Compute the area (times 2) of the polygon */
	for (i = 0; i < n - 1; i++)
		area += v[i].mX * v[i + 1].mY - v[i + 1].mX * v[i].mY;

	if (area >= 0.0)
		return COUNTER_CLOCKWISE;
	else
		return CLOCKWISE;
} /* End of orientation */

/*
 * determinant
 *
 * Computes the determinant of the three points.
 * Returns whether the triangle is clockwise or counter-clockwise.
 */

static int determinant(int p1, int p2, int p3, FPoint* v) //49-65
{
	float x1, x2, x3, y1, y2, y3;
	float determ;

	x1 = v[p1].mX;
	y1 = v[p1].mY;
	x2 = v[p2].mX;
	y2 = v[p2].mY;
	x3 = v[p3].mX;
	y3 = v[p3].mY;

	determ = (x2 - x1) * (y3 - y1) - (x3 - x1) * (y2 - y1);
	if (determ >= 0.0)
		return COUNTER_CLOCKWISE;
	else
		return CLOCKWISE;
} /* End of determinant */



/*
 * distance2
 *
 * Returns the square of the distance between the two points
 */

static float distance2(float x1, float y1, float x2, float y2) //76-85
{
	float xd, yd;		/* The distances in X and Y */
	float dist2;		/* The square of the actual distance */

	xd = x1 - x2;
	yd = y1 - y2;
	dist2 = xd * xd + yd * yd;

	return dist2;
} /* End of distance2 */






/*
 * no_interior
 *
 * Returns 1 if no other point in the vertex list is inside
 * the triangle specified by the three points.  Returns
 * 0 otherwise.
 */

static int no_interior(int p1, int p2, int p3, FPoint* v, int* vp, int n, int poly_or) //101-122
{
	int i;			/* Iterative counter */
	int p;			/* The test point */

	for (i = 0; i < n; i++) {
		p = vp[i];		/* The point to test */
		if ((p == p1) || (p == p2) || (p == p3))
			continue;		/* Don't bother checking against yourself */
		if ((determinant(p2, p1, p, v) == poly_or)
			|| (determinant(p1, p3, p, v) == poly_or)
			|| (determinant(p3, p2, p, v) == poly_or)) {
			continue;		/* This point is outside */
		}
		else {
			return 0;		/* The point is inside */
		}
	}
	return 1;			/* No points inside this triangle */
} /* End of no_interior */

/*
 * draw_poly
 *
 * Call this procedure with a polygon, this divides it into triangles
 * and calls the triangle routine once for each triangle.
 *
 * Note that this does not work for polygons with holes or self
 * penetrations.
 */

bool Sexy::DividePoly(FPoint* v, int n, FPoint* theTris[][3], int theMaxTris, int* theNumTris) //135-233
{
	//int prev, cur, next;	/* Three points currently being considered */
	//int vp[V_MAX];		/* Pointers to vertices still left */
	//int count;			/* How many vertices left */
	//int min_vert;		/* Vertex with minimum distance */
	//int i;			/* Iterative counter */
	//float dist;			/* Distance across this one */
	//float min_dist;		/* Minimum distance so far */
	//int poly_orientation;	/* Polygon orientation */
	//
	//if (n > V_MAX)
	//	return false;
	//
	//poly_orientation = orientation(n, v);
	//
	//for (i = 0; i < n; i++)
	//	vp[i] = i;		/* Put vertices in order to begin */
	//
	///* Slice off clean triangles until nothing remains */
	//
	//count = n;
	//while (count > 3) {
	//	min_dist = BIG;		/* A real big number */
	//	min_vert = 0;		/* Just in case we don't find one... */
	//	for (cur = 0; cur < count; cur++) {
	//		prev = cur - 1;
	//		next = cur + 1;
	//		if (cur == 0)	/* Wrap around on the ends */
	//			prev = count - 1;
	//		else if (cur == count)
	//			next = 0;
	//		/* Pick out shortest distance that forms a good triangle */
	//		if ((determinant(vp[prev], vp[cur], vp[next], v) == poly_orientation)
	//			/* Same orientation as polygon */
	//			&& no_interior(vp[prev], vp[cur], vp[next], v, vp, count, poly_orientation)
	//			/* No points inside */
	//			&& ((dist = distance2(v[vp[prev]].mX, v[vp[prev]].mY,
	//				v[vp[next]].mX, v[vp[next]].mY)) < min_dist))
	//			/* Better than any so far */
	//		{
	//			min_dist = dist;
	//			min_vert = cur;
	//		}
	//	} /* End of for each vertex (cur) */
	//
	//	/* The following error should "never happen". */
	//	if (min_dist == BIG)
	//		return false;
	//
	//	prev = min_vert - 1;
	//	next = min_vert + 1;
	//	if (min_vert == 0)	/* Wrap around on the ends */
	//		prev = count - 1;
	//	else if (min_vert == count)
	//		next = 0;
	//
	//	/* Output this triangle */
	//
	//	t[0].x = v[vp[prev]].x;
	//	t[0].y = v[vp[prev]].y;
	//	t[0].z = v[vp[prev]].z;
	//	t[0].color = v[vp[prev]].color;
	//	t[1].x = v[vp[min_vert]].x;
	//	t[1].y = v[vp[min_vert]].y;
	//	t[1].z = v[vp[min_vert]].z;
	//	t[1].color = v[vp[min_vert]].color;
	//	t[2].x = v[vp[next]].x;
	//	t[2].y = v[vp[next]].y;
	//	t[2].z = v[vp[next]].z;
	//	t[2].color = v[vp[next]].color;
	//
	//	draw_triangle(t);
	//
	//	/* Remove the triangle from the polygon */
	//
	//	count -= 1;
	//	for (i = min_vert; i < count; i++)
	//		vp[i] = vp[i + 1];
	//}
	//
	///* Output the final triangle */
	//
	//t[0].x = v[vp[0]].x;
	//t[0].y = v[vp[0]].y;
	//t[0].z = v[vp[0]].z;
	//t[0].color = v[vp[0]].color;
	//t[1].x = v[vp[1]].x;
	//t[1].y = v[vp[1]].y;
	//t[1].z = v[vp[1]].z;
	//t[1].color = v[vp[1]].color;
	//t[2].x = v[vp[2]].x;
	//t[2].y = v[vp[2]].y;
	//t[2].z = v[vp[2]].z;
	//t[2].color = v[vp[2]].color;
	return false;
} /* End of draw_poly */