#ifndef AIRFOIL_H
#define AIRFOIL_H

#include <array>
#include <cmath>
#include <tuple>
#include <vector>

#include "mesh.h"
#include "utils.h"

using std::array;
using std::get;
using std::tuple;
using std::vector;




//// CONSTANTS
double gam, gm1, cfl, eps, mach, alpha, qinf[4];


//// TYPES

// Border edges
typedef vector<int> P_BOUND;

// Nodes
typedef vector<Coordinate> P_X;

// Cells
typedef vector<array<double, 4>> P_Q;
typedef vector<array<double, 4>> P_QOLD;
typedef vector<double> P_ADT;
typedef vector<array<double, 4>> P_RES;

// Scalars
typedef double RMS;



//// INITIALIZE
void airfoil_init(const int ncell, P_Q& q, P_RES& res) {
  gam = 1.4f;
  gm1 = gam - 1.0f;
  cfl = 0.9f;
  eps = 0.05f;

  mach  = 0.4f;
  alpha = 3.0f*atan(1.0f)/45.0f;
  double p     = 1.0f;
  double r     = 1.0f;
  double u     = sqrt(gam*p/r)*mach;
  double e     = p/(r*gm1) + 0.5f*u*u;

  qinf[0] = r;
  qinf[1] = r*u;
  qinf[2] = 0.0f;
  qinf[3] = r*e;

  for (int n=0; n<ncell; n++) {
    for (int m=0; m<4; m++) {
        q[n][m] = qinf[m];
      res[n][m] = 0.0f;
    }
  }
}




// KERNELS


inline void save_soln(
  const int cell_id,
  const QuadNeighbours& cell_nodes,
  const QuadCellMesh& mesh,
  const tuple<const P_Q&>& inputs,
  const tuple<P_QOLD*>& outputs) {

  const P_Q& p_q = get<0>(inputs);
  P_QOLD& p_qold = *get<0>(outputs);

  const array<double, 4>& q = p_q[cell_id];
  array<double, 4>& qold = p_qold[cell_id];

  for (int n=0; n<4; n++) qold[n] = q[n];
}



inline void adt_calc(
  const int cell_id,
  const QuadNeighbours& cell_nodes,
  const QuadCellMesh& mesh,
  const tuple<const P_X&, const P_Q&>& inputs,
  const tuple<P_ADT*>& outputs) {

  const P_X& p_x = get<0>(inputs);
  const P_Q& p_q = get<1>(inputs);
  P_ADT& p_adt = *get<0>(outputs);

  const Coordinate& x1 = p_x[cell_nodes[0]];
  const Coordinate& x2 = p_x[cell_nodes[1]];
  const Coordinate& x3 = p_x[cell_nodes[2]];
  const Coordinate& x4 = p_x[cell_nodes[3]];
  const array<double, 4>& q = p_q[cell_id];
  double* adt = &p_adt[cell_id];


  double dx,dy, ri,u,v,c;

  ri =  1.0f/q[0];
  u  =   ri*q[1];
  v  =   ri*q[2];
  c  = sqrt(gam*gm1*(ri*q[3]-0.5f*(u*u+v*v)));

  dx = x2.x - x1.x;
  dy = x2.y - x1.y;
  *adt  = fabs(u*dy-v*dx) + c*sqrt(dx*dx+dy*dy);

  dx = x3.x - x2.x;
  dy = x3.y - x2.y;
  *adt += fabs(u*dy-v*dx) + c*sqrt(dx*dx+dy*dy);

  dx = x4.x - x3.x;
  dy = x4.y - x3.y;
  *adt += fabs(u*dy-v*dx) + c*sqrt(dx*dx+dy*dy);

  dx = x1.x - x4.x;
  dy = x1.y - x4.y;
  *adt += fabs(u*dy-v*dx) + c*sqrt(dx*dx+dy*dy);

  *adt = (*adt) / cfl;
}



