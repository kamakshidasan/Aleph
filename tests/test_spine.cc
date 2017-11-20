#include <tests/Base.hh>

#include <aleph/containers/PointCloud.hh>

#include <aleph/geometry/BruteForce.hh>
#include <aleph/geometry/VietorisRipsComplex.hh>

#include <aleph/geometry/distances/Euclidean.hh>

#include <aleph/persistentHomology/Calculation.hh>
#include <aleph/persistentHomology/PhiPersistence.hh>

#include <aleph/topology/BarycentricSubdivision.hh>
#include <aleph/topology/Simplex.hh>
#include <aleph/topology/SimplicialComplex.hh>
#include <aleph/topology/Skeleton.hh>
#include <aleph/topology/Spine.hh>

#include <aleph/topology/io/LinesAndPoints.hh>

#include <random>
#include <vector>

#include <cmath>

template <class T> void testDisk()
{
  ALEPH_TEST_BEGIN( "Spine: disk" );

  using DataType   = bool;
  using VertexType = T;

  using Simplex           = aleph::topology::Simplex<DataType, VertexType>;
  using SimplicialComplex = aleph::topology::SimplicialComplex<Simplex>;

  std::vector<Simplex> simplices;

  unsigned n = 7;
  for( unsigned i = 0; i < n; i++ )
  {
    if( i+1 < n )
      simplices.push_back( Simplex( {T(0),T(i+1),T(i+2)} ) );
    else
      simplices.push_back( Simplex( {T(0),T(i+1),T(  1)} ) );
  }

  SimplicialComplex K( simplices.begin(), simplices.end() );

  K.createMissingFaces();
  K.sort();

  auto L = aleph::topology::spine( K );

  ALEPH_ASSERT_THROW( L.size() < K.size() );
  ALEPH_ASSERT_EQUAL( L.size(), 1 );

  ALEPH_TEST_END();
}

template <class T> void testPinchedTorus()
{
  using DataType   = T;
  using PointCloud = aleph::containers::PointCloud<DataType>;

  ALEPH_TEST_BEGIN( "Spine: pinched torus" );

  unsigned n = 400;
  PointCloud pc( n, 3 );

  auto g = [] ( T x, T y )
  {
    return T(2) + std::sin(x/2) * std::cos(y);
  };

  std::random_device rd;
  std::mt19937 rng( rd() );

  std::normal_distribution<DataType> noise( T(0), T(0.05) );

  for( unsigned i = 0; i < static_cast<unsigned>( std::sqrt(n) ); i++ )
  {
    auto x = T( 2*M_PI / std::sqrt(n) * i );
    for( unsigned j = 0; j < static_cast<unsigned>( std::sqrt(n) ); j++ )
    {
      auto y = T( 2*M_PI / std::sqrt(n) * j );

      auto x0 = g(x,y) * std::cos(x)        + noise( rng );
      auto x1 = g(x,y) * std::sin(x)        + noise( rng );
      auto x2 = std::sin(x/2) * std::sin(y) + noise( rng );

      pc.set(static_cast<unsigned>( std::sqrt(n) ) * i + j, {x0,x1,x2} );
    }
  }

  using Distance          = aleph::geometry::distances::Euclidean<DataType>;
  using NearestNeighbours = aleph::geometry::BruteForce<PointCloud, Distance>;

  auto K
    = aleph::geometry::buildVietorisRipsComplex(
      NearestNeighbours( pc ),
      DataType( 0.75 ),
      2
  );

  std::ofstream out( "/tmp/Pinched_torus.txt" );
  aleph::topology::io::LinesAndPoints lap;
  lap( out, K, pc );

  auto D1 = aleph::calculatePersistenceDiagrams( K );

  ALEPH_ASSERT_EQUAL( D1.size(), 2 );
  ALEPH_ASSERT_EQUAL( D1[0].dimension(), 0 );
  ALEPH_ASSERT_EQUAL( D1[1].dimension(), 1 );
  ALEPH_ASSERT_EQUAL( D1[1].betti(),     1 );

  ALEPH_TEST_END();
}

