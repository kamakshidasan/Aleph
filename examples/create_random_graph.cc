/*
  This is an example file shipped by 'Aleph - A Library for Exploring
  Persistent Homology'.

  This example demonstrates how to create a random graph with a given
  linkage probability $p$ and a number of vertices $n$, convert it to
  a simplicial complex, and write it to STDOUT in GML format.

  Demonstrates classes:

    - aleph::topology::SimplicialComplex
    - aleph::topology::io::GMLWriter

  Demonstrated functions:

    - aleph::topology::generateErdosRenyiGraph
    - aleph::topology::generateWeightedRandomGraph

  Original author: Bastian Rieck
*/

#include "topology/RandomGraph.hh"
#include "topology/SimplicialComplex.hh"

#include <fstream>
#include <iostream>
#include <string>

#include <getopt.h>

/** Auxiliary function for storing a graph in an output stream. */
template <class SimplicialComplex> void storeGraph(const SimplicialComplex& K, std::ostream& out)
{
  // TODO: not yet implemented
  out << K << "\n";
}

int main( int argc, char** argv )
{
  static option commandLineOptions[] = {
    { "cardinality", required_argument, nullptr, 'n' },
    { "probability", required_argument, nullptr, 'p' },
    { "weighted"   , no_argument      , nullptr, 'w' },
    { nullptr      , 0                , nullptr,  0  }
  };

  double p      = 0.25;
  unsigned n    = 100;
  bool weighted = false;

  {
    int option = 0;
    while( ( option = getopt_long( argc, argv, "n:p:w", commandLineOptions, nullptr ) ) != -1 )
    {
      switch( option )
      {
      case 'n':
        n = static_cast<unsigned>( std::stoul(optarg) );
        break;
      case 'p':
        p = std::stod(optarg);
        break;
      case 'w':
        weighted = true;
        break;
      default:
        break;
      }
    }
  }

  std::cerr << "* Generating a random graph with n=" << n << " and p=" << p << "...";

  if( weighted )
  {
    auto K = aleph::topology::generateErdosRenyiGraph( n, p );
    storeGraph(K, std::cout);
  }
  else
  {
    auto K = aleph::topology::generateWeightedRandomGraph( n, p );
    storeGraph(K, std::cout);
  }

  std::cerr << "finished\n";
}