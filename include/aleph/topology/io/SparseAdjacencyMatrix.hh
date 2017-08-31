#ifndef ALEPH_TOPOLOGY_IO_SPARSE_ADJACENCY_MATRIX_HH__
#define ALEPH_TOPOLOGY_IO_SPARSE_ADJACENCY_MATRIX_HH__

#include <aleph/utilities/Filesystem.hh>
#include <aleph/utilities/String.hh>

// FIXME: remove after debugging
#include <iostream>

#include <fstream>
#include <set>
#include <string>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <tuple>
#include <vector>

namespace aleph
{

namespace topology
{

namespace io
{

class SparseAdjacencyMatrixReader
{
public:
  template <class SimplicialComplex> void operator()( const std::string& filename,
                                                      std::vector<SimplicialComplex>& result )
  {
    using Simplex    = typename SimplicialComplex::ValueType;
    using VertexType = typename Simplex::VertexType;
    using Edge       = std::pair<VertexType, VertexType>;

    std::unordered_set<VertexType> vertices;
    std::vector<Edge> edges;

    std::tie( vertices, edges )
      = this->readVerticesAndEdges<VertexType>( filename );

    auto graphIndicatorFilename = getFilenameGraphIndicator( filename );

#if 0
    // TODO: implement?

    if( !aleph::utilities::exists( graphIndicatorFilename ) )
      throw std::runtime_error( "Missing required graph indicator file" );
#endif

    // Stores *all* graph IDs. The set makes sense here because I want
    // to ensure that repeated calls to this function always yield the
    // same order.
    std::set<VertexType> graphIDs;

    // Maps a node ID to a graph ID, i.e. yields the inex of the graph
    // that should contain the node. All indices are starting at 1 but
    // will be mapped to 0 later on.
    std::unordered_map<VertexType, VertexType> node_id_to_graph_id;

    std::tie( graphIDs, node_id_to_graph_id )
      = readGraphAndNodeIDs<VertexType>( graphIndicatorFilename );

    // Maps a graph ID (arbitrary start point) to an index in the
    // vector.

    using IndexType = std::size_t;
    std::unordered_map<VertexType, IndexType> graph_id_to_index;

    {
      IndexType index = IndexType();

      for( auto&& id : graphIDs )
        graph_id_to_index[id] = index++;
    }

    // Reading optional attributes -------------------------------------

    if( _readGraphLabels )
      this->readGraphLabels( filename );

    if( _readNodeLabels )
      this->readNodeLabels( filename );

    // Create output ---------------------------------------------------
    //
    // Create the set of output graphs and distribute the edges among
    // them according to their graph ID. This function also does some
    // sanity checks in order to check input data consistency.

    result.clear();
    result.resize( graphIDs.size() );

    for( auto&& vertex : vertices )
    {
      auto&& id    = node_id_to_graph_id[vertex];
      auto&& index = graph_id_to_index[id];
      auto&& K     = result[index];

      K.push_back( Simplex( vertex ) );
    }

    for( auto&& edge : edges )
    {
      auto&& u   = edge.first;
      auto&& v   = edge.second;
      auto&& uID = node_id_to_graph_id[u];
      auto&& vID = node_id_to_graph_id[v];

      if( uID != vID )
        throw std::runtime_error( "Format error: an edge must not belong to multiple graphs" );

      auto&& index = graph_id_to_index[ uID ];
      auto&& K     = result[index];

      K.push_back( Simplex( {u,v} ) );
    }
  }

  // Configuration options ---------------------------------------------
  //
  // The following attributes configure how the parsing process works
  // and which attributes are being read.

  void setReadGraphLabels( bool value = true ) noexcept { _readGraphLabels = value; }
  void setTrimLines( bool value = true )       noexcept { _trimLines = value; }

  bool readGraphLabels() const noexcept { return _readGraphLabels;  }
  bool trimLines()       const noexcept { return _trimLines; }

private:

  /**
    Reads all vertices and  edges from a sparse adjacency matrix. Note
    that this function is not static because it needs access to values
    that depend on class instances.
  */

