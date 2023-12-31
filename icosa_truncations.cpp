/*
Copyright (C) 2023 Christopher J Kitrick

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
/*

	This code is designed to determine the equilateral positions of vertices
	on a class I icosahedron based geodesic configuration. Using spherical 
	trigonometry and spherical coordinates, cartesian coordinates are computed,
	when necessary in iterative fashion, to fit the geometric constraints.
	The code provided handles {3,5+}(b,c) for b,c pairs: (2,0) (3,0) (4,0) (5,0) (6,0) and (7,0).
	Most of the functions in the file are utility based, providing necessary 
	handling of trigonometry, vector, and matrix based operations. 
	Final solutions are designed to output geometry information in OFF file format.
	Refer to additional image that defines the geometric notation for vertices
	that are used to derive solutions. 

	The code was confirmed to work correctly when built with VisualStudio 2017.

*/
// icosa_truncations.cpp : Defines the entry point for the console application.
//
#include <stdio.h>
#include <memory.h>
#include <math.h>

#define MTX_ROTATE_X_AXIS	1
#define MTX_ROTATE_Y_AXIS	2
#define MTX_ROTATE_Z_AXIS	3

// Utility Conversion Macros
#define DTR( degree )	( ( degree ) * 0.01745329251994329576923690768489 )
#define RTD( degree )	( ( degree ) * 57.295779513082320876798154814105 )

// sructure for a spherical triangle and its component angles
typedef struct {
	double a, b, c, A, B, C;
} SPH_TRI;

// cartesian coordinate
typedef struct {
	double x, y, z, w;
} GUT_POINT;

// vector
typedef struct {
	double i, j, k, l;
} GUT_VECTOR;

// spherical coordinate 
typedef struct {
	double radius, azimuth, inclination;
} GUT_SPHERICAL_COORD;

// each vertex resides in 6 symmetrical positions 
typedef struct {
	GUT_POINT			p[6];		// cartesian coordinates
	GUT_SPHERICAL_COORD	sc[6];		// spherical coordinates
} VERTEX;

// transform matrices 
typedef struct {
	struct {
		double	m[16];
	} sub[6];
} SUBFACE;

typedef struct {
	// contains the rotation matrices for moving faces to global position and reverse
	struct {
		double	tm[16], tmt[16]; // local to global rotations for icosa face in equalatorial position to z up and back
		double	m0[16], mt0[16]; // z rotation 0 degrees
		double	m1[16], mt1[16]; // z rotation 120 degrees
		double	m2[16], mt2[16]; // z rotation 240 degrees
	} face;

	// contains the rotation and mirroring matrics for replicating 'A' positions within face
	SUBFACE subface[6];

	// reference spherical triangle (LCD)
	SPH_TRI	ref;

	// space for vertices
	VERTEX	v[20];
} PROGRAM;

typedef double BUILD(double*, void*);

void gut_cartesian_to_spherical(GUT_POINT *p, GUT_SPHERICAL_COORD *sc); //GUT_POINT, GUT_SPHERICAL_COORD  
void gut_spherical_to_cartesian(GUT_SPHERICAL_COORD *sc, GUT_POINT *p);
void gut_cross_product(GUT_VECTOR *a, GUT_VECTOR *b, GUT_VECTOR *c);
void gut_normalize_vector(GUT_VECTOR *v);
void gut_vector(GUT_POINT *a, GUT_POINT *b, GUT_VECTOR *v);
void mtx_create_rotation_matrix(double m[], int axis, double angle);
void mtx_create_scale_matrix(double m[], double x, double y, double z); //MTX_MATRIX *m, double x, double y, double z )
void mtx_transpose_matrix(double m[]); //MTX_MATRIX *m )
void mtx_set_unity(double m[]); //double m[16]
void mtx_multiply_matrix(double a[], double b[], double c[]); //MTX_MATRIX *a, MTX_MATRIX *b, MTX_MATRIX *c )
void mtx_vec4_multiply(int length, GUT_VECTOR *a, GUT_VECTOR *b, double m[]);


void generate_all_vertices(PROGRAM *pgm, int v, int a);
void build_a_transforms(PROGRAM *pgm);
void subface_exchange(int aSrc, int aDst, GUT_POINT *pSrc, GUT_POINT *pDst, SUBFACE *subface);
void build_subface_transforms(PROGRAM *pgm);
void build_face_transforms(PROGRAM *pgm);
void rotation_matrix_from_triangle(GUT_POINT *p0, GUT_POINT *p1, GUT_POINT *p2, double mr[], double mrt[]);
double build_loop(BUILD* build, double *seed, double r_zero, void *var);
void vertex_by_strig(SPH_TRI *st, double b, double c, double C, GUT_SPHERICAL_COORD *sc, GUT_POINT *p);
void sph_tri_bcC(SPH_TRI *st); //, b, c, C );
void create_vertex_by_strig(PROGRAM *pgm, int v, int a, double b, double c, double C);
void create_vertex_by_sc(PROGRAM *pgm, int v, int a, double azimuth, double inclination);
void create_vertex_from_vertex(PROGRAM *pgm, int vd, int ad, int vs, int as, double b, double C);
void classI_2v_output(PROGRAM *pgm, char *filename);
void classI_2v(PROGRAM *pgm);
void classI_3v_output(PROGRAM *pgm, char *filename);
void classI_3v(PROGRAM *pgm);
void classI_4v_output(PROGRAM *pgm, char *filename);
void classI_4v(PROGRAM *pgm);
void classI_5v_output(PROGRAM *pgm, char *filename);
double classI_5v(double *var, void *program);
double classI_7v_a(double *var, void *program);
double classI_7v_b1(double *var, PROGRAM *pgm);
double classI_7v_b2(double *var, PROGRAM *pgm);
double classI_7v_b3(double *var, PROGRAM *pgm);
void classI_7v_details(PROGRAM *pgm);
void classI_7v_output(PROGRAM *pgm, char *filename);
double classI_6v_a(double *var, PROGRAM *pgm);
double classI_6v_b(double *var, PROGRAM *pgm);
void classI_6v(PROGRAM *pgm);
void classI_6v_output(PROGRAM *pgm, char *filename);
void classI_2v_solution(PROGRAM *pgm, char *base_filename);
void classI_3v_solution(PROGRAM *pgm, char *base_filename);
void classI_4v_solution(PROGRAM *pgm, char *base_filename);
void classI_5v_solution(PROGRAM *pgm, char *base_filename);
void classI_6v_solution(PROGRAM *pgm, char *base_filename);
void classI_7v_solution(PROGRAM *pgm, char *base_filename);
int main(int ac, char **av);

//-----------------------------------------------------------------------------------
void gut_vector(GUT_POINT *a, GUT_POINT *b, GUT_VECTOR *v)
//-----------------------------------------------------------------------------------
{
	v->i = b->x - a->x;
	v->j = b->y - a->y;
	v->k = b->z - a->z;
}

//-----------------------------------------------------------------------------------
void gut_normalize_vector(GUT_VECTOR *v)
//-----------------------------------------------------------------------------------
{
	double	distance;

	distance = sqrt(v->i * v->i + v->j * v->j + v->k * v->k);

	v->i /= distance;
	v->j /= distance;
	v->k /= distance;
}

//-----------------------------------------------------------------------------------
void gut_cross_product(GUT_VECTOR *a, GUT_VECTOR *b, GUT_VECTOR *c)
//-----------------------------------------------------------------------------------
{
	//	ax = by * cz - bz * cy;
	//	ay = bz * cx - bx * cz;
	//	az = bx * cy - by * cx;

	c->i = a->j * b->k - a->k * b->j;
	c->j = a->k * b->i - a->i * b->k;
	c->k = a->i * b->j - a->j * b->i;
}

//-----------------------------------------------------------------------------------
void gut_spherical_to_cartesian(GUT_SPHERICAL_COORD *sc, GUT_POINT* p)//double radius, double inclination, double azimuth)
//-----------------------------------------------------------------------------------
{
	// http://en.wikipedia.org/wiki/Spherical_coordinate_system

	p->x = sc->radius * sin(sc->inclination) * cos(sc->azimuth);
	p->y = sc->radius * sin(sc->inclination) * sin(sc->azimuth);
	p->z = sc->radius * cos(sc->inclination);
}

