/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "base/boost_include.h"
#include "base/linearalgebra.h"
#include "base/analysis.h"
#include "openfoam/openfoamtools.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

#include <QApplication>
#include "mainwindow.h"

using namespace std;
using namespace insight;
using namespace boost;
namespace bf = boost::filesystem;


int main(int argc, char *argv[])
{
    insight::UnhandledExceptionHandling ueh;
    insight::GSLExceptionHandling gsl_errtreatment;

    namespace po = boost::program_options;

    typedef std::vector<string> StringList;

    StringList cmds, icmds;

    // Declare the supported options.
    po::options_description desc("Allowed options");
    desc.add_options()
    ("help,h", "produce help message")

    //("ofe,o", po::value<std::string>(), "use specified OpenFOAM environment instead of detected")
     ("meta-file,m", po::value<std::string>(), "use the specified remote execution config file instead of \"meta.foam\"")
     ("case-dir,d", po::value<std::string>(), "case location")
     ("sync-remote,r", "sync current case to remote location")
     ("sync-local,l", "sync from remote location to current case")
     ("include-processor-dirs,p", "if this flag is set, processor directories will be tranferred as well")
     ("command,q", po::value<StringList>(&cmds), "add this command to remote execution queue (will be executed after the previous command has finished successfully)")
     ("immediate-command,i", po::value<StringList>(&icmds), "add this command to remote execution queue (execute in parallel without waiting for the previous command)")
     ("wait,w", "wait for command queue completion")
     ("wait-last,W", "wait for the last added command to finish")
     ("cancel,c", "cancel remote commands (remove all from queue)")
     ("clean,x", "remove the remote case directory from server")
    ;

    po::positional_options_description p;
    p.add("command", -1);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
              options(desc).positional(p).run(), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
        cout << desc << endl;
        exit(-1);
    }

    boost::filesystem::path location=".";
    if (vm.count("case-dir"))
    {
        location = vm["case-dir"].as<std::string>();
    }

    bool include_processor=false;
    if (vm.count("include-processor-dirs")>0)
        include_processor=true;

    bfs_path mf="";
    if (vm.count("meta-file"))
        mf=vm["meta-file"].as<std::string>();

    try
    {
        if (
                vm.count("sync-remote")==0
                &&
                vm.count("sync-local")==0
                &&
                vm.count("command")==0
                &&
                vm.count("immediate-command")==0
                &&
                vm.count("wait")==0
                &&
                vm.count("wait-last")==0
                &&
                vm.count("clean")==0
                &&
                vm.count("cancel")==0
                )
        {
          QApplication app(argc, argv);
          MainWindow w;
          w.show();
          return app.exec();
        }
        else
        {
            insight::RemoteExecutionConfig re(location, true, mf);

            if(vm.count("cancel"))
                re.cancelRemoteCommands();

            if (vm.count("sync-remote"))
                re.syncToRemote();

            if (vm.count("command"))
            {
                for (const auto& c: cmds)
                {
                    re.queueRemoteCommand(c);
                }
            }

            if (vm.count("immediate-command"))
            {
                for (const auto& c: icmds)
                {
                    re.queueRemoteCommand(c, false);
                }
            }

            if (vm.count("wait"))
                re.waitRemoteQueueFinished();

            if (vm.count("wait-last"))
                re.waitLastCommandFinished();

            if(vm.count("sync-local"))
                re.syncToLocal();

            if(vm.count("clean"))
                re.removeRemoteDir();
        }
    }
    catch (insight::Exception e)
    {
        cout<<"Error: "<<e<<endl;
        exit(-1);
    }
    catch (std::exception e)
    {
        cout<<"Error: "<<e.what()<<endl;
        exit(-1);
    }
}