template <class T> void testS1vS1()
{
  using DataType   = T;
  using PointCloud = aleph::containers::PointCloud<DataType>;

  ALEPH_TEST_BEGIN( "Spine: S^1 v S^1" );

  unsigned n = 50;

  PointCloud pc( 2*n, 2 );

  for( unsigned i = 0; i < n; i++ )
  {
    auto x0 = DataType( std::cos( 2*M_PI / n * i ) );
    auto y0 = DataType( std::sin( 2*M_PI / n * i ) );

    auto x1 = x0 + 2;
    auto y1 = y0;

    pc.set(2*i  , {x0, y0});
    pc.set(2*i+1, {x1, y1});
  }

  using Distance          = aleph::geometry::distances::Euclidean<DataType>;
  using NearestNeighbours = aleph::geometry::BruteForce<PointCloud, Distance>;

  auto K
    = aleph::geometry::buildVietorisRipsComplex(
      NearestNeighbours( pc ),
      DataType( 0.30 ),
      2
  );

  auto D1 = aleph::calculatePersistenceDiagrams( K );

  // Persistent homology -----------------------------------------------
  //
  // This should not be surprising: it is possible to extract the two
  // circles from the data set. They form one connected component.

  ALEPH_ASSERT_EQUAL( D1.size(),     2 );
  ALEPH_ASSERT_EQUAL( D1[0].betti(), 1 );
  ALEPH_ASSERT_EQUAL( D1[1].betti(), 2 );

  // Persistent intersection homology ----------------------------------
  //
  // Regardless of the stratification, it is impossible to detect the
  // singularity in dimension 0.

  auto L  = aleph::topology::BarycentricSubdivision()( K, [] ( std::size_t dimension ) { return dimension == 0 ? 0 : 0.5; } );
  auto K0 = aleph::topology::Skeleton()( 0, K );
  auto D2 = aleph::calculateIntersectionHomology( L, {K0,K}, aleph::Perversity( {-1} ) );

  ALEPH_ASSERT_EQUAL( D2.size(),         3 );
  ALEPH_ASSERT_EQUAL( D2[0].dimension(), 0 );
  ALEPH_ASSERT_EQUAL( D2[0].betti(),     1 );

  // Spine calculation -------------------------------------------------

  auto M = aleph::topology::spine( K );

  {
    auto D = aleph::calculatePersistenceDiagrams( M );

    ALEPH_ASSERT_EQUAL( D.size()    ,     2 );
    ALEPH_ASSERT_EQUAL( D[0].dimension(), 0 );
    ALEPH_ASSERT_EQUAL( D[1].dimension(), 1 );
    ALEPH_ASSERT_EQUAL( D[0].betti(),     1 );
    ALEPH_ASSERT_EQUAL( D[1].betti(),     2 );
  }

  ALEPH_ASSERT_THROW( M.size() < K .size() );
  ALEPH_TEST_END();

  L       = aleph::topology::BarycentricSubdivision()( M, [] ( std::size_t dimension ) { return dimension == 0 ? 0 : 0.5; } );
  K0      = aleph::topology::Skeleton()(0, M);
  auto D3 = aleph::calculateIntersectionHomology( L, {K0,M}, aleph::Perversity( {0} ) );

  ALEPH_ASSERT_EQUAL( D3.size(),         3  );
  ALEPH_ASSERT_EQUAL( D3[0].dimension(), 0  );
  ALEPH_ASSERT_EQUAL( D3[0].betti(),     43 );
}

template <class T> void testTriangle()
{
  using DataType   = bool;
  using VertexType = T;

  using Simplex           = aleph::topology::Simplex<DataType, VertexType>;
  using SimplicialComplex = aleph::topology::SimplicialComplex<Simplex>;

  SimplicialComplex K = {
    {0,1,2},
    {0,1}, {0,2}, {1,2},
    {0}, {1}, {2}
  };

  auto L = aleph::topology::spine( K );

  ALEPH_ASSERT_THROW( L.size() < K.size() );
  ALEPH_ASSERT_EQUAL( L.size(), 1 );
}

int main( int, char** )
{
  testDisk<short>   ();
  testDisk<unsigned>();

  testPinchedTorus<float> ();
  testPinchedTorus<double>();

  testS1vS1<float> ();
  testS1vS1<double>();

  testTriangle<short>   ();
  testTriangle<unsigned>();
}