//-----------------------------------------------------------------------------------
void gut_cartesian_to_spherical(GUT_POINT *p, GUT_SPHERICAL_COORD *s)
//-----------------------------------------------------------------------------------
{
	// http://en.wikipedia.org/wiki/Spherical_coordinate_system
	double	ZERO = 0.00000000000001;
	s->radius = sqrt(p->x * p->x + p->y * p->y + p->z * p->z);
	if (fabs(s->radius) > ZERO)
		s->inclination = acos(p->z / s->radius);
	else
		return;

	/* RESULTS
	x		y		result
	----	-----	--------
	0		0		0
	0		+		90
	0		- 		-90
	+		0		0
	+		+		atan(y/x)
	+		- 		atan(y/x)
	-		0		180
	-		+		180 + atan(y/x)
	-		-		-180 + atan(y/x)
	*/
	if (fabs(p->x) <= ZERO)
	{
		if (fabs(p->y) <= ZERO)
			s->azimuth = 0;						// 0 0 0
		else if (p->y > 0)
			s->azimuth = DTR(90.0);				// 0 + 90
		else
			s->azimuth = DTR(-90.0);			// 0 - -90
	}
	else if (p->x > 0) 
	{
		if (fabs(p->y) <= ZERO)
			s->azimuth = 0;						// + 0 0
		else if (p->y > 0)
			s->azimuth = atan(p->y / p->x);		// + + atan(y/x)
		else
			s->azimuth = atan(p->y / p->x);		// + - atan(y/x)
	}
	else 
	{
		if (fabs(p->y) <= ZERO)
			s->azimuth = DTR(180.0);						// - 0 180
		else if (p->y > 0)
			s->azimuth = DTR(180.0) + atan(p->y / p->x);// - + 180 + atan(y/x)
		else
			s->azimuth = DTR(-180.0) + atan(p->y / p->x);// - - -180 + atan(y/x)
	}
}

//--------------------------------------------------------------------------
void mtx_create_scale_matrix(double m[], double x, double y, double z) 
//--------------------------------------------------------------------------
{
	mtx_set_unity(m); //MTX_MATRIX *m )
	m[0] = x;
	m[5] = y;
	m[10] = z;
}

//--------------------------------------------------------------------------
void mtx_transpose_matrix(double m[])  
//--------------------------------------------------------------------------
{
	int		r, c, fr, to;
	double	temp;

	// transpose the incoming matrix structure
	for (r = 0; r < 4; ++r) // row
	{
		for (c = r + 1; c < 4; ++c) // column
		{
			fr = r * 4 + c;
			to = c * 4 + r;
			temp = m[to];
			m[to] = m[fr];
			m[fr] = temp;
		}
	}
}

//--------------------------------------------------------------------------
void mtx_set_unity(double m[])  
//--------------------------------------------------------------------------
{
	// reset the incoming matrix structure
	m[0] = m[1] = m[2] = m[3] = 0.0;
	m[4] = m[5] = m[6] = m[7] = 0.0;
	m[8] = m[9] = m[10] = m[11] = 0.0;
	m[12] = m[13] = m[14] = m[15] = 0.0;
	m[0] = m[5] = m[10] = m[15] = 1.0;
}

//---------------------------------------------------------------------------
void mtx_multiply_matrix(double a[], double b[], double c[]) 
//---------------------------------------------------------------------------
{												  // row 1
	c[0] = a[0] * b[0] + a[1] * b[4] + a[2] * b[8] + a[3] * b[12];
	c[1] = a[0] * b[1] + a[1] * b[5] + a[2] * b[9] + a[3] * b[13];
	c[2] = a[0] * b[2] + a[1] * b[6] + a[2] * b[10] + a[3] * b[14];
	c[3] = a[0] * b[3] + a[1] * b[7] + a[2] * b[11] + a[3] * b[15];

	// row 2
	c[4] = a[4] * b[0] + a[5] * b[4] + a[6] * b[8] + a[7] * b[12];
	c[5] = a[4] * b[1] + a[5] * b[5] + a[6] * b[9] + a[7] * b[13];
	c[6] = a[4] * b[2] + a[5] * b[6] + a[6] * b[10] + a[7] * b[14];
	c[7] = a[4] * b[3] + a[5] * b[7] + a[6] * b[11] + a[7] * b[15];

	// row 3
	c[8] = a[8] * b[0] + a[9] * b[4] + a[10] * b[8] + a[11] * b[12];
	c[9] = a[8] * b[1] + a[9] * b[5] + a[10] * b[9] + a[11] * b[13];
	c[10] = a[8] * b[2] + a[9] * b[6] + a[10] * b[10] + a[11] * b[14];
	c[11] = a[4] * b[3] + a[5] * b[7] + a[6] * b[11] + a[7] * b[15];

	// row 4
	c[12] = a[12] * b[0] + a[13] * b[4] + a[14] * b[8] + a[15] * b[12];
	c[13] = a[12] * b[1] + a[13] * b[5] + a[14] * b[9] + a[15] * b[13];
	c[14] = a[12] * b[2] + a[13] * b[6] + a[14] * b[10] + a[15] * b[14];
	c[15] = a[12] * b[3] + a[13] * b[7] + a[14] * b[11] + a[15] * b[15];
}

//---------------------------------------------------------------------------
void mtx_vec4_multiply(int length, GUT_VECTOR *a, GUT_VECTOR *b, double m[])
//---------------------------------------------------------------------------
{
	// 'a' is the input vector
	// 'b' is the output vector
	// 'm' is the matrix
	// 'length' determines whether 'a' and 'b' is a single vector or an array of vectors

	if (length < 1)
	{
		return;
	}
	else if (length == 1)
	{
		b->i =
			a->i * m[0] +
			a->j * m[4] +
			a->k * m[8] +
			a->l * m[12];

		b->j =
			a->i * m[1] +
			a->j * m[5] +
			a->k * m[9] +
			a->l * m[13];

		b->k =
			a->i * m[2] +
			a->j * m[6] +
			a->k * m[10] +
			a->l * m[14];

		b->l =
			a->i * m[3] +
			a->j * m[7] +
			a->k * m[11] +
			a->l * m[15];
	}
	else
	{
		int		i;

		for (i = 0; i<length; ++i)
		{
			b[i].i =
				a[i].i * m[0] +
				a[i].j * m[4] +
				a[i].k * m[8] +
				a[i].l * m[12];

			b[i].j =
				a[i].i * m[1] +
				a[i].j * m[5] +
				a[i].k * m[9] +
				a[i].l * m[13];

			b[i].k =
				a[i].i * m[2] +
				a[i].j * m[6] +
				a[i].k * m[10] +
				a[i].l * m[14];

			b[i].l =
				a[i].i * m[3] +
				a[i].j * m[7] +
				a[i].k * m[11] +
				a[i].l * m[15];
		}
	}
}

//---------------------------------------------------------------------------
void mtx_create_rotation_matrix(double m[], int axis, double angle)
//---------------------------------------------------------------------------
{
	double	s, // sin value
		c; // cos value

		   // reset the incoming matrix structure
	mtx_set_unity(m);// *m = m_unity;

	switch (axis) {
	case MTX_ROTATE_X_AXIS:
		c = cos(angle);
		s = sin(angle);
		m[5] = c;
		m[7] = s;
		m[9] = -s;
		m[10] = c;
		break;

	case MTX_ROTATE_Y_AXIS:
		c = cos(angle);
		s = sin(angle);
		m[0] = c;
		m[2] = -s;
		m[8] = s;
		m[10] = c;
		break;

	case MTX_ROTATE_Z_AXIS:
		c = cos(angle);
		s = sin(angle);
		m[0] = c;
		m[1] = s;
		m[4] = -s;
		m[5] = c;
		break;
	}
}

//---------------------------------------------------------------------------
void generate_all_vertices(PROGRAM *pgm, int v, int a)
//---------------------------------------------------------------------------
{
	// vertex already has one defined point in triangle in area (a)
	// need to compute all the symmetrical equivalents

	// first: transform known point to global position in respective triangle
	// second: duplicate remaining points in other triangle areas 
	// third: transform all global points back to local position of first triangle
	// fourth: transform all global points back to local positon of second triangle
	//
	int			i;
	GUT_POINT	lp[6], gp[6];

	// get a pointer to the known vertex in local space
	lp[a] = pgm->v[v].p[a];

	// transform known point to global position based on its triangle
	mtx_vec4_multiply(1, (GUT_VECTOR*)&lp[a], (GUT_VECTOR*)&gp[a], pgm->face.tm);

	// in global position compute the symmetrical counterpart points
	for (i = 0; i < 6; ++i)
	{
		if (a == i)
			continue; // skip 
		subface_exchange(a, i, &gp[a], &gp[i], pgm->subface);
	}

	// transform points to local position based on its triangle
	mtx_vec4_multiply(6, (GUT_VECTOR*)gp, (GUT_VECTOR*)pgm->v[v].p, pgm->face.tmt);

	// recompute the spherical coordinates for local vertices
	for (i = 0; i < 6; ++i)
		gut_cartesian_to_spherical(&pgm->v[v].p[i], &pgm->v[v].sc[i]); //GUT_POINT, GUT_SPHERICAL_COORD  

	return;
}

