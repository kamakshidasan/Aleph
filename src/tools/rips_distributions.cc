#include <aleph/containers/PointCloud.hh>

#include <aleph/config/FLANN.hh>

#include <aleph/geometry/BruteForce.hh>
#include <aleph/geometry/FLANN.hh>

#include <aleph/geometry/distances/Euclidean.hh>
#include <aleph/geometry/distances/Manhattan.hh>
#include <aleph/geometry/distances/Traits.hh>

#include <iostream>
#include <string>
#include <vector>

#include <getopt.h>

using DataType          = double;
using PointCloud        = aleph::containers::PointCloud<DataType>;
using EuclideanDistance = aleph::distances::Euclidean<DataType>;
using ManhattanDistance = aleph::distances::Manhattan<DataType>;

template <class Distance> std::vector<DataType> pairwiseDistances( const PointCloud& pointCloud, Distance distance = Distance() )
{
  if( pointCloud.empty() )
    return {};

  std::vector<DataType> distances;
  distances.reserve( pointCloud.size() * ( pointCloud.size() - 1 ) / 2 );

  auto d       = pointCloud.dimension();
  auto n       = pointCloud.size();
  using Traits = aleph::distances::Traits<Distance>;

  Traits traits;
  for( decltype(n) i = 0; i < n; i++ )
  {
    for( decltype(n) j = i+1; j < n; j++ )
    {
      auto dist = distance( pointCloud[i].begin(),
                            pointCloud[j].begin(),
                            d );

      dist      = traits.from( dist );

      distances.emplace_back( dist );
    }
  }

  return distances;
}

int main( int argc, char** argv )
{
  static option commandLineOptions[] =
  {
    { "distance"      , required_argument, nullptr, 'd' },
    { nullptr         , 0                , nullptr,  0  }
  };

  std::string selectedDistanceFunctor = "euclidean";

  {
    int option = 0;
    while( ( option = getopt_long( argc, argv, "d:", commandLineOptions, nullptr ) ) != -1 )
    {
      switch( option )
      {
      case 'd':
        selectedDistanceFunctor = optarg;
        break;
      }
    }
  }

  if( (argc - optind) < 1 )
    return -1;

  auto filename   = std::string( argv[optind++] );
  auto pointCloud = aleph::containers::load<DataType>( filename );

  std::cerr << "* Loaded point cloud with " << pointCloud.size() << " points\n";

  std::vector<DataType> distances;

  if( selectedDistanceFunctor == "euclidean" )
    distances = pairwiseDistances( pointCloud, EuclideanDistance() );
  else if( selectedDistanceFunctor == "manhattan" )
    distances = pairwiseDistances( pointCloud, ManhattanDistance() );

  for( auto&& distance : distances )
    std::cout << distance << "\n";
}
