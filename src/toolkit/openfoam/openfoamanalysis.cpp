/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2013  Hannes Kroeger <email>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "openfoamanalysis.h"
#include "openfoamtools.h"
#include "openfoamcaseelements.h"

#include <boost/assign/list_of.hpp>
#include <boost/assign/ptr_map_inserter.hpp>
#include "boost/lexical_cast.hpp"
#include "boost/regex.hpp"
#include "boost/filesystem.hpp"

using namespace boost;
using namespace boost::assign;
using namespace boost::filesystem;
using namespace std;

namespace insight
{

void insertTurbulenceModel(OpenFOAMCase& cm, const std::string& name)
{
  turbulenceModel* model = turbulenceModel::lookup(name, cm);
  
  if (!model) 
    throw insight::Exception("Unrecognized RASModel selection: "+name);
  
  cm.insert(model);
}


void OpenFOAMAnalysis::cancel()
{
  stopFlag_=true;
}

OpenFOAMAnalysis::OpenFOAMAnalysis(const std::string& name, const std::string& description)
: Analysis(name, description)
{
}

ParameterSet OpenFOAMAnalysis::defaultParameters() const
{

  return ParameterSet
  (
    boost::assign::list_of<ParameterSet::SingleEntry>
      
      ("run", new SubsetParameter	
	    (
		  ParameterSet
		  (
		    boost::assign::list_of<ParameterSet::SingleEntry>
		    ("machine", 	new StringParameter("", "machine or queue, where the external commands are executed on"))
		    ("OFEname", 	new StringParameter("OF22x", "identifier of the OpenFOAM installation, that shall be used"))
		    ("np", 		new IntParameter(1, "number of processors for parallel run (less or equal 1 means serial execution)"))
		    ("deltaT", 		new DoubleParameter(1.0, "simulation time step"))
		    ("endTime", 	new DoubleParameter(1000.0, "simulation time at which the solver should stop"))
		    ("mapFrom", 	new DirectoryParameter("", "Map solution from specified case, potentialinit is skipped when specified"))
		    ("potentialinit", 	new BoolParameter(false, "Whether to initialize the flow field by potentialFoam when no mapping is done"))
		    ("evaluateonly", 	new BoolParameter(false, "Whether to skip solver run and do only the evaluation"))
		    .convert_to_container<ParameterSet::EntryList>()
		  ), 
		  "Execution parameters"
      ))

      ("fluid", new SubsetParameter
	(
	  ParameterSet
	  (
	    boost::assign::list_of<ParameterSet::SingleEntry>
	    ("turbulenceModel",new SelectionParameter(0, turbulenceModel::factoryToC(), "Turbulence model"))
	    .convert_to_container<ParameterSet::EntryList>()
	  ), 
	  "Parameters of the fluid"
	))
      
//       ("run", new SubsetParameter	
// 	    (
// 		  ParameterSet
// 		  (
// 		    boost::assign::list_of<ParameterSet::SingleEntry>
// 		    ("endTime", 	new DoubleParameter(1000.0, "simulation time at which the solver should stop"))
// 		    .convert_to_container<ParameterSet::EntryList>()
// 		  ), 
// 		  "Solver parameters"
//       ))

      .convert_to_container<ParameterSet::EntryList>()
  );
}

void OpenFOAMAnalysis::calcDerivedInputData(const ParameterSet& p)
{
}

void OpenFOAMAnalysis::createDictsInMemory(OpenFOAMCase& cm, const ParameterSet& p, boost::shared_ptr<OFdicts>& dicts)
{
  dicts=cm.createDictionaries();
}

void OpenFOAMAnalysis::applyCustomOptions(OpenFOAMCase& cm, const ParameterSet& p, boost::shared_ptr<OFdicts>& dicts)
{
  PSINT(p, "run", np);
  PSDBL(p, "run", deltaT);
  PSDBL(p, "run", endTime);

  OFDictData::dict& controlDict=dicts->addDictionaryIfNonexistent("system/controlDict");
  controlDict["deltaT"]=deltaT;
  controlDict["endTime"]=endTime;
  
  OFDictData::dict& decomposeParDict=dicts->addDictionaryIfNonexistent("system/decomposeParDict");
  decomposeParDict["numberOfSubdomains"]=np;
  decomposeParDict["method"]="scotch";
}

void OpenFOAMAnalysis::writeDictsToDisk(OpenFOAMCase& cm, const ParameterSet& p, boost::shared_ptr<OFdicts>& dicts)
{
  cm.createOnDisk(executionPath(), dicts);
}

void OpenFOAMAnalysis::applyCustomPreprocessing(OpenFOAMCase& cm, const ParameterSet& p)
{
}

void OpenFOAMAnalysis::runSolver(ProgressDisplayer* displayer, OpenFOAMCase& cm, const ParameterSet& p)
{
  SolverOutputAnalyzer analyzer(*displayer);
  
  string solverName;
  int np=readDecomposeParDict(executionPath());
  
  {
    OFDictData::dict controlDict;
    std::ifstream cdf( (executionPath()/"system"/"controlDict").c_str() );
    readOpenFOAMDict(cdf, controlDict);
    solverName=controlDict.getString("application");
  }

  
  bool is_parallel = np>1;
  
  std::cout<<"Executing application "<<solverName<<std::endl;
  path mapFromPath=p.getPath("run/mapFrom");
  
  if ((cm.OFversion()>=230) && (mapFromPath!=""))
  {
    // parallelTarget option is not present in OF2.3.x
    mapFields(cm, mapFromPath, executionPath(), false);
  }

  if (is_parallel)
  {
    cm.executeCommand(executionPath(), "decomposePar");
  }
  
  if (!cm.outputTimesPresentOnDisk(executionPath()))
  {
    if ( (!(cm.OFversion()>=230)) && (mapFromPath!=""))
    {
      mapFields(cm, mapFromPath, executionPath(), is_parallel);
    }
    else
    {
      if (p.getBool("run/potentialinit"))
	runPotentialFoam(cm, executionPath(), &stopFlag_, np);
    }
  }
  else
  {
    cout<<"case in "<<executionPath()<<": output timestep are already there, skipping initialization."<<endl;
  }
  
  cm.runSolver(executionPath(), analyzer, solverName, &stopFlag_, np);
  
  if (is_parallel)
  {
    cm.executeCommand(executionPath(), "reconstructPar", list_of<string>("-latestTime") );
  }
}

ResultSetPtr OpenFOAMAnalysis::evaluateResults(OpenFOAMCase& cm, const ParameterSet& p)
{
  ResultSetPtr results(new ResultSet(p, name_, "Result Report"));
  return results;
}

ResultSetPtr OpenFOAMAnalysis::operator()(ProgressDisplayer* displayer)
{  
  const ParameterSet& p = *parameters_;
  
  PSSTR(p, "run", machine);
  PSSTR(p, "run", OFEname);
  
  OFEnvironment ofe = OFEs::get(OFEname);
  ofe.setExecutionMachine(machine);
  
  path dir = setupExecutionEnvironment();
  
  calcDerivedInputData(p);

  OpenFOAMCase runCase(ofe);
  
  if (p.getBool("run/evaluateonly"))
    cout<< "Parameter \"run/evaluateonly\" is set: SKIPPING SOLVER RUN AND PROCEEDING WITH EVALUATION!" <<endl;

  if (!p.getBool("run/evaluateonly"))
  {
    p.saveToFile(dir/"parameters.ist", type());
    
    {
      OpenFOAMCase meshCase(ofe);
      if (!meshCase.meshPresentOnDisk(dir))
	createMesh(meshCase, p);
      else
	cout<<"case in "<<dir<<": mesh is already there, skipping mesh creation."<<endl;
    }
  }

  createCase(runCase, p);
  boost::shared_ptr<OFdicts> dicts;
  createDictsInMemory(runCase, p, dicts);
  applyCustomOptions(runCase, p, dicts);
  
  if (!runCase.outputTimesPresentOnDisk(dir))
  {
    writeDictsToDisk(runCase, p, dicts);
    applyCustomPreprocessing(runCase, p);
  }
  else
    cout<<"case in "<<dir<<": output timestep are already there, skipping case recreation."<<endl;    
    
  if (!p.getBool("run/evaluateonly"))
  {
    runSolver(displayer, runCase, p);
  }
  
  return evaluateResults(runCase, p);
}

}

