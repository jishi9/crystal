#include "airfoil.h"
#include "cell-computation.h"
#include "edge-computation.h"
#include "mesh-builder.h"
#include "mesh.h"
#include "utils.h"

using std::forward_as_tuple;
using std::make_tuple;
using std::vector;


template<typename C>
void write_quad_data(const C& data, const char* filename) {
  FILE* fp;
  if ( (fp = fopen(filename,"w")) == NULL) {
    printf("can't open file %s\n",filename);
    exit(2);
  }

  fprintf(fp, "%lu 4\n", data.size());
  for (const auto& dat : data) {
    fprintf(fp, "%lf %lf %lf %lf\n", dat[0], dat[1], dat[2], dat[3]);
  }
}


int main(int argc, const char* argv[]) {
  // Check arguments
  const char* usage = "USAGE:\n"
    "    bin/run-airfoil-computation [filename]\n";
  check(argc == 2, usage);

  // Get mesh filename
  const char* filename = argv[1];

  // Read mesh
  QuadCellMesh mesh = readQuadMesh(filename);

  // Check mesh
  check(mesh.cell2ordnodes.size() == mesh.num_cells, "Bad cell2ordnodes");
  check(mesh.new_cell2ordnodes.size() == mesh.num_cells, "Bad new_cell2ordnodes");

  check(mesh.node_coord_data.size() == mesh.num_nodes, "Bad node_coord_data");
  check(mesh.new_node_coord_data.size() == mesh.num_nodes, "Bad new_node_coord_data");

  check(mesh.node_structured_regions.size() == mesh.node_structured_regions.size(),
    "Cell and node structured regions size mismatch");

  check(mesh.unstructured_cells_offset <= mesh.num_cells,
    "Unstructured cell offset too large");


  // Create dats
  P_BOUND p_bound = mesh.borderedge_bounds;
  P_X p_x = mesh.get_structured_node_data();
  P_Q p_q(mesh.num_cells);
  P_QOLD p_qold(mesh.num_cells);
  P_ADT p_adt(mesh.num_cells);
  P_RES p_res(mesh.num_cells);

  // INIT
  airfoil_init(mesh.num_cells, p_q, p_res);

  static_assert(sizeof(double) == sizeof(long), "NEIN");

  auto checksum = [&] () {
    long sum_q = 0;
    for (const auto& x : p_q) {
      for (const auto& y : x) {
        sum_q ^= *reinterpret_cast<const long*>(&y);
      }
    }
    printf("q: %ld\n", sum_q);

    long sum_qold = 0;
    for (const auto& x : p_qold) {
      for (const auto& y : x) {
        sum_qold ^= *reinterpret_cast<const long*>(&y);
      }
    }
    printf("qold: %ld\n", sum_qold);

    int sum_bound = 0;
    // for (int i=0 ; i<5 ; ++i) {
      // const auto& x = p_bound[i];
    for (const auto& x : p_bound) {
      sum_bound ^= (x);
    }
    printf("pbound: %d\n", sum_bound);

    long sum_res = 0;
    for (const auto& x : p_res) {
      for (const auto& y : x) {
        sum_res ^= *reinterpret_cast<const long*>(&y);
      }
    }
    printf("pres: %ld\n", sum_res);
  };

  auto miniprint = [&] () {

    for (int i=0 ; i<3 ; ++i) {
      const auto& x =  p_q[i];
      for (const auto& y : x) {
        printf("q: %.17g\n", y);
      }
    }

    for (int i=0 ; i<3 ; ++i) {
      const auto& x =  p_qold[i];
      for (const auto& y : x) {
        printf("qold: %.17g\n", y);
      }
    }

    for (int i=0 ; i<12 ; ++i) {
      const auto& x =  p_bound[i];
        printf("bound: %d\n", x);
    }

    for (int i=0 ; i<3 ; ++i) {
      const auto& x =  p_res[i];
      for (const auto& y : x) {
        printf("res: %.17g\n", y);
      }
    }
  };

  auto printit = [&] () {
    // return;
    cerr << "p_q" << endl;
    for (const auto & x: p_q) {
      cerr << '(' << x[0] << ',' << x[1] << ',' << x[2] << ',' << x[3] << ")" << endl;
    }
    cerr << endl << endl << endl << endl;

    cerr << "p_qold" << endl;
    for (const auto & x: p_qold) {
      cerr << '(' << x[0] << ',' << x[1] << ',' << x[2] << ',' << x[3] << "),";
    }
    cerr << endl << endl << endl << endl;

    cerr << "p_adt" << endl;
    for (const auto & x: p_adt) cerr << x << ",";
    cerr << endl << endl << endl << endl;

    cerr << "p_res" << endl;
    for (const auto & x: p_res) {
      cerr << '(' << x[0] << ',' << x[1] << ',' << x[2] << ',' << x[3] << "),";
    }
    cerr << endl << endl << endl << endl;

    for (const auto & x: p_bound) cerr << x << ",";
    cerr << endl << endl << endl << endl;
  };

  double rms;
  const int niter = 1000;
  // checksum();
  // miniprint();
  Timer timer;
  for(int iter=1; iter<=niter; iter++) {
    {
      const tuple<const P_Q&>& inputs = forward_as_tuple(p_q);
      const tuple<P_QOLD*>& outputs = forward_as_tuple(&p_qold);
      run_structured_cells(mesh, save_soln, inputs, outputs);
    }
    for(int k=0; k<2; k++) {
      {
        const tuple<const P_X&, const P_Q&>& inputs = forward_as_tuple(p_x, p_q);
        const tuple<P_ADT*>& outputs = forward_as_tuple(&p_adt);
        run_structured_cells(mesh, adt_calc, inputs, outputs);
      }
      {
        const tuple<const P_X&, const P_Q&, const P_ADT&>& inputs = forward_as_tuple(p_x, p_q, p_adt);
        const tuple<P_RES*>& outputs = forward_as_tuple(&p_res);
        run_structured_edges(mesh, res_calc, inputs, outputs);
      }
      {
      const tuple<const P_X&, const P_Q&, const P_ADT&, const P_BOUND&>& inputs = forward_as_tuple(p_x, p_q, p_adt, p_bound);
        const tuple<P_RES*>& outputs = forward_as_tuple(&p_res);
      run_border_edges(mesh,
        mesh.new_borderedge2ordnodes,
        mesh.new_borderedge2cell,
        mesh.new_cell2ordnodes,
        bres_calc, inputs, outputs);
      }

      rms = 0.0;
      {
        const tuple<const P_QOLD&, const P_ADT&>& inputs = forward_as_tuple(p_qold, p_adt);
        const tuple<P_Q*, P_RES*, RMS*>& outputs = forward_as_tuple(&p_q, &p_res, &rms);
        run_structured_cells(mesh, update, inputs, outputs);
      }
    }
    // printit();
    rms = sqrt(rms/(double) mesh.num_cells);
    if (iter%100 == 0)
      printf("%d  %10.5e \n",iter,rms);
    // printf("\n**ITER %d**\n", iter);
    // checksum();
  }
  write_quad_data(p_q, "out_my_data.dat");
  timer.lapAndPrint("Completed!");
  // printf("%d  %10.5e \n",niter,rms);
}