//---------------------------------------------------------------------------
void build_rotation_matrix(GUT_VECTOR *x, GUT_VECTOR *y, GUT_VECTOR *z, double m[], double mt[])
//---------------------------------------------------------------------------
{
	// create rotation matrix based on the local coordinate axii to global coordinates
	// transpose is the reverse rotation for global to local
	// basically just assigning the vectors to correct position in the array
	//
	m[0] = x->i, m[1] = y->i, m[2] = z->i, m[3] = 0;
	m[4] = x->j, m[5] = y->j, m[6] = z->j, m[7] = 0;
	m[8] = x->k, m[9] = y->k, m[10] = z->k, m[11] = 0;
	m[12] = 0, m[13] = 0, m[14] = 0, m[15] = 1;
	memcpy(mt, m, sizeof(double) * 16);
	mtx_transpose_matrix(mt);
}

//---------------------------------------------------------------------------
void build_a_transforms(PROGRAM *pgm)
//---------------------------------------------------------------------------
{
	// 	Each number (0-5) represents the 'a' area of the x axis aligned face triangle 
	//
	//		          ^ y axis
	//		          |
	//		          +
	//		        . | .
	//	          .   |   .
	//          .  3  |  2  .      
	//        .       |       .    
	//      .  4      +     1   .   -----> x (+ center of triangle at 0,0,1)
	//    .        5  |  0         .
	//	+ . . . . . . . . . . ... .+   
	//
	double mm[16], mx[16];	  // temp matrices 
	mtx_create_scale_matrix(mx, -1.0, 1.0, 1.0); // x axis mirror transform
	// 0 0	.	.	.		no change
	// 0 1	.	-x	MT1		apply x axis mirror then transform back to 1 (120 z rotation)
	// 0 2	.	.	MT1		120 z rotation
	// 0 3	.	-x	MT2		apply x axis mirror then transform back to 2 (240 z rotation)
	// 0 4	.	.	MT2		240 z rotation
	// 0 5	.	-x	.		apply axis mirror
	mtx_set_unity(pgm->subface[0].sub[0].m); //double m[16]
	mtx_multiply_matrix(mx, pgm->face.mt1, pgm->subface[0].sub[1].m);
	memcpy(pgm->subface[0].sub[2].m, pgm->face.mt1,sizeof(double)*16);
	mtx_multiply_matrix(mx, pgm->face.mt2, pgm->subface[0].sub[3].m);
	memcpy(pgm->subface[0].sub[4].m, pgm->face.mt2, sizeof(double)*16);
	memcpy(pgm->subface[0].sub[5].m, mx, sizeof(double)*16);
	// 1 0 	M1	-x	.
	// 1 1 	.	.	.
	// 1 2	M1	-x	MT1
	// 1 3	M1		MT2
	// 1 4	.	-x	.
	// 1 5 	M1	.	.
	mtx_multiply_matrix(pgm->face.m1, mx, pgm->subface[1].sub[0].m);
	mtx_set_unity(pgm->subface[1].sub[1].m);
	mtx_multiply_matrix(pgm->face.m1, mx, mm);
	mtx_multiply_matrix(mm, pgm->face.mt1, pgm->subface[1].sub[2].m);
	mtx_multiply_matrix(pgm->face.m1, pgm->face.mt2, pgm->subface[1].sub[3].m);
	memcpy( pgm->subface[1].sub[4].m, mx, sizeof(double) * 16);
	memcpy( pgm->subface[1].sub[5].m, pgm->face.m1, sizeof(double) * 16);
	// 2 0 	M1	.	.
	// 2 1	M1	-x	MT1
	// 2 2	.	.	.
	// 2 3	.	-x	. 
	// 2 4	M1	.	MT2
	// 2 5	M1	-x	.
	memcpy( pgm->subface[2].sub[0].m, pgm->face.m1, sizeof(double) * 16);
	mtx_multiply_matrix(pgm->face.m1, mx, mm);
	mtx_multiply_matrix(mm, pgm->face.mt1, pgm->subface[2].sub[1].m);
	mtx_set_unity(pgm->subface[2].sub[2].m);
	memcpy (pgm->subface[2].sub[3].m, mx, sizeof(double) * 16);
	mtx_multiply_matrix(pgm->face.m1, pgm->face.mt2, pgm->subface[2].sub[4].m);
	mtx_multiply_matrix(pgm->face.m1, mx, pgm->subface[2].sub[5].m);
	// 3 0 	M2	-x	.
	// 3 1 	M2	.	MT1
	// 3 2	.	-x	.
	// 3 3	.	.	.
	// 3 4	M2	-x	MT2
	// 3 5	M2	.	.
	mtx_multiply_matrix(pgm->face.m2, mx, pgm->subface[3].sub[0].m);
	mtx_multiply_matrix(pgm->face.m2, pgm->face.mt1, pgm->subface[3].sub[1].m);
	memcpy(pgm->subface[3].sub[2].m,mx, sizeof(double) * 16);
	mtx_set_unity(pgm->subface[3].sub[3].m);
	mtx_multiply_matrix(pgm->face.m2, mx, mm);
	mtx_multiply_matrix(mm, pgm->face.mt2, pgm->subface[3].sub[4].m);
	memcpy(pgm->subface[3].sub[5].m, pgm->face.m2, sizeof(double) * 16);
	// 4 0	M2	.	.
	// 4 1	.	-x	.
	// 4 2	M2	.	MT1
	// 4 3	M2	-x	MT2
	// 4 4	.	.	.
	// 4 5	M2	-x	.
	memcpy(pgm->subface[4].sub[0].m, pgm->face.m2, sizeof(double) * 16);
	memcpy( pgm->subface[4].sub[1].m, mx, sizeof(double) * 16);
	mtx_multiply_matrix(pgm->face.m2, pgm->face.mt1, pgm->subface[4].sub[2].m);
	mtx_multiply_matrix(pgm->face.m2, mx, mm);
	mtx_multiply_matrix(mm, pgm->face.mt2, pgm->subface[4].sub[3].m);
	mtx_set_unity(pgm->subface[4].sub[4].m);
	mtx_multiply_matrix(pgm->face.m2, mx, pgm->subface[4].sub[5].m);
	// 5 0	.	-x	.
	// 5 1	.	.	MT1
	// 5 2 	.	-x	MT1
	// 5 3	.	.	MT2
	// 5 4	.	-x	MT2
	// 5 5	.	.	.		
	memcpy(pgm->subface[5].sub[0].m,mx, sizeof(double) * 16);
	memcpy( pgm->subface[5].sub[1].m, pgm->face.mt1, sizeof(double) * 16);
	mtx_multiply_matrix(mx, pgm->face.mt1, pgm->subface[5].sub[2].m);
	memcpy( pgm->subface[5].sub[3].m, pgm->face.mt2, sizeof(double) * 16);
	mtx_multiply_matrix(mx, pgm->face.mt2, pgm->subface[5].sub[4].m);
	mtx_set_unity(pgm->subface[5].sub[5].m);
}

//---------------------------------------------------------------------------
void subface_exchange(int aSrc, int aDst, GUT_POINT *pSrc, GUT_POINT *pDst, SUBFACE *subface)
//---------------------------------------------------------------------------
{
	// transform point 'pSrc' from area 'aSrc' to point 'pDst' in area 'aDst' 
	mtx_vec4_multiply(1, (GUT_VECTOR*)pSrc, (GUT_VECTOR*)pDst, subface[aSrc].sub[aDst].m);
}

