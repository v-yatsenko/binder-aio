// Copyright (c) 2018 Vadym Yatsenko <vadim.yatsenko@gmail.com>
//
// All rights reserved. Use of this source code is governed by a
// MIT license that can be found in the LICENSE file.

/// @file   binder/reporter.cpp
/// @brief  Binding statistic reporting
/// @author Vadym Yatsenko

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include <reporter.hpp>

using namespace binder;
using namespace llvm;
using namespace clang;

using boost::property_tree::ptree;

#define JACOCO_DTD "<!DOCTYPE report SYSTEM 'http://www.eclemma.org/jacoco/trunk/coverage/report.dtd' >"

class CoverageExporterJacoco {
  ptree doc;

  void emitMethod(ptree& tree, ptree& sourcefile, const PythonCoverage::Method& m, unsigned& covered, unsigned& missed)
  {
    ptree method;
    method.put( "<xmlattr>.name", m.name );
    method.put( "<xmlattr>.desc", m.desc );
    method.put( "<xmlattr>.line", m.declaration_line );

    ptree m_counter;
    m_counter.put( "<xmlattr>.type", "METHOD" );
    ptree l_counter;
    l_counter.put( "<xmlattr>.type", "LINE" );
    ptree i_counter;
    i_counter.put( "<xmlattr>.type", "INSTRUCTION" );

    ptree line;
    line.put( "<xmlattr>.ln", m.declaration_line );

    llvm::outs() << "    " << m.name << " use " << m.usages << "\n";
    if( m.usages > 0 ) {
      m_counter.put( "<xmlattr>.covered", 1 );
      m_counter.put( "<xmlattr>.missed", 0 );
      l_counter.put( "<xmlattr>.covered", 1 );
      l_counter.put( "<xmlattr>.missed", 0 );
      i_counter.put( "<xmlattr>.covered", 1 );
      i_counter.put( "<xmlattr>.missed", 0 );
      line.put( "<xmlattr>.ci", 1 );
      line.put( "<xmlattr>.mi", 0 );
      covered++;
    } else {
      m_counter.put( "<xmlattr>.covered", 0 );
      m_counter.put( "<xmlattr>.missed", 1 );
      l_counter.put( "<xmlattr>.covered", 0 );
      l_counter.put( "<xmlattr>.missed", 1 );
      i_counter.put( "<xmlattr>.covered", 0 );
      i_counter.put( "<xmlattr>.missed", 1 );
      line.put( "<xmlattr>.ci", 0 );
      line.put( "<xmlattr>.mi", 1 );
      missed++;
    }

    method.add_child( "counter", m_counter );
    method.add_child( "counter", l_counter );
    //method.add_child( "counter", i_counter );
    tree.add_child( "method", method );

    sourcefile.add_child( "line", line );
  }

  void emitClass(ptree& tree, ptree& sourcefile, const PythonCoverage::Class& c, unsigned& covered, unsigned& missed)
  {
    unsigned _covered = 0;
    unsigned _missed = 0;
    ptree cl;
    cl.put( "<xmlattr>.name", c.name );
    cl.put( "<xmlattr>.sourcefilename", c.file_name );

    sourcefile.put( "<xmlattr>.name", c.file_name );

    llvm::outs() << "  " << c.name << ":" << "\n";
    for( const PythonCoverage::Method& m: c.methods ) {
      emitMethod( cl, sourcefile, m, _covered, _missed );
    }

    ptree counter;
    counter.put( "<xmlattr>.type", "METHOD" );
    counter.put( "<xmlattr>.covered", _covered );
    counter.put( "<xmlattr>.missed", _missed );

    cl.add_child( "counter", counter );

    tree.add_child( "class", cl );

    ptree* class_counter = nullptr;
    ptree* method_counter = nullptr;
    ptree* line_counter = nullptr;

    try {
    for( auto src: sourcefile.get_child( "counter" ) ) {
      if( src.second.get<std::string>( "<xmlattr>.type" ) == "CLASS" ) {
        class_counter = &src.second;
      }
    }
    }
    catch(...) {
      ptree counter;
      counter.put( "<xmlattr>.type", "CLASS" );
      class_counter = &sourcefile.add_child( "counter", counter );
    }

    try {
    for( auto src: sourcefile.get_child( "counter" ) ) {
      if( src.second.get<std::string>( "<xmlattr>.type" ) == "METHOD" ) {
        method_counter = &src.second;
      }
    }
    }
    catch(...) {
      ptree counter;
      counter.put( "<xmlattr>.type", "METHOD" );
      method_counter = &sourcefile.add_child( "counter", counter );
    }

    try {
    for( auto src: sourcefile.get_child( "counter" ) ) {
      if( src.second.get<std::string>( "<xmlattr>.type" ) == "LINE" ) {
        line_counter = &src.second;
      }
    }
    }
    catch(...) {
      ptree counter;
      counter.put( "<xmlattr>.type", "LINE" );
      line_counter = &sourcefile.add_child( "counter", counter );
    }

    int class_c = class_counter->get<int>( "<xmlattr>.covered", 0 ) + 1;
    int class_m = class_counter->get<int>( "<xmlattr>.missed", 0 );
    int methods_c = method_counter->get<int>( "<xmlattr>.covered", 0 ) + _covered;
    int methods_m = method_counter->get<int>( "<xmlattr>.missed", 0 ) + _missed;
    int line_c = line_counter->get<int>( "<xmlattr>.covered", 0 ) + _covered;
    int line_m = line_counter->get<int>( "<xmlattr>.missed", 0 ) + _missed;

    class_counter->put( "<xmlattr>.covered", class_c );
    class_counter->put( "<xmlattr>.missed", class_m );
    method_counter->put( "<xmlattr>.covered", methods_c );
    method_counter->put( "<xmlattr>.missed", methods_m );
    line_counter->put( "<xmlattr>.covered", methods_c );
    line_counter->put( "<xmlattr>.missed", methods_m );

    covered += _covered;
    missed += _missed;
  }

  void emitPackages(ptree& tree, const std::vector<PythonCoverage::Package>& packages, unsigned& covered, unsigned& missed)
  {
    for( auto p: packages ) {
      unsigned _covered = 0;
      unsigned _missed = 0;
      ptree package;
      package.put( "<xmlattr>.name", p.name );

      ptree sourcefile;
      sourcefile.put( "<xmlattr>.name", p.name ); // TODO: determine name properly

      llvm::outs() << "Reporting: " << p.name << ":" << "\n";

      if( !p.classes.empty() ) {
        for( const PythonCoverage::Class& c: p.classes ) {
          ptree* class_src = nullptr;
          try {
          for( auto src: package.get_child( "sourcefile") ) {
            if( src.second.get<std::string>( "<xmlattr>.name" ) == c.file_name ) {
              class_src = &src.second;
              break;
            }
          }
          } catch(...) {
            ptree src;
            src.put( "<xmlattr>.name", c.file_name );
            class_src = &package.add_child( "sourcefile", src );
          }
          emitClass( package, *class_src, c, _covered, _missed );
          package.add_child( "sourcefile", *class_src );
        }
      }

      if( !p.methods.empty() ) {
        for( const PythonCoverage::Method& m: p.methods ) {
          emitMethod( package, sourcefile, m, _covered, _missed );
        }
        package.add_child( "sourcefile", sourcefile );
      }

      if ( _covered or _missed ) { // Emit only known packages
        ptree counter_methods;
        counter_methods.put( "<xmlattr>.type", "METHOD" );
        counter_methods.put( "<xmlattr>.covered", _covered );
        counter_methods.put( "<xmlattr>.missed", _missed );

        package.add_child( "counter", counter_methods );

        ptree counter_classes;
        counter_classes.put( "<xmlattr>.type", "CLASS" );
        counter_classes.put( "<xmlattr>.covered", p.classes.size() );
        counter_classes.put( "<xmlattr>.missed", 0 );

        package.add_child( "counter", counter_classes );

        tree.add_child( "package", package );

        covered += _covered;
        missed += _missed;
      }
    }
  }

public:
  CoverageExporterJacoco()
  {
  }

  /// \brief Print the CoverageMapping.
  void print(const PythonCoverage& CoverageMapping, const std::string& file)
  {
    unsigned covered = 0;
    unsigned missed = 0;
    time_t stamp = time(0);
    ptree report;
    report.put( "<xmlattr>.name", Config::get().root_module );
    ptree sessioninfo;
    sessioninfo.put( "<xmlattr>.id", Config::get().root_module );
    sessioninfo.put( "<xmlattr>.start", stamp );
    report.add_child( "sessioninfo", sessioninfo );

    ptree group;
    group.put( "<xmlattr>.name", Config::get().root_module );

    emitPackages( group, CoverageMapping.packages, covered, missed );

    ptree counter;
    counter.put( "<xmlattr>.type", "METHOD" );
    counter.put( "<xmlattr>.covered", covered );
    counter.put( "<xmlattr>.missed", missed );

    group.add_child( "counter", counter );

    report.add_child( "group", group );

    ptree overal_counter;
    overal_counter.put( "<xmlattr>.type", "METHOD" );
    overal_counter.put( "<xmlattr>.covered", covered );
    overal_counter.put( "<xmlattr>.missed", missed );

    report.add_child( "counter", overal_counter );

    doc.add_child( "report", report );

    boost::property_tree::xml_writer_settings<std::string> w( ' ', 2 );
    boost::property_tree::write_xml( file, doc, std::locale(), w );

    std::error_code EC;
    std::basic_ofstream<ptree::key_type::value_type> stream( file.c_str() );
    stream.imbue( std::locale() );

    stream << "<?xml version=\"1.0\" encoding=\""
           << w.encoding
           << "\"?>\n";
    stream << JACOCO_DTD "\n";

    boost::property_tree::xml_parser::write_xml_element(stream, std::basic_string<ptree::key_type::value_type>(), doc, -1, w);

    stream.close();
  }
};

Reporter& Reporter::get()
{
	static Reporter instance;
	return instance;
}

void Reporter::write_report(const std::string& path)
{
	CoverageExporterJacoco exporter;
	exporter.print(Coverage, path);
}
