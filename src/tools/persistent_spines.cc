/*
  This is a tool shipped by 'Aleph - A Library for Exploring Persistent
  Homology'.

  Given an input filename and a radius parameter, the tool first does a
  Čech complex expansion. Afterwards, a *spine* in the PL sense will be
  calculated using different methods.

  If desired, all simplicial complexes generated by this method will be
  rendered as TikZ pictures for LaTeX output.

  Original author: Bastian Rieck
*/

#include <iostream>
#include <string>

#include <aleph/containers/PointCloud.hh>

#include <aleph/geometry/CechComplex.hh>

#include <aleph/topology/Spine.hh>

#include <aleph/topology/io/TikZ.hh>

#include <getopt.h>

using DataType   = double;
using PointCloud = aleph::containers::PointCloud<DataType>;

void usage()
{
}

int main( int argc, char** argv )
{
  std::string method = "dumb";
  DataType radius    = DataType();
  bool tikzOutput    = false;

  {
    static option commandLineOptions[] =
    {
      { "radius"     , required_argument, nullptr, 'r' },
      { "spine"      , required_argument, nullptr, 's' },
      { "tikz"       , no_argument      , nullptr, 't' },
      { nullptr      , 0                , nullptr,  0  }
    };

    int option = 0;
    while( ( option = getopt_long( argc, argv, "r:st", commandLineOptions, nullptr ) ) != -1 )
    {
      switch( option )
      {
      case 'r':
        radius = static_cast<DataType>( std::stod( optarg ) );
        break;
      case 's':
        method = optarg;
        break;
      case 't':
        tikzOutput = true;
        break;
      }
    }
  }

  if( ( argc - optind ) < 1 )
  {
    usage();
    return -1;
  }

  std::string filename = argv[ optind++ ];

  // 1. Point cloud loading --------------------------------------------

  std::cerr << "* Loading point cloud from '" << filename << "'...";

  auto pointCloud
    = aleph::containers::load<DataType>( filename );

  std::cerr << "finished\n"
            << "* Point cloud contains " << pointCloud.size() << " points of dimensionality " << pointCloud.dimension() << "\n";

  // 2. Čech complex calculation ---------------------------------------

  std::cerr << "* Calculating Čech complex with r=" << radius << "...";

  auto simplicialComplex
    = aleph::geometry::buildCechComplex(
        pointCloud,
        radius
  );

  std::cerr << "finished\n"
            << "* Čech complex contains " << simplicialComplex.size() << " simplices\n";

  using SimplicialComplex = decltype( simplicialComplex );

  // 3. Spine calculation ----------------------------------------------

  std::cerr << "* Calculating spine (" << method << " method)...";

  SimplicialComplex spine;
  if( method == "dumb" )
    spine = aleph::topology::dumb::spine( simplicialComplex );

  // Not so many choices here for now, to be honest...
  else
    spine = aleph::topology::spine( simplicialComplex );

  std::cerr << "finished\n";

  // 4. Output ---------------------------------------------------------

  if( tikzOutput )
  {
    std::cout << "\\documentclass{standalone}\n"
              << "\\usepackage{tikz}\n"
              << "\\begin{document}\n";

    aleph::topology::io::TikZ writer;

    writer.showBalls( true );
    writer.ballRadius( radius );

    writer( std::cout,
            simplicialComplex,
            pointCloud );

    std::cout << "\n\n";

    writer.showBalls( false );
    writer( std::cout,
            spine,
            pointCloud );

    std::cout << "\\end{document}\n";
  }
  else
    std::cout << spine << "\n";
}