//---------------------------------------------------------------------------
void build_subface_transforms(PROGRAM *pgm)
//---------------------------------------------------------------------------
{
	// build simple z rotation matrices for 0, 120, and 240 degrees from vectors
	// Face areas: 0, 5 are in z rotation 0 
	// Face areas: 1, 2 are in z rotation 120
	// Face areas: 3, 4 are in z rotation 240 
	//
	GUT_POINT p[3];
	GUT_VECTOR vx, vy, vz;
	double m[16];

	// these points are used to derive the vectors
	// and are corners of equilateral triangle 
	// 
	// orientation
	//   1	  ^ local y axis 
	//	. .   |
	// 2 . 0  .----> local x axis
	//
	p[0].x = 0.5;	p[0].y = -sqrt(3.0) / 6;	p[0].z = 0;
	p[1].x = 0;		p[1].y =  sqrt(3.0) / 3;	p[1].z = 0;
	p[2].x = -0.5;	p[2].y = -sqrt(3.0) / 6;	p[2].z = 0;

	// z rotation matrix for 0 degrees and its transpose
	gut_vector(&p[2], &p[0], &vx);
	gut_normalize_vector(&vx);
	gut_vector(&p[2], &p[1], &vy);
	gut_normalize_vector(&vy);
	gut_cross_product(&vx, &vy, &vz);
	gut_normalize_vector(&vz);
	gut_cross_product(&vz, &vx, &vy);
	gut_normalize_vector(&vz);
	build_rotation_matrix(&vx, &vy, &vz, pgm->face.m0, pgm->face.mt0);

	// z rotation matrix for 120 degrees and its transpose
	gut_vector(&p[0], &p[1], &vx);
	gut_normalize_vector(&vx);
	gut_vector(&p[0], &p[2], &vy);
	gut_normalize_vector(&vy);
	gut_cross_product(&vx, &vy, &vz);
	gut_normalize_vector(&vz);
	gut_cross_product(&vz, &vx, &vy);
	gut_normalize_vector(&vy);
	build_rotation_matrix(&vx, &vy, &vz, pgm->face.m1, pgm->face.mt1);
	//mtx_create_rotation_matrix(m, MTX_ROTATE_Z_AXIS, DTR(240.0)); //MTX_MATRIX *m, int axis, double angle )

	// z rotation matrix for 240 degrees and its transpose
	gut_vector(&p[1], &p[2], &vx);
	gut_normalize_vector(&vx);
	gut_vector(&p[1], &p[0], &vy);
	gut_normalize_vector(&vy);
	gut_cross_product(&vx, &vy, &vz);
	gut_normalize_vector(&vz);
	gut_cross_product(&vz, &vx, &vy);
	gut_normalize_vector(&vy);
	build_rotation_matrix(&vx, &vy, &vz, pgm->face.m2, pgm->face.mt2);
}

//---------------------------------------------------------------------------
void build_face_transforms(PROGRAM *pgm)
//---------------------------------------------------------------------------
{
	GUT_SPHERICAL_COORD sc;
	GUT_POINT			vtx[6];

	// define each of the vertices of the icosahedral face that lies on the equator 
	// x axis aligned with the center of the face
	// use spherical coordinates to derive cartesian coordinates
	//
	sc.radius = 1;
	sc.inclination = atan(2.0);
	sc.azimuth = 0;
	gut_spherical_to_cartesian(&sc, &vtx[1]);
	sc.radius = 1;
	sc.inclination = DTR(180.0) - atan(2.0);
	sc.azimuth = DTR(-36.0);
	gut_spherical_to_cartesian(&sc, &vtx[2]);
	sc.radius = 1;
	sc.inclination = DTR(180.0) - atan(2.0);
	sc.azimuth = DTR(36.0);
	gut_spherical_to_cartesian(&sc, &vtx[0]);
	// this point is not used 
	sc.radius = 1;
	sc.inclination = atan(2.0);
	sc.azimuth = DTR(72.0);
	gut_spherical_to_cartesian(&sc, &vtx[3]);

	// create rotation transformation from the triangle from local to global position and back
	// local position is the icosahedral triangle on the equator with the x axis extending
	// out through the face, z axis is up, y axis is to the left and right 
	// 'tm' is the local to global transform matrix 
	// 'tmt' (transpose) is the global to local transform
	rotation_matrix_from_triangle(&vtx[0], &vtx[1], &vtx[2], pgm->face.tm, pgm->face.tmt);
}

//---------------------------------------------------------------------------
void rotation_matrix_from_triangle(GUT_POINT *p0, GUT_POINT *p1, GUT_POINT *p2, double mr[], double mrt[])
//---------------------------------------------------------------------------
{
	// create a three dimensional rotation matrix from a triangle in space
	// defined by three cartesian coordinates
	// NOTE: translation is not considered
	//
	GUT_POINT	t;
	GUT_VECTOR	x, y, z;

	// find center of the initial points
	t.x = (p0->x + p1->x + p2->x) / 3;
	t.y = (p0->y + p1->y + p2->y) / 3;
	t.z = (p0->z + p1->z + p2->z) / 3;
	t.w = (p0->w + p1->w + p2->w) / 3;

	// orientation
	//  1
	// 2 0  ----> local x axis

	// compute local coordinate axii (x,y,z)
	gut_vector(p2, p0, &x);
	gut_vector(p2, p1, &y);
	gut_normalize_vector(&x);
	gut_normalize_vector(&y);
	gut_cross_product(&x, &y, &z);
	gut_normalize_vector(&z);
	gut_cross_product(&z, &x, &y);
	gut_normalize_vector(&y);

	// create rotation matrix based on the local coordinate axii
	mr[0] = x.i; mr[1] = y.i; mr[2] = z.i; mr[3] = 0;
	mr[4] = x.j; mr[5] = y.j; mr[6] = z.j; mr[7] = 0;
	mr[8] = x.k; mr[9] = y.k; mr[10] = z.k; mr[11] = 0;
	mr[12] = 0; mr[13] = 0; mr[14] = 0; mr[15] = 1;
	memcpy(mrt, mr, sizeof(double) * 16); // copy the matrix for the transpose
	mtx_transpose_matrix(mrt);
}

//--------------------------------------------------------------------
double build_loop( BUILD *buildFunction, double *seed, double r_zero, void *var)
//--------------------------------------------------------------------
{
	// This is an iteration function that calls a supplied function 'build' with a 'seed' value
	// and returns a 'difference' that is compared with the 'r_zero' value. When the 'difference'
	// value is within the tolerance of 'r_zero' the build_loop function is finished. 
	// The 'var' argument is supplied to the 'build' function for any necessary purpose (state) 
	// The original 'seed' value is modified and the final value represents the solution 
	//
	// 'build' is the function pointer that is called and should return a double value 
	// 'seed' is the value passed to the 'build' function to base its computations
	//		(this value will change)
	// 'r_zero' is the  

	// interative loop to find correct seed value
	double	inc;
	double	zero;
	int		max;

	max = 200; // avoid infinite loop
	inc = DTR(0.5);
	zero = r_zero;  

	struct {
		double	delta;
		double	tolerance;
		int		loop;
		double	lastdiff;
		int		dir;
	} prob;
	double	diff;

	// initialize loop structure
	prob.delta = inc;
	prob.tolerance = zero;
	prob.loop = 0;
	prob.lastdiff = 0;
	prob.dir = 1;

	// solution loop
	while (1)
	{	
		// increment the solution counter
		++prob.loop;

		// the function to build the grid
		diff = buildFunction(seed, var);
		//		printf ( "inner loop: %5d seed %f diff %f delta %f\n", prob.loop, seed, diff, prob.delta )

		if (prob.loop == max)
		{
			printf("NOTE:inner_loop_exceeded max, current_diff= %10.8f\n", diff);
			return -1.0;
		}

		// check to determine how close to correct
		if (fabs(diff) <= prob.tolerance)
		{	
			//			printf ( "inner loop: seed = %f diff = %f\n", seed, diff )
			//			format "iterations: %d\n" prob.loop
			break; // exit the while loop since we are done
		}

		if (diff > 0.0)
		{
			if (prob.lastdiff > 0.0)
			{
				if (diff < prob.lastdiff)
				{
					*seed += prob.delta;
				}
				else
				{
					*seed -= prob.delta;
					prob.delta *= -1;
					*seed += prob.delta;
				}
			}
			else if (prob.lastdiff < 0.0)
			{
				*seed -= prob.delta;
				prob.delta /= -2.0;
				*seed += prob.delta;
			}
			else
			{
				*seed += prob.delta;
			}
		}
		else if (diff < 0.0)
		{
			if (prob.lastdiff < 0.0)
			{
				if (diff > prob.lastdiff)
				{
					*seed += prob.delta;
				}
				else
				{
					*seed -= prob.delta;
					prob.delta *= -1;
					*seed += prob.delta;
				}
			}
			else if (prob.lastdiff > 0.0)
			{
				*seed -= prob.delta;
				prob.delta /= -2.0;
				*seed += prob.delta;
			}
			else
			{
				*seed += prob.delta;
			}
		}
		prob.lastdiff = diff;
	}

	return fabs(diff);
}

