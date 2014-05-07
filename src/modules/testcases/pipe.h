/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2014  <copyright holder> <email>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef INSIGHT_PIPE_H
#define INSIGHT_PIPE_H

#include "base/linearalgebra.h"
#include "openfoam/openfoamanalysis.h"
#include "openfoam/openfoamcaseelements.h"

namespace insight {
  

class PipeBase 
: public OpenFOAMAnalysis
{
public:
  /**
   * convert friction velocity Reynolds number into bulk velocity Re
   */
  static double Re(double Retau);

  /**
   * convert friction velocity Reynolds number into bulk velocity Re
   */
  static double Retau(double Re);

  /**
   * compute factor Umax/Ubulk
   */
  static double UmaxByUbulk(double Retau);
  
protected:
  std::string cycl_in_, cycl_out_;
  double Lc_;
  int nc_, nr_;
  double nu_, gradr_, ywall_, Re_, Ubulk_, T_, utau_;
  
public:
  declareType("Pipe Flow Test Case");
  
  PipeBase(const NoParameters&);
  ~PipeBase();
  
  virtual ParameterSet defaultParameters() const;
  
  std::string cyclPrefix() const;
  virtual void calcDerivedInputData(const ParameterSet& p);
//   virtual double calcLc(const ParameterSet& p) const;
//   virtual int calcnc(const ParameterSet& p) const;
//   virtual int calcnr(const ParameterSet& p) const;
//   virtual double calcgradr(const ParameterSet& p) const;
//   virtual double calcywall(const ParameterSet& p) const;
//   virtual double calcRe(const ParameterSet& p) const;
//   virtual double calcUbulk(const ParameterSet& p) const;
//   virtual double calcT(const ParameterSet& p) const;
//   virtual double calcUtau(const ParameterSet& p) const;

  virtual void createMesh
  (
    OpenFOAMCase& cm,
    const ParameterSet& p
  );
  
  virtual void createCase
  (
    OpenFOAMCase& cm,
    const ParameterSet& p
  );

  virtual void evaluateAtSection(
    OpenFOAMCase& cm, const ParameterSet& p, 
    ResultSetPtr results, double x, int i
  );

  virtual ResultSetPtr evaluateResults(OpenFOAMCase& cm, const ParameterSet& p);
  
};




class PipeCyclic
: public PipeBase
{
public:
  declareType("Pipe Flow Test Case (Axial Cyclic)");
  
  PipeCyclic(const NoParameters&);
  
  virtual void createMesh
  (
    OpenFOAMCase& cm,
    const ParameterSet& p
  );  
  
  virtual void createCase
  (
    OpenFOAMCase& cm,
    const ParameterSet& p
  );

  virtual void applyCustomOptions(OpenFOAMCase& cm, const ParameterSet& p, boost::shared_ptr<OFdicts>& dicts);
  virtual void applyCustomPreprocessing(OpenFOAMCase& cm, const ParameterSet& p);
  
};


class PipeInflow
: public PipeBase
{
  
  const static int ntpc_ = 4;
  const static char* tpc_names_[ntpc_]; 
  const static double tpc_xlocs_[ntpc_];
  
public:
  declareType("Pipe Flow Test Case (Inflow Generator)");
  
  PipeInflow(const NoParameters&);
  
  ParameterSet defaultParameters() const;
  
  virtual void createMesh
  (
    OpenFOAMCase& cm,
    const ParameterSet& p
  );  
  
  virtual void createCase
  (
    OpenFOAMCase& cm,
    const ParameterSet& p
  );

  ResultSetPtr evaluateResults(OpenFOAMCase& cm, const ParameterSet& p);

  virtual void applyCustomOptions(OpenFOAMCase& cm, const ParameterSet& p, boost::shared_ptr<OFdicts>& dicts);
  virtual void applyCustomPreprocessing(OpenFOAMCase& cm, const ParameterSet& p);
  
};


}

#endif // INSIGHT_PIPE_H
