/*
  Copyright (c) 2013, Randolph Voorhies, Shane Grant
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
      * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
      * Neither the name of cereal nor the
        names of its contributors may be used to endorse or promote products
        derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <sstream>
#include <iostream>
#include <chrono>
#include <random>

#include <boost/format.hpp>

#include <boost/serialization/serialization.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/base_object.hpp>

#include <cereal/binary_archive/binary_archive.hpp>
#include <cereal/binary_archive/vector.hpp>

//! Runs serialization to save data to an ostringstream
/*! Used to time how long it takes to save data to an ostringstream.
    Everything that happens within the save function will be timed, including
    any set-up necessary to perform the serialization.

    @param data The data to save
    @param saveFunction A function taking in an ostringstream and the data and returning void
    @return The ostringstream and the time it took to save the data */
template <class T>
std::chrono::milliseconds
saveData( T const & data, std::function<void(std::ostringstream &, T const&)> saveFunction, std::ostringstream & os )
{
  auto start = std::chrono::high_resolution_clock::now();
  saveFunction( os, data );
  return std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::high_resolution_clock::now() - start );
}

//! Runs serialization to load data to from an istringstream
/*! Used to time how long it takes to load data from an istringstream.
    Everything that happens within the load function will be timed, including
    any set-up necessary to perform the serialization.

    @param dataStream The saved data stream
    @param loadFunction A function taking in an istringstream and a data reference and returning void
    @return The loaded data and the time it took to save the data */
template <class T>
std::pair<T, std::chrono::milliseconds>
loadData( std::ostringstream const & dataStream, std::function<void(std::istringstream &, T &)> loadFunction )
{
  T data;
  std::istringstream os( dataStream.str() );

  auto start = std::chrono::high_resolution_clock::now();
  loadFunction( os, data );

  return {data, std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::high_resolution_clock::now() - start )};
}

struct cerealBinary
{
  //! Saves data to a cereal binary archive
  template <class T>
  static void save( std::ostringstream & os, T const & data )
  {
    cereal::BinaryOutputArchive oar(os);
    oar & data;
  }

  //! Loads data to a cereal binary archive
  template <class T>
  static void load( std::istringstream & is, T & data )
  {
    cereal::BinaryInputArchive iar(is);
    iar & data;
  }
};

struct boostBinary
{
  //! Saves data to a boost binary archive
  template <class T>
  static void save( std::ostringstream & os, T const & data )
  {
    boost::archive::binary_oarchive oar(os);
    oar & data;
  }

  //! Loads data to a boost binary archive
  template <class T>
  static void load( std::istringstream & is, T & data )
  {
    boost::archive::binary_iarchive iar(is);
    iar & data;
  }
};

struct binary
{
  typedef boostBinary  boost;
  typedef cerealBinary cereal;
};

//! Times how long it takes to serialize (load and store) some data
/*! Times how long and the size of the serialization object used to serialize
    some data.  Result is output to standard out.

    @tparam SerializationT The serialization struct that has all save and load functions
    @tparam DataT The type of data to test
    @param name The name for this test
    @param data The data to serialize
    @param numAverages The number of times to average
    @param validateData Whether data should be validated (input == output) */
template <class SerializationT, class DataT>
void test( std::string const & name,
            DataT const & data,
            size_t numAverages = 10,
            bool validateData = false )
{
  std::cout << "-----------------------------------" << std::endl;
  std::cout << "Running test: " << name << std::endl;

  std::chrono::milliseconds totalBoostSave{0};
  std::chrono::milliseconds totalBoostLoad{0};

  std::chrono::milliseconds totalCerealSave{0};
  std::chrono::milliseconds totalCerealLoad{0};

  size_t boostSize = 0;
  size_t cerealSize = 0;

  for(size_t i = 0; i < numAverages; ++i)
  {
    // Boost
    {
      std::ostringstream os;
      auto saveResult = saveData<DataT>( data, {SerializationT::boost::template save<DataT>}, os );
      totalBoostSave += saveResult;
      if(!boostSize)
        boostSize = os.tellp();

      auto loadResult = loadData<DataT>( os, {SerializationT::boost::template load<DataT>} );
      totalBoostLoad += loadResult.second;

      if( validateData )
        ; // TODO
    }

    // Cereal
    {
      std::ostringstream os;
      auto saveResult = saveData<DataT>( data, {SerializationT::cereal::template save<DataT>}, os );
      totalCerealSave += saveResult;
      if(!cerealSize)
        cerealSize = os.tellp();

      auto loadResult = loadData<DataT>( os, {SerializationT::cereal::template load<DataT>} );
      totalCerealLoad += loadResult.second;

      if( validateData )
        ; // TODO
    }
  }

  // Averages
  double averageBoostSave = totalBoostSave.count() / static_cast<double>( numAverages );
  double averageBoostLoad = totalBoostLoad.count() / static_cast<double>( numAverages );

  double averageCerealSave = totalCerealSave.count() / static_cast<double>( numAverages );
  double averageCerealLoad = totalCerealLoad.count() / static_cast<double>( numAverages );

  // Percentages relative to boost
  double cerealSaveP = averageCerealSave / averageBoostSave;
  double cerealLoadP = averageCerealLoad / averageBoostLoad;
  double cerealSizeP = cerealSize / static_cast<double>( boostSize );

  std::cout << "  Boost results:" << std::endl;
  std::cout << boost::format("\tsave | time: %06.4fms (%1.2f) size: %20.8fkb (%1.8f) total: %6.1fms")
    % averageBoostSave % 1.0 % (boostSize / 1024.0) % 1.0 % static_cast<double>( totalBoostSave.count() );
  std::cout << std::endl;
  std::cout << boost::format("\tload | time: %06.4fms (%1.2f) total: %6.1fms")
    % averageBoostLoad % 1.0 % static_cast<double>( totalBoostLoad.count() );
  std::cout << std::endl;

  std::cout << "  Cereal results:" << std::endl;
  std::cout << boost::format("\tsave | time: %06.4fms (%1.2f) size: %20.8fkb (%1.8f) total: %6.1fms")
    % averageCerealSave % cerealSaveP % (cerealSize / 1024.0) % cerealSizeP % static_cast<double>( totalCerealSave.count() );
  std::cout << std::endl;
  std::cout << boost::format("\tload | time: %06.4fms (%1.2f) total: %6.1fms")
    % averageCerealLoad % cerealLoadP % static_cast<double>( totalCerealLoad.count() );
  std::cout << std::endl;
}