//---------------------------------------------------------------------------
void vertex_by_strig ( SPH_TRI *st, double b, double c, double C, GUT_SPHERICAL_COORD *sc, GUT_POINT *p)
//---------------------------------------------------------------------------
{
	// use spherical trigonometry to solve an oblique triangle definition given bcC angles

	st->a = st->b = st->c = st->A = st->B = st->C = 0; // clear memory of the spherical triangle structure
	/*
		.
		|.
		| .
	 b  |A .
		|   . c
		| C  .
		|  . a  
		.
	*/
	// assign provided values to triangle structure
	st->b = b;
	st->c = c;
	st->C = C;
	sph_tri_bcC(st); // requires oblique spherical triangle solution
	// upon completion convert the 'c' to 'inclination' and 'A' to 'azimuth'
	// to compute a cartesian coordinate
	sc->radius      = 1;
	sc->inclination = st->c;
	sc->azimuth     = st->A;
	gut_spherical_to_cartesian(sc, p); //GUT_SPHERICAL_COORD, GUT_POINT 
}

//---------------------------------------------------------------------------
double asin_clamp(double equation)
//---------------------------------------------------------------------------
{
	if (equation > 1.0 || equation < -1.0)
	{
		;
	}

	// decide which clamp to use
	if (equation > 1.0)
	{
		equation = 1.0;
	}
	else if (equation < -1.0)
	{
		equation = -1.0;
	}

	return asin(equation);
}

//---------------------------------------------------------------------------
void sph_tri_bcC(SPH_TRI *st)
//---------------------------------------------------------------------------
{
	// Solve oblique spherical triangle (more complex)
	//
	double c, C, b, A, A2, B, B2, v, a;
	b = st->b;
	c = st->c;
	C = st->C;

	if (b > c && C < DTR(90.0))
	{	
		// complex case
		B = asin(sin(C)*sin(b) / sin(c));
		B2 = DTR(180.0) - B;

		v = tan((C - B) / 2.0)*sin((c + b) / 2.0) / sin((c - b) / 2.0);
		A = atan(1 / v) * 2.0;

		v = tan((C - B2) / 2.0)*sin((c + b) / 2.0) / sin((c - b) / 2.0);
		A2 = atan(1 / v) * 2.0;

		if (A < 0)
		{
			A = A2;
			B = B2;
		}
		st->B = B;
		st->A = A;
		st->a = asin(sin(A)*sin(b) / sin(B));
	}
	else
	{
		B = asin_clamp(sin(b) * sin(C) / sin(c));
		// Napier's Analogies
		a = 2.0 * atan(tan((b + c) / 2.0) * cos((B + C) / 2.0) / cos((B - C) / 2.0));
		A = acos((cos(a) - cos(b) * cos(c)) / (sin(b) * sin(c)));
		st->B = B;
		st->A = A;
		st->a = a;
	}
}

//---------------------------------------------------------------------------
void create_vertex_by_strig(PROGRAM *pgm, int v, int a, double b, double c, double C)
//---------------------------------------------------------------------------
{
	// Compute position of a vertex using spherical trigonometry 
	// with an oblique spherical triangle definition
	GUT_POINT			*p;
	SPH_TRI				st; // temporary 
	GUT_SPHERICAL_COORD	*sc;

	// assign pointer
	p  = &pgm->v[v].p[a];
	sc = &pgm->v[v].sc[a];

	st.a = st.b = st.c = st.A = st.B = st.C = 0; // clear memory of the spherical triangle structure
	st.b = b;
	st.c = c;
	st.C = C;
	sph_tri_bcC(&st);

	sc->radius = 1;
	sc->inclination = st.c;
	sc->azimuth = st.A;

	// compute the cartesian coordinates from the spherical coordinates
	gut_spherical_to_cartesian(sc, p);  
	
	// generate all of the same vertex in the other symmetrical positions in the icosa face
	generate_all_vertices(pgm, v, a);
}

//---------------------------------------------------------------------------
void create_vertex_by_sc(PROGRAM *pgm, int v, int a, double azimuth, double inclination)
//---------------------------------------------------------------------------
{
	// Compute position of a vertex using spherical coordinates 										
	// radius is one by default
	GUT_POINT			*p;
	GUT_SPHERICAL_COORD	*sc;

	// assign pointers
	p = &pgm->v[v].p[a];
	sc = &pgm->v[v].sc[a];

	// set up pointers 
	p = &pgm->v[v].p[a];
	sc = &pgm->v[v].sc[a];

	sc->radius = 1;
	sc->inclination = inclination;
	sc->azimuth = azimuth;

	// compute the cartesian coordinates from the spherical coordinates
	gut_spherical_to_cartesian(sc, p); //GUT_SPHERICAL_COORD, GUT_POINT 

	// generate all of the same vertex in the other symmetrical positions in the icosa face
	generate_all_vertices(pgm, v, a);
}

//---------------------------------------------------------------------------
void create_vertex_from_vertex(PROGRAM *pgm, int vd, int ad, int vs, int as, double b, double C)
//---------------------------------------------------------------------------
{
	// Create a new vertex based on b,C spherical triangle components provided
	// and c is the inclination angle from the source vertex
	// vd - destination vertex id
	// ad - destination vertex LCD area index
	// vs - source vertex id 
	// as - source vertex LCD area index
	create_vertex_by_strig(pgm, vd, ad, b, pgm->v[vs].sc[as].inclination, C);
}

//---------------------------------------------------------------------------
void classI_2v_output(PROGRAM *pgm, char *filename)
//---------------------------------------------------------------------------
{
	GUT_POINT	lp[20], gp[20];
	FILE		*fp;

	// copy required vertices (cartesian coords) to lp array
	lp[0] = pgm->v[0].p[0];
	lp[1] = pgm->v[1].p[0];
	lp[2] = pgm->v[0].p[4];
	lp[3] = pgm->v[0].p[1];

	// transform points to local position based on its triangle
	mtx_vec4_multiply(4, (GUT_VECTOR*)lp, (GUT_VECTOR*)gp, pgm->face.tm);

	// open output file
	fopen_s(&fp, filename, "w");
	if (!fp)
	{	//echo ERROR
		return;
	}

	printf("\tGeometry output: %s\n", filename);
	// output the OFF geometry file for all the triangles that cover
	// LCD area 0 of the global icosahedron face - z axis aligned
	int		i;
	fprintf(fp, "OFF\n");
	fprintf(fp, "4 2 0\n");
	for (i = 0; i < 4; ++i)
	{
		fprintf(fp, "%12.9f %12.9f %12.9f \n", gp[i].x, gp[i].y, gp[i].z);
	}
	fprintf(fp, "3 0 1 3 \n");
	fprintf(fp, "3 0 3 2 \n");
	fclose(fp);
}

//---------------------------------------------------------------------------
void classI_2v(PROGRAM *pgm)
//---------------------------------------------------------------------------
{
	// This is the simplest truncatable configuration - a stantard 2 frequency icosahedron
	printf("Class I Icosahedron (2,0) - compute truncation configuration\n");

	// create vertex 2,0 by spherical coordinates
	// no dependency
	create_vertex_by_sc(pgm, 0, 0, 0, pgm->ref.b * 2 + (pgm->ref.c + pgm->ref.a));

	// create vertex 1,0 by spherical coordinates
	// no dependency
	create_vertex_by_sc(pgm, 1, 0, DTR(36.0), (pgm->ref.c + pgm->ref.a) * 2);
}

//---------------------------------------------------------------------------
void classI_3v_output(PROGRAM *pgm, char *filename)
//---------------------------------------------------------------------------
{
	// This is the truncatable configuration - a stantard 3 frequency icosahedron
	GUT_POINT	lp[20], gp[20];
	FILE *fp;

	lp[0] = pgm->v[0].p[5];
	lp[1] = pgm->v[0].p[0];
	lp[2] = pgm->v[1].p[0];
	lp[3] = pgm->v[2].p[0];
	lp[4] = pgm->v[0].p[1] ;

	// transform points to local position based on its triangle
	mtx_vec4_multiply(5, (GUT_VECTOR*)lp, (GUT_VECTOR*)gp, pgm->face.tm);

	fopen_s(&fp, filename, "w");
	if (!fp)
	{//echo ERROR
		return;
	}

	printf("\tGeometry output: %s\n", filename);
	int		i;
	fprintf(fp, "OFF\n");
	fprintf(fp, "5 3 0\n");
	for (i = 0; i < 5; ++i)
	{
		fprintf(fp, "%12.9f %12.9f %12.9f \n", gp[i].x, gp[i].y, gp[i].z);
	}
	fprintf(fp, "3 0 1 3 \n");
	fprintf(fp, "3 1 4 3 \n");
	fprintf(fp, "3 1 2 4 \n");
	fclose(fp);
}