inline void res_calc(
  const int node0_id, const int node1_id,
  const int cell0_id, const int cell1_id,
  const QuadNeighbours& cell0_nodes, const QuadNeighbours& cell1_nodes,
  const int edge_id, const QuadCellMesh& mesh,
  const tuple<const P_X&, const P_Q&, const P_ADT&>& inputs,
  const tuple<P_RES*>& outputs) {

  const P_X& p_x = get<0>(inputs);
  const P_Q& p_q = get<1>(inputs);
  const P_ADT& p_adt = get<2>(inputs);
  P_RES& p_res = *get<0>(outputs);

  const Coordinate& x1 = p_x[node0_id];
  const Coordinate& x2 = p_x[node1_id];
  const array<double, 4>& q1 = p_q[cell0_id];
  const array<double, 4>& q2 = p_q[cell1_id];
  const double* adt1 = &p_adt[cell0_id];
  const double* adt2 = &p_adt[cell1_id];
  array<double, 4>& res1 = p_res[cell0_id];
  array<double, 4>& res2 = p_res[cell1_id];


  double dx,dy,mu, ri, p1,vol1, p2,vol2, f;

  dx = x1.x - x2.x;
  dy = x1.y - x2.y;

  ri   = 1.0f/q1[0];
  p1   = gm1*(q1[3]-0.5f*ri*(q1[1]*q1[1]+q1[2]*q1[2]));
  vol1 =  ri*(q1[1]*dy - q1[2]*dx);

  ri   = 1.0f/q2[0];
  p2   = gm1*(q2[3]-0.5f*ri*(q2[1]*q2[1]+q2[2]*q2[2]));
  vol2 =  ri*(q2[1]*dy - q2[2]*dx);

  mu = 0.5f*((*adt1)+(*adt2))*eps;

  f = 0.5f*(vol1* q1[0]         + vol2* q2[0]        ) + mu*(q1[0]-q2[0]);
  res1[0] += f;
  res2[0] -= f;
  f = 0.5f*(vol1* q1[1] + p1*dy + vol2* q2[1] + p2*dy) + mu*(q1[1]-q2[1]);
  res1[1] += f;
  res2[1] -= f;
  f = 0.5f*(vol1* q1[2] - p1*dx + vol2* q2[2] - p2*dx) + mu*(q1[2]-q2[2]);
  res1[2] += f;
  res2[2] -= f;
  f = 0.5f*(vol1*(q1[3]+p1)     + vol2*(q2[3]+p2)    ) + mu*(q1[3]-q2[3]);
  res1[3] += f;
  res2[3] -= f;
}




inline void bres_calc(
  const int node0_id, const int node1_id,
  const int cell_id, const QuadNeighbours& cell_nodes,
  const int edge_id, const QuadCellMesh& mesh,
  const tuple<const P_X&, const P_Q&, const P_ADT&, const P_BOUND&>& inputs,
  const tuple<P_RES*>& outputs) {

  const P_X& p_x = get<0>(inputs);
  const P_Q& p_q = get<1>(inputs);
  const P_ADT& p_adt = get<2>(inputs);
  const P_BOUND& p_bound = get<3>(inputs);
  P_RES& p_res = *get<0>(outputs);

  const Coordinate& x1 = p_x[node0_id];
  const Coordinate& x2 = p_x[node1_id];
  const array<double, 4>& q1 = p_q[cell_id];
  const double* adt1 = &p_adt[cell_id];
  const int* bound = &p_bound[edge_id];
  array<double, 4>& res1 = p_res[cell_id];

  double dx,dy,mu, ri, p1,vol1, p2,vol2, f;

  dx = x1.x - x2.x;
  dy = x1.y - x2.y;

  ri = 1.0f/q1[0];
  p1 = gm1*(q1[3]-0.5f*ri*(q1[1]*q1[1]+q1[2]*q1[2]));

  if (*bound==1) {
    res1[1] += + p1*dy;
    res1[2] += - p1*dx;
  }
  else {
    vol1 =  ri*(q1[1]*dy - q1[2]*dx);

    ri   = 1.0f/qinf[0];
    p2   = gm1*(qinf[3]-0.5f*ri*(qinf[1]*qinf[1]+qinf[2]*qinf[2]));
    vol2 =  ri*(qinf[1]*dy - qinf[2]*dx);

    mu = (*adt1)*eps;

    f = 0.5f*(vol1* q1[0]         + vol2* qinf[0]        ) + mu*(q1[0]-qinf[0]);
    res1[0] += f;
    f = 0.5f*(vol1* q1[1] + p1*dy + vol2* qinf[1] + p2*dy) + mu*(q1[1]-qinf[1]);
    res1[1] += f;
    f = 0.5f*(vol1* q1[2] - p1*dx + vol2* qinf[2] - p2*dx) + mu*(q1[2]-qinf[2]);
    res1[2] += f;
    f = 0.5f*(vol1*(q1[3]+p1)     + vol2*(qinf[3]+p2)    ) + mu*(q1[3]-qinf[3]);
    res1[3] += f;
  }
}



inline void update(
  const int cell_id,
  const QuadNeighbours& cell_nodes,
  const QuadCellMesh& mesh,
  const tuple<const P_QOLD&, const P_ADT&>& inputs,
  const tuple<P_Q*, P_RES*, RMS*>& outputs) {

  const P_QOLD& p_qold = get<0>(inputs);
  const P_ADT& p_adt = get<1>(inputs);

  P_Q& p_q = *get<0>(outputs);
  P_RES& p_res = *get<1>(outputs);
  RMS* rms = get<2>(outputs);

  const array<double, 4>& qold = p_qold[cell_id];
  array<double, 4>& q = p_q[cell_id];
  array<double, 4>& res = p_res[cell_id];
  const double* adt = &p_adt[cell_id];

  double del, adti;

  adti = 1.0f/(*adt);

  for (int n=0; n<4; n++) {
    del    = adti*res[n];
    q[n]   = qold[n] - del;
    res[n] = 0.0f;
    *rms  += del*del;
  }
}

#endif