// Copyright (c) 2018 Vadym Yatsenko <vadim.yatsenko@gmail.com>
//
// All rights reserved. Use of this source code is governed by a
// MIT license that can be found in the LICENSE file.

/// @file   binder/reporter.hpp
/// @brief  Binding statistic reporting
/// @author Vadym Yatsenko

#ifndef _INCLUDED_reporter_h_
#define _INCLUDED_reporter_h_

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <map>

#include <clang/AST/Decl.h>
#include <clang/AST/ASTContext.h>

#include <util.hpp>

namespace binder {

struct PythonCoverage {
  struct Method {
    std::string name;
    std::string desc;
    int declaration_line;
    int usages;
  };
  struct Class {
    std::string name;
    std::string file_name;
    std::vector<Method> methods;
  };
  /**
   * For C++ this represents a namespace
   */
  struct Package {
    std::string name;
    std::vector<Class> classes;
    std::vector<Method> methods;   // non-class functions
  };
  std::vector<Package> packages;

  void addDeclaration( const clang::FunctionDecl* F, clang::ASTContext* ast_context )
  {
    clang::SourceManager& SM = ast_context->getSourceManager();
    auto loc = clang::FullSourceLoc( F->getLocation(), SM );
    auto ploc = SM.getPresumedLoc( loc );
    std::string file_name( ploc.getFilename() );
    int line_number = ploc.getLine();

    std::string method_name = F->getNameAsString();
    std::string method_desc;
    method_desc += "(";
    int args = F->getNumParams();
    for(int i = 0; i < args; i++) {
      if (i > 0) {
        method_desc += ";";
      }
      method_desc += F->getParamDecl(i)->getOriginalType().getAsString();
    }
    method_desc += ")";
    method_desc += F->getReturnType().getAsString();
    llvm::outs() << "Decl: " << method_name << method_desc;

    Method m;
    m.name = method_name;
    m.desc = method_desc;
    m.declaration_line = line_number;
    m.usages = -1;

    const clang::DeclContext* parent = F->getParent();
    if( const clang::NamespaceDecl* ns = clang::dyn_cast<clang::NamespaceDecl>( parent ) ) {
      std::string path = ns->getQualifiedNameAsString();
      Package& p = getPackage( path );
      llvm::outs() << " in package: " << p.name;
      p.methods.push_back( m );
    } else if( const clang::CXXRecordDecl* cl = clang::dyn_cast<clang::CXXRecordDecl>( parent ) ) {
      Class& c = getClass( cl->getQualifiedNameAsString() );
      llvm::outs() << " in class: " << c.name;
      if( c.file_name.empty() ) {
        const char* source_root = std::getenv( "SOURCE_DIR" );
        if( source_root ) {
          if( file_name.find( source_root ) == 0 and
              file_name.find( "../" ) == file_name.npos // only absolute path
          ) {
            file_name.erase( 0, strlen(source_root) + 1 );
          } else {
            // Not project source
            return;
          }
        }
        c.file_name = file_name;
      }
      c.methods.push_back( m );
    } else {
      Package& p = getPackage( "" );
      llvm::outs() << " in package: " << p.name;
      p.methods.push_back( m );
    }

    llvm::outs() << "\n";
  }

  Package& getPackage( const std::string& path )
  {
    std::string name( path );

    if( name.empty() ) {
      name = "unknown";
    }

    for( Package& p : packages ) {
      if( p.name == name ) {
        return p;
      }
    }

    Package p;
    p.name = name;
    packages.push_back( p );
    return packages.back();
  }

  Class& getClass( const std::string& path )
  {
    const std::string parent = base_namespace( path );
    const std::string name = last_namespace( path );

    Package& p = getPackage( parent );

    for( Class& c : p.classes ) {
      if( c.name == path ) {
        return c;
      }
    }

    Class c;
    c.name = path;
    p.classes.push_back( c );

    return p.classes.back();
  }

  void useMethod( const clang::FunctionDecl* F )
  {
    std::string name = F->getNameAsString();

    llvm::outs() << "Looking for: " << name;

    const clang::DeclContext* parent = F->getParent();
    if( const clang::NamespaceDecl* ns = clang::dyn_cast<clang::NamespaceDecl>( parent ) ) {
      std::string path = ns->getQualifiedNameAsString();
      Package& p = getPackage( path );
      llvm::outs() << " in package: " << p.name;
      for( Method& m: p.methods ) {
        if( m.name == name ) {
          m.usages = 1;
          break;
        }
      }
    } else if( const clang::CXXRecordDecl* cl = clang::dyn_cast<clang::CXXRecordDecl>( parent ) ) {
      Class& c = getClass( cl->getQualifiedNameAsString() );
      llvm::outs() << " in class: " << c.name;
      for( Method& m: c.methods ) {
        if( m.name == name ) {
          m.usages = 1;
          break;
        }
      }
    } else {
      Package& p = getPackage( "" );
      llvm::outs() << " in package: " << p.name;
      for( Method& m: p.methods ) {
        if( m.name == name ) {
          m.usages = 1;
          break;
        }
      }
    }
  }
};

class Reporter {
  friend class CoverageExporterJacoco;

  PythonCoverage Coverage;

  Reporter() = default;

public:
  static Reporter& get();

  ~Reporter() = default;

  void add_method_decl( const clang::FunctionDecl* F, clang::ASTContext* ast_context )
  {
    Coverage.addDeclaration( F, ast_context );
  }

  void mark_method_used( const clang::FunctionDecl* F )
  {
    llvm::outs() << "Usage+ " << F->getQualifiedNameAsString() << "\n";
    Coverage.useMethod( F );
  }

  void write_report( const std::string& path );
};

} // namespace binder

#endif /* _INCLUDED_reporter_h_ */