//---------------------------------------------------------------------------
void classI_3v(PROGRAM *pgm)
//---------------------------------------------------------------------------
{
	// This is the truncatable configuration - a stantard 3 frequency icosahedron
	// There are no unknown dependencies
	printf("Class I Icosahedron (2,0) - compute truncation configuration\n");

	// create vertex 2,0 by spherical coordinates
	// no dependency
	create_vertex_by_sc(pgm, 2, 0, 0, pgm->ref.b * 2 + pgm->ref.c);

	// create vertex 1,0 by spherical coordinates
	// no dependency
	create_vertex_by_sc(pgm, 1, 0, DTR(36.0), (pgm->ref.c + pgm->ref.a) * 2);

	// create vertex 0,1 by spherical trig from 2,0
	create_vertex_from_vertex(pgm, 0, 1, 2, 0, pgm->ref.b * 2, DTR(144.0));
}

//---------------------------------------------------------------------------
void classI_4v_output(PROGRAM *pgm, char *filename)
//---------------------------------------------------------------------------
// This is the truncatable configuration - a stantard 4 frequency icosahedron
// There are no unknown dependencies
{
	GUT_POINT	lp[20], gp[20];
	FILE *fp;

	lp[0] = pgm->v[0].p[0];
	lp[1] = pgm->v[1].p[0];
	lp[2] = pgm->v[2].p[0];
	lp[3] = pgm->v[3].p[5];
	lp[4] = pgm->v[3].p[0];
	lp[5] = pgm->v[1].p[1];
	lp[6] = pgm->v[3].p[2] ;

	// transform points to local position based on its triangle
	mtx_vec4_multiply(7, (GUT_VECTOR*)lp, (GUT_VECTOR*)gp, pgm->face.tm);

	fopen_s(&fp, filename, "w");
	if (!fp)
	{ //echo ERROR
		return;
	}

	printf("\tGeometry output: %s\n", filename);
	int		i;
	fprintf(fp, "OFF\n");
	fprintf(fp, "7 5 0\n");
	for (i = 0; i < 7; ++i)
	{
		fprintf(fp, "%12.9f %12.9f %12.9f \n", gp[i].x, gp[i].y, gp[i].z);
	}
	fprintf(fp, "3 0 4 3 \n");
	fprintf(fp, "3 0 1 4 \n");
	fprintf(fp, "3 1 5 4 \n");
	fprintf(fp, "3 1 2 5 \n");
	fprintf(fp, "3 3 4 6 \n");
	fclose(fp);
}

//---------------------------------------------------------------------------
void classI_4v(PROGRAM *pgm)
//---------------------------------------------------------------------------
{
	// This is the truncatable configuration - a stantard 4 frequency icosahedron
	// There are no unknown dependencies
	printf("Class I Icosahedron (4,0) - compute truncation configuration\n");

	// create vertex 2,0 by spherical coordinates
	// no dependency
	create_vertex_by_sc(pgm, 2, 0, DTR(36.0), (pgm->ref.c + pgm->ref.a) * 2);

	// create vertex 0,1 by spherical coordinates
	create_vertex_by_strig(pgm, 0, 1, pgm->ref.b * 2, DTR(90.0), DTR(144.0));
	// or alternative since result is equivalent
	// create_vertex_by_sc ( pgm, 0, 1, 18, 90 )

	// create vertex 3,2 by spherical trig from 0,2
	create_vertex_by_sc(pgm, 3, 2, 0, DTR(90.0));

	// create vertex 1,1 by spherical trig from 3,0
	create_vertex_from_vertex(pgm, 1, 1, 3, 0, pgm->ref.b * 2, DTR(144.0));
}

//---------------------------------------------------------------------------
void classI_5v_output(PROGRAM *pgm, char *filename)
//---------------------------------------------------------------------------
{
	GUT_POINT	lp[20], gp[20];
	FILE *fp;

	lp[0] = pgm->v[0].p[5];
	lp[1] = pgm->v[0].p[0];
	lp[2] = pgm->v[1].p[0];
	lp[3] = pgm->v[2].p[0];
	lp[4] = pgm->v[3].p[0];
	lp[5] = pgm->v[4].p[0];
	lp[6] = pgm->v[1].p[1];
	lp[7] = pgm->v[3].p[3];
	lp[8] = pgm->v[3].p[1] ;

	// transform points to local position based on its triangle
	mtx_vec4_multiply(9, (GUT_VECTOR*)lp, (GUT_VECTOR*)gp, pgm->face.tm);

	fopen_s(&fp, filename, "w");
	if (!fp)
	{//echo ERROR
		return;
	}

	printf("\tGeometry output: %s\n", filename);
	int		i;
	fprintf(fp, "OFF\n");
	fprintf(fp, "9 7 0\n");
	for (i = 0; i < 9; ++i)
		fprintf(fp, "%12.9f %12.9f %12.9f \n", gp[i].x, gp[i].y, gp[i].z);
	fprintf(fp, "3 0 1 4 \n");
	fprintf(fp, "3 1 5 4 \n");
	fprintf(fp, "3 1 2 5 \n");
	fprintf(fp, "3 2 6 5 \n");
	fprintf(fp, "3 2 3 6 \n");
	fprintf(fp, "3 4 5 8 \n");
	fprintf(fp, "3 4 8 7 \n");
	fclose(fp);
}

//---------------------------------------------------------------------------
double classI_5v(double *var, void *program)
//---------------------------------------------------------------------------
{
	// This is the truncatable configuration - a stantard 5 frequency icosahedron
	// There is one unknown variable to determine 
	// There is a single solution
	PROGRAM	*pgm = (PROGRAM*)program;
	double diff;

	// create vertex 3,0 by spherical coordinates
	create_vertex_by_sc(pgm, 3, 0, 0, pgm->ref.b * 2 + pgm->ref.c + *var);

	// create vertex 4,0 by spherical trig from 3,0
	create_vertex_from_vertex(pgm, 4, 0, 3, 0, pgm->ref.b * 2 + pgm->ref.c, DTR(120.0));

	// create vertex 1,1 by spherical trig from 3,0
	create_vertex_from_vertex(pgm, 1, 1, 3, 0, pgm->ref.b * 2, DTR(144.0));

	// create vertex 0,1 by spherical trig from 3,1
	create_vertex_from_vertex(pgm, 0, 1, 3, 1, pgm->ref.b * 2, DTR(144.0));

	// create vertex 2,0 by spherical coordinates
	// no dependency
	create_vertex_by_sc(pgm, 2, 0, DTR(36.0), (pgm->ref.c + pgm->ref.a) * 2);

	// determine required difference to compare 2 vertices that must lie at the same inclination
	diff = pgm->v[4].sc[2].inclination - pgm->v[0].sc[2].inclination;
	//printf ( "%12.9f %12.9f %12.9f %12.9f\n", var, pgm->v[4].sc[2].inclination, pgm->v[0].sc[2].inclination, diff )

	return diff;
}

//---------------------------------------------------------------------------
double classI_7v_a(double *var, void *program)
//---------------------------------------------------------------------------	
{
	// This is the truncatable configuration - a 7 frequency icosahedron
	// There is no solution for all levels to be lesser/greater circles 
	// There is are 3 solutions where each has 2 levels that are planar
	PROGRAM	*pgm = (PROGRAM*)program;
	double diff;

	// non-dependent position
	// create vertex 3,0 by spherical coordinates 
	create_vertex_by_sc(pgm, 3, 0, DTR(36.0), (pgm->ref.a + pgm->ref.c) * 2);

	// PART 1 ==================================================================
	// create vertex 7,2 by spherical coordinates 
	create_vertex_by_sc(pgm, 7, 2, 0, pgm->ref.b * 2 + pgm->ref.c - *var);

	// create vertex 4,2 by association trig
	create_vertex_from_vertex(pgm, 4, 2, 7, 2, pgm->ref.b * 2 + pgm->ref.c, DTR(60.0));

	// create vertex 0,1 by association trig
	create_vertex_from_vertex(pgm, 0, 1, 7, 2, pgm->ref.b * 2, DTR(144.0));

	// PART 2 ==================================================================
	// create vertex 1,1 by association trig
	create_vertex_from_vertex(pgm, 1, 1, 7, 1, pgm->ref.b * 2, DTR(144.0));

	// PART 3 ==================================================================
	// create vertex 6,0 by association trig
	create_vertex_from_vertex(pgm, 6, 0, 4, 0, pgm->ref.b * 2 + pgm->ref.c, DTR(120.0));

	// create vertex 2,1 by association trig
	create_vertex_from_vertex(pgm, 2, 1, 4, 0, pgm->ref.b * 2, DTR(144.0));

	// return the difference in inclination between 6,2 and 1,2
	diff = pgm->v[6].sc[2].inclination - pgm->v[1].sc[2].inclination;

	return diff;
}