  template <class VertexType>
    std::pair<
      std::unordered_set<VertexType>,
      std::vector< std::pair<VertexType, VertexType> >
    > readVerticesAndEdges( const std::string& filename )
  {
    using Edge = std::pair<VertexType, VertexType>;

    std::unordered_set<VertexType> vertices;
    std::vector<Edge> edges;

    std::ifstream in( filename );
    if( !in )
      throw std::runtime_error( "Unable to read input adjacency matrix file" );

    std::string line;

    while( std::getline( in, line ) )
    {
      using namespace aleph::utilities;

      auto tokens = split( line, _separator );

      if( tokens.size() == 2 )
      {
        auto u = convert<VertexType>( tokens.front() );
        auto v = convert<VertexType>( tokens.back()  );

        edges.push_back( std::make_pair(u, v) );

        vertices.insert( u );
        vertices.insert( v );
      }
      else
        throw std::runtime_error( "Format error: cannot parse line in sparse adjacency matrix" );
    }

    return std::make_pair( vertices, edges );
  }

  template <class VertexType>
    std::pair<
      std::set<VertexType>,
      std::unordered_map<VertexType, VertexType>
    > readGraphAndNodeIDs( const std::string& filename )
  {
    std::unordered_map<VertexType, VertexType> node_id_to_graph_id;
    std::set<VertexType> graphIDs;

    std::ifstream in( filename );
    if( !in )
      throw std::runtime_error( "Unable to read graph indicator file" );

    std::string line;
    VertexType nodeID = 1;

    while( std::getline( in, line ) )
    {
      using namespace aleph::utilities;
      line = trim( line );

      auto graphID                    = convert<VertexType>( line );
      node_id_to_graph_id[ nodeID++ ] = graphID;

      graphIDs.insert( graphID );
    }

    return std::make_pair( graphIDs, node_id_to_graph_id );
  }

  std::vector<std::string> readLabels( const std::string& filename )
  {
    std::ifstream in( filename );
    if( !in )
      throw std::runtime_error( "Unable to read labels input file" );

    std::vector<std::string> labels;
    std::string line;

    while( std::getline( in, line ) )
    {
      if( _trimLines )
        line = aleph::utilities::trim( line );

      labels.push_back( line );
    }

    return labels;
  }

  void readGraphLabels( const std::string& filename )
  {
    auto graphLabelsFilename = getFilenameGraphLabels( filename );
    _graphLabels             = readLabels( graphLabelsFilename );
  }

  void readNodeLabels( const std::string& filename )
  {
    auto nodeLabelsFilename = getFilenameNodeLabels( filename );
    _nodeLabels             = readLabels( nodeLabelsFilename );
  }

  /**
   Given a base filename, gets its prefix. The prefix is everything that
   comes before the last `_` character. It is used to generate filenames
   for various auxiliary information about the graph if not specified by
   the client.
  */

  static std::string getPrefix( const std::string& filename )
  {
    // Note that this contains the complete filename portion of the path
    // along with any subdirectories.
    auto prefix = filename.substr( 0, filename.find_last_of( '_' ) );
    return prefix;
  }

  /*
    Given a base filename, calculates the default filename for the graph
    indicator values, for example. This filename is generated by getting
    a prefix of the filename and attaching e.g. `_graph_indicator.txt`.

    This function is *not* used if the user specified a file manually in
    which to find such information.
  */

  static std::string getFilenameGraphIndicator( const std::string& filename )  { return getPrefix(filename) + "_graph_indicator.txt";  }
  static std::string getFilenameGraphLabels( const std::string& filename )     { return getPrefix(filename) + "_graph_labels.txt";     }
  static std::string getFilenameNodeLabels( const std::string& filename )      { return getPrefix(filename) + "_node_labels.txt";      }
  static std::string getFilenameEdgeLabels( const std::string& filename )      { return getPrefix(filename) + "_edge_labels.txt";      }
  static std::string getFilenameEdgeAttributes( const std::string& filename )  { return getPrefix(filename) + "_edge_attributes.txt";  }
  static std::string getFilenameNodeAttributes( const std::string& filename )  { return getPrefix(filename) + "_node_attributes.txt";  }
  static std::string getFilenameGraphAttributes( const std::string& filename ) { return getPrefix(filename) + "_graph_attributes.txt"; }

  bool _readGraphLabels = true;
  bool _readNodeLabels  = false;
  bool _trimLines       = true;

  /**
    Graph labels stored during the main parsing routine of this class.
    If no graph labels are specified, this vector remains empty. Since
    the format does not specify the format of graph labels, the labels
    will *not* be converted but reported as-is.

    Graph labels are only read if `_readGraphLabels` is true.
  */

  std::vector<std::string> _graphLabels;

  /** Node labels; the same comments as for the graph labels apply */
  std::vector<std::string> _nodeLabels;

  // TODO: make configurable
  std::string _separator = ",";
};

} // namespace io

} // namespace topology

} // namespace aleph

#endif