template<class T>
typename std::enable_if<std::is_floating_point<T>::value, T>::type
random_value(std::mt19937 & gen)
{ return std::uniform_real_distribution<T>(-10000.0, 10000.0)(gen); }

template<class T>
typename std::enable_if<std::is_integral<T>::value, T>::type
random_value(std::mt19937 & gen)
{ return std::uniform_int_distribution<T>(std::numeric_limits<T>::lowest(), std::numeric_limits<T>::max())(gen); }

template<class T>
typename std::enable_if<std::is_same<T, std::string>::value, std::string>::type
random_value(std::mt19937 & gen)
{
  std::string s(std::uniform_int_distribution<int>(3, 30)(gen), ' ');
  for(char & c : s)
    c = std::uniform_int_distribution<char>(' ', '~')(gen);
  return s;
}

template<class C>
std::basic_string<C> random_basic_string(std::mt19937 & gen)
{
  std::basic_string<C> s(std::uniform_int_distribution<int>(3, 30)(gen), ' ');
  for(C & c : s)
    c = std::uniform_int_distribution<C>(' ', '~')(gen);
  return s;
}

template <size_t N>
std::string random_binary_string(std::mt19937 & gen)
{
  std::string s(N, ' ');
  for(auto & c : s )
    c = std::uniform_int_distribution<char>('0', '1')(gen);
  return s;
}

struct PoDStruct
{
  int32_t a;
  int64_t b;
  float c;
  double d;

  template <class Archive>
  void serialize( Archive & ar )
  {
    ar & a & b & c & d;
  };

  template <class Archive>
  void serialize( Archive & ar, const unsigned int version )
  {
    ar & a & b & c & d;
  };
};

struct PoDChild : PoDStruct
{
  PoDChild() : v(1024)
  { }

  std::vector<float> v;

  template <class Archive>
  void serialize( Archive & ar )
  {
    ar & static_cast<PoDStruct>(*this);
    ar & v;
  };

  template <class Archive>
  void serialize( Archive & ar, const unsigned int version )
  {
    ar & boost::serialization::base_object<PoDStruct>(*this);
    ar & v;
  };
};

int main()
{
  std::random_device rd;
  std::mt19937 gen(rd());
  auto rngC = [&](){ return random_value<uint8_t>(gen); };
  auto rngD = [&](){ return random_value<double>(gen); };
  const bool randomize = false;

  //########################################
  auto vectorDoubleTest = [&](size_t s, bool randomize)
  {
    std::ostringstream name;
    name << "Vector(double) size " << s;

    std::vector<double> data(s);
    if(randomize)
      for( auto & d : data )
        d = rngD();

    test<binary>( name.str(), data );
  };

  vectorDoubleTest(1, randomize); // 8B
  vectorDoubleTest(16, randomize); // 128B
  vectorDoubleTest(1024, randomize); // 8KB
  vectorDoubleTest(1024*1024, randomize); // 8MB

  //########################################
  auto vectorCharTest = [&](size_t s, bool randomize)
  {
    std::ostringstream name;
    name << "Vector(uint8_t) size " << s;

    std::vector<uint8_t> data(s);
    if(randomize)
      for( auto & d : data )
        d = rngC();

    test<binary>( name.str(), data );
  };

  vectorCharTest(1024*1024*1024, randomize); // 1 GB

  //########################################
  auto vectorPoDStructTest = [&](size_t s)
  {
    std::ostringstream name;
    name << "Vector(PoDStruct) size " << s;

    std::vector<PoDStruct> data(s);
    test<binary>( name.str(), data );
  };

  vectorPoDStructTest(1);
  vectorPoDStructTest(64);
  vectorPoDStructTest(1024);
  vectorPoDStructTest(1024*1024);
  vectorPoDStructTest(1024*1024*64);

  //########################################
  auto vectorPoDChildTest = [&](size_t s)
  {
    std::ostringstream name;
    name << "Vector(PoDChild) size " << s;

    std::vector<PoDChild> data(s);
    test<binary>( name.str(), data );
  };

  vectorPoDChildTest(1024*64);

  return 0;
}