//---------------------------------------------------------------------------
double classI_7v_b1(double *var, PROGRAM *pgm)
//---------------------------------------------------------------------------
{
	double diff;

	// PART 4 ==================================================================
	// create vertex 5,2 by spherical coordinates to match inclination of vertex 0,2
	create_vertex_by_sc(pgm, 5, 2, *var, pgm->v[0].sc[2].inclination);

	// version 1
	// return the difference in inclination between vertex 5,1 and vertex 1,1
	diff = pgm->v[5].sc[1].inclination - pgm->v[1].sc[1].inclination;

	//	echo diff $diff
	return diff;
}

//---------------------------------------------------------------------------
double classI_7v_b2(double *var, PROGRAM *pgm)
//---------------------------------------------------------------------------
{
	double diff;

	// PART 4 ==================================================================
	// create vertex 5,2 by spherical coordinates to match inclination of vertex 0,2
	create_vertex_by_sc(pgm, 5, 2, *var, pgm->v[0].sc[2].inclination);

	// version 2 
	// return the difference in inclination between v(5,0) and v(4,0)
	diff = pgm->v[5].sc[0].inclination - pgm->v[4].sc[0].inclination;
	//	echo diff $diff
	return diff;
}

//---------------------------------------------------------------------------
double classI_7v_b3(double *var, PROGRAM *pgm)
//---------------------------------------------------------------------------
{
	double diff;

	// PART 4 ==================================================================
	// create vertex 7,0 by spherical coordinates to match inclination of vertex 2,0
	create_vertex_by_sc(pgm, 5, 0, *var, pgm->v[4].sc[0].inclination);

	// version 2 
	// return the difference in inclination between v(5,1) and v(1,1)
	diff = pgm->v[5].sc[1].inclination - pgm->v[1].sc[1].inclination;
	//	echo diff $diff
	return diff;
}


//---------------------------------------------------------------------------
void classI_7v_details(PROGRAM *pgm)
//---------------------------------------------------------------------------
{
	printf(" 2,3 2,2          %12.9f  %12.9f \n", 
		RTD(pgm->v[2].sc[3].inclination), RTD(pgm->v[2].sc[2].inclination));
	printf(" 6,2 1,2          %12.9f  %12.9f \n", 
		RTD(pgm->v[6].sc[2].inclination), RTD(pgm->v[1].sc[2].inclination));
	printf(" 5,2 0,2          %12.9f  %12.9f \n", 
		RTD(pgm->v[5].sc[2].inclination), RTD(pgm->v[0].sc[2].inclination));
	printf(" 7,2 4,2 0,1      %12.9f  %12.9f  %12.9f \n", 
		RTD(pgm->v[7].sc[2].inclination), RTD(pgm->v[4].sc[2].inclination), RTD(pgm->v[0].sc[1].inclination));
	printf(" 7,1 5,1 1,1      %12.9f  %12.9f  %12.9f \n", 
		RTD(pgm->v[7].sc[1].inclination), RTD(pgm->v[5].sc[1].inclination), RTD(pgm->v[1].sc[1].inclination));
	printf(" 4,0 5,0 6,0 2,1  %12.9f  %12.9f  %12.9f  ", 
		RTD(pgm->v[4].sc[0].inclination), RTD(pgm->v[5].sc[0].inclination), RTD(pgm->v[6].sc[0].inclination));
	printf(" %12.9f  \n", RTD(pgm->v[2].sc[1].inclination));
}

//---------------------------------------------------------------------------
void classI_7v_output(PROGRAM *pgm, char *filename)
//---------------------------------------------------------------------------
{
	int		i;
	struct {
		int		v, a;
	}p[20];
	FILE *fp;

	fopen_s(&fp, filename, "w");
	if (!fp)
	{//echo ERROR
		return;
	}
	printf("\tGeometry output: %s\n", filename);

	p[0].v = 0, p[0].a = 5;
	p[1].v = 0, p[1].a = 0;
	p[2].v = 1, p[2].a = 0;
	p[3].v = 2, p[3].a = 0;
	p[4].v = 3, p[4].a = 0;

	p[5].v = 4, p[5].a = 0;
	p[6].v = 5, p[6].a = 0;
	p[7].v = 6, p[7].a = 0;
	p[8].v = 2, p[8].a = 1;

	p[9].v  = 7, p[9].a  = 5;
	p[10].v = 7, p[10].a = 0;
	p[11].v = 5, p[11].a = 1;
	p[12].v = 7, p[12].a = 2;

	GUT_POINT	lp[20], gp[20];

	for (i = 0; i < 13; ++i)
	{
		lp[i] = pgm->v[p[i].v].p[p[i].a];
	}

	// transform points to local position based on its triangle
	mtx_vec4_multiply(13, (GUT_VECTOR*)lp, (GUT_VECTOR*)gp, pgm->face.tm);

	fprintf(fp, "OFF\n");
	fprintf(fp, "13 12 0\n");
	for (i = 0; i < 13; ++i)
		fprintf(fp, "%12.9f %12.9f %12.9f \n", gp[i].x, gp[i].y, gp[i].z);

	fprintf(fp, "3 0 1 5 \n");
	fprintf(fp, "3 1 6 5 \n");
	fprintf(fp, "3 1 2 6 \n");
	fprintf(fp, "3 2 7 6 \n");
	fprintf(fp, "3 2 3 7 \n");
	fprintf(fp, "3 3 8 7 \n");
	fprintf(fp, "3 3 4 8 \n");

	fprintf(fp, "3 5 10  9 \n");
	fprintf(fp, "3 5  6 10 \n");
	fprintf(fp, "3 6 11 10 \n");
	fprintf(fp, "3 6  7 11 \n");

	fprintf(fp, "3 9 10 12 \n");

	fclose(fp);
}

//---------------------------------------------------------------------------
double classI_6v_a(double *var, PROGRAM *pgm)
//---------------------------------------------------------------------------
{
	double  diff;

	// PART 3 ==================================================================
	// create vertex 4,0 by spherical coordinates 
	// using inclination of vertex 5,0 and 'A' being variable
	create_vertex_by_sc(pgm, 4, 0, *var, pgm->v[5].sc[0].inclination);

	// return difference in inclination between vertex 4,1 and vertex 1,1
	diff = pgm->v[4].sc[1].inclination - pgm->v[1].sc[1].inclination;

	return diff;
}

//---------------------------------------------------------------------------
double classI_6v_b(double *var, PROGRAM *pgm)
//---------------------------------------------------------------------------
{
	double  diff;

	// PART 3 ==================================================================
	// create vertex 4,0 by spherical coordinates 
	// using inclination of vertex 5,0 and 'A' being variable
	create_vertex_by_sc(pgm, 4, 0, *var, pgm->v[5].sc[0].inclination);

	// return difference in inclination between vertex 4,2 and vertex 0,1
	diff = pgm->v[4].sc[2].inclination - pgm->v[0].sc[1].inclination;

	return diff;
}

//---------------------------------------------------------------------------
void classI_6v(PROGRAM *pgm)
//---------------------------------------------------------------------------
{
	// PART 1 ==================================================================
	// create all the non-dependent vertices

	// create vertex 0,0 by spherical coordinates 
	create_vertex_by_sc(pgm, 0, 0, 0, pgm->ref.b * 2 + pgm->ref.c + pgm->ref.a);
	// create vertex 6,0 by spherical coordinates 
	create_vertex_by_sc(pgm, 6, 0, 0, pgm->ref.b * 2 + pgm->ref.c);
	// create vertex 3,0 by spherical coordinates 
	create_vertex_by_sc(pgm, 3, 0, DTR(36.0), (pgm->ref.c + pgm->ref.a) * 2);

	// PART 2 ==================================================================
	// create dependent vertices
	// create vertex 1,1 by association trig
	create_vertex_from_vertex(pgm, 1, 1, 6, 0, pgm->ref.b * 2, DTR(144.0));

	// create vertex 5,2 by spherical coordinates 
	create_vertex_by_sc(pgm, 5, 2, 0, pgm->v[1].sc[2].inclination);

	// create vertex 2,1 by association trig
	create_vertex_from_vertex(pgm, 2, 1, 5, 0, pgm->ref.b * 2, DTR(144.0));

	// solution is incomplete
}

//---------------------------------------------------------------------------
void classI_6v_output(PROGRAM *pgm, char *filename)
//---------------------------------------------------------------------------
{
	// This is a typical output 
	// All the vertices in area 0 of the local equitorial icosa triangle
	// are referenced and copied into local vertex memory (lp[]) then 
	// transformed into global (z up) position (gp[]). The global vertex 
	// positions are output with connectivity information into the 
	// referenced OFF file. The OFF geometry completely covers the area 0 
	// of the global icosahedron triangle.
	//
	int		i;
	struct { //p[20]
		int		v, a;
	}p[20];
	FILE *fp;

	fopen_s(&fp, filename, "w");
	if (!fp)
	{ //echo ERROR
		return;
	}
	printf("\tGeometry output: %s\n", filename);

	p[0].v = 0, p[0].a = 0;
	p[1].v = 1, p[1].a = 0;
	p[2].v = 2, p[2].a = 0;
	p[3].v = 3, p[3].a = 0;

	p[4].v = 4, p[4].a = 5;
	p[5].v = 4, p[5].a = 0;
	p[6].v = 5, p[6].a = 0;
	p[7].v = 2, p[7].a = 1;

	p[8].v = 6, p[8].a = 0;
	p[9].v = 4, p[9].a = 1;

	GUT_POINT	lp[20], gp[20];

	for (i = 0; i < 10; ++i)
		lp[i] = pgm->v[p[i].v].p[p[i].a];

	// transform points to local position based on its triangle
	mtx_vec4_multiply(10, (GUT_VECTOR*)lp, (GUT_VECTOR*)gp, pgm->face.tm);

	fprintf(fp, "OFF\n");
	fprintf(fp, "10 9 0\n");
	for (i = 0; i < 10; ++i)
		fprintf(fp, "%12.9f %12.9f %12.9f \n", gp[i].x, gp[i].y, gp[i].z);

	fprintf(fp, "3 0 1 5 \n");
	fprintf(fp, "3 0 5 4 \n");
	fprintf(fp, "3 1 2 6 \n");
	fprintf(fp, "3 1 6 5 \n");
	fprintf(fp, "3 2 3 7 \n");
	fprintf(fp, "3 2 7 6 \n");
	fprintf(fp, "3 4 5 8 \n");
	fprintf(fp, "3 5 9 8 \n");
	fprintf(fp, "3 5 6 9 \n");

	fclose(fp);
}

//---------------------------------------------------------------------------
void classI_2v_solution(PROGRAM *pgm, char *base_filename)
//---------------------------------------------------------------------------	
{
	char	filename_specific[128];

	// find geometry - no variable required
	classI_2v(pgm);

	sprintf_s(filename_specific, 128, "%s.off", base_filename);
	classI_2v_output(pgm, filename_specific);
}

//---------------------------------------------------------------------------
void classI_3v_solution(PROGRAM *pgm, char *base_filename)
//---------------------------------------------------------------------------
{
	char	filename_specific[128];

	// find geometry - no variable required
	classI_3v(pgm);

	sprintf_s(filename_specific, 128, "%s.off", base_filename);
	classI_3v_output(pgm, filename_specific);
}

//---------------------------------------------------------------------------
void classI_4v_solution(PROGRAM *pgm, char *base_filename)
//---------------------------------------------------------------------------
{

	char	filename_specific[128];

	// find geometry - no variable required
	classI_4v(pgm);

	sprintf_s(filename_specific, 128, "%s.off", base_filename);
	classI_4v_output(pgm, filename_specific);
}

//---------------------------------------------------------------------------
void classI_5v_solution(PROGRAM *pgm, char *base_filename)
//---------------------------------------------------------------------------
{
	double	seed = DTR(9.0);
	char	filename_specific[128];

	printf("Class I Icosahedron (5,0) - compute truncation configuration\n");

	// find initial geometry - single variable to be resolved
	build_loop(classI_5v, &seed, 0.00000000001, (void*)pgm);

	//	classI_5v_details ( pgm )
	sprintf_s(filename_specific, 128, "%s.off", base_filename);
	classI_5v_output(pgm, filename_specific);
}

//---------------------------------------------------------------------------
void classI_6v_solution(PROGRAM *pgm, char *base_filename)
//---------------------------------------------------------------------------
{
	double	seed = DTR(5.0);
	char	filename_specific[128];

	// version A
	printf("Class I Icosahedron (6,0) - compute truncation configuration (A)\n");
	// find initial geometry - single variable to be resolved
	classI_6v(pgm);
	build_loop((BUILD*)classI_6v_a, &seed, 0.00000000001, (void*)pgm);

	sprintf_s(filename_specific, 128, "%s_a.off", base_filename);
	classI_6v_output(pgm, filename_specific);

	// version B
	printf("Class I Icosahedron (6,0) - compute truncation configuration (B)\n");
	// find initial geometry - single variable to be resolved
	classI_6v(pgm);
	seed = DTR(6.0);
	build_loop((BUILD*)classI_6v_b, &seed, 0.00000000001, (void*)pgm);

	sprintf_s(filename_specific, 128, "%s_b.off", base_filename);
	classI_6v_output(pgm, filename_specific);
}

//---------------------------------------------------------------------------
void classI_7v_solution(PROGRAM *pgm, char *base_filename)
//---------------------------------------------------------------------------
{
	double  seed;
	char	filename_specific[128];

	// find initial geometry 
	seed = DTR(5.5);
	build_loop((BUILD*)classI_7v_a, &seed, 0.00000000001, (void*)pgm);
//	classI_7v_details(pgm);

	// need to try to fit final vertex
	//echo "Class I (7,0) Version 1"
	printf("Class I Icosahedron (7,0) - compute truncation configuration (A)\n");
	seed = DTR(4.0);
	build_loop((BUILD*)classI_7v_b1, &seed, 0.00000000001, (void*)pgm);
//	classI_7v_details(pgm);
	sprintf_s(filename_specific, 128, "%s_a.off", base_filename);
	classI_7v_output(pgm, filename_specific);

	//echo "Class I (7,0) Version 2"
	printf("Class I Icosahedron (7,0) - compute truncation configuration (B)\n");
	seed = DTR(4.0);
	build_loop((BUILD*)classI_7v_b2, &seed, 0.00000000001, (void*)pgm);
	//classI_7v_details ( pgm )
	sprintf_s(filename_specific, 128, "%s_b.off", base_filename);
	classI_7v_output(pgm, filename_specific);

	//echo "Class I (7,0) Version 3"
	printf("Class I Icosahedron (7,0) - compute truncation configuration (C)\n");
	seed = DTR(4.0);
	build_loop((BUILD*)classI_7v_b3, &seed, 0.00000000001, (void*)pgm);
	//classI_7v_details ( pgm )
	sprintf_s(filename_specific, 128, "%s_c.off", base_filename);
	classI_7v_output(pgm, filename_specific);
}

//---------------------------------------------------------------------------
int main(int ac, char **av)
//---------------------------------------------------------------------------
{
	// create the program structure
	PROGRAM	pgm;

	// define the global program structure

	// build the rotation matrices for icosahedral face on the equator
	build_face_transforms(&pgm);

	// build the 3 subface global rotations (0,120,240 degree rotations on the z axis) 
	build_subface_transforms(&pgm);

	// build all the matrices to transition a point in any section a(0-5) to any other section a(0-5)
	build_a_transforms(&pgm);

	// run a simple test to check the results of using the 'a' transforms
	//	test_subface_transforms ( pgm )

	// initialize reference LCD triangle for icosahedron
	pgm.ref.A = DTR(36.0);
	pgm.ref.B = DTR(60.0);
	// cos A = cos a sin B
	pgm.ref.a = acos(cos(pgm.ref.A) / sin(pgm.ref.B));
	// cos B = cos b sin A
	pgm.ref.b = acos(cos(pgm.ref.B) / sin(pgm.ref.A));
	// cos c = cot A cot B
	pgm.ref.c = acos(1.0 / (tan(pgm.ref.A) * tan(pgm.ref.B)));
	printf("%f %f %f\n", RTD(pgm.ref.a), RTD(pgm.ref.b), RTD(pgm.ref.c));

	// generate the (2,0) solution
	classI_2v_solution(&pgm,"icosa20");

	// generate the (3,0) solution
	classI_3v_solution(&pgm, "icosa30");

	// generate the (4,0) solution
	classI_4v_solution(&pgm, "icosa40");

	// generate the (5,0) solution
	classI_5v_solution(&pgm, "icosa50");

	// generate the (6,0) solution(s)
	classI_6v_solution(&pgm, "icosa60");

	// generate the (7,0) solution(s)
	classI_7v_solution(&pgm, "icosa70");
}
