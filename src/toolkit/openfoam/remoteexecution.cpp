#include "remoteexecution.h"

#include <cstdlib>
#include "base/exception.h"
#include "openfoam/openfoamcase.h"

namespace insight
{


void RemoteExecutionConfig::execRemoteCmd(const std::string& command)
{
    std::ostringstream cmd;

    cmd << "ssh " << server_ << " \"";
     cmd << "export TS_SOCKET="<<(remoteDir_/"tsp.socket")<<";";

     try {
         const OFEnvironment& cofe = OFEs::getCurrent();
         cmd << "source " << cofe.bashrc().filename() << ";";
     } catch (insight::Exception e) {
         // ignore, don't load OF config remotely
     }

     cmd << "cd "<<remoteDir_<<" && (";
     cmd << command;
    cmd << ")\"";

    std::cout<<cmd.str()<<std::endl;
    if (! ( std::system(cmd.str().c_str()) == 0 ))
    {
        throw insight::Exception("Could not execute command on server "+server_+": \""+cmd.str()+"\"");
    }
}

bool RemoteExecutionConfig::isValid() const
{
    return (!server_.empty())&&(!remoteDir_.empty());
}

RemoteExecutionConfig::RemoteExecutionConfig(const boost::filesystem::path& location, bool needConfig, const bfs_path& meta_file)
  : localDir_(location)
{
  if (meta_file.empty()) {
      meta_file_ = location/"meta.foam";
  } else {
      meta_file_ = meta_file;
  }

  CurrentExceptionContext ce("reading configuration for remote execution in  directory "+location.string()+" from file "+meta_file_.string());

  if (!boost::filesystem::exists(meta_file_))
  {
      if (needConfig)
          throw insight::Exception("There is no remote execution configuration file present!");
  }
  else {
    std::ifstream f(meta_file_.c_str());
    std::string line;
    if (!getline(f, line))
      throw insight::Exception("Could not read first line from file "+meta_file_.string());

    std::vector<std::string> pair;
    boost::split(pair, line, boost::is_any_of(":"));
    if (pair.size()!=2)
      throw insight::Exception("Error reading "+meta_file_.string()+": expected <server>:<remote directory>, got "+line);

    server_=pair[0];
    remoteDir_=pair[1];

    std::cout<<"configured "<<server_<<":"<<remoteDir_<<std::endl;
  }
}

const std::string& RemoteExecutionConfig::server() const
{
  return server_;
}

const boost::filesystem::path& RemoteExecutionConfig::localDir() const
{
  return localDir_;
}

const boost::filesystem::path& RemoteExecutionConfig::remoteDir() const
{
  return remoteDir_;
}


void RemoteExecutionConfig::syncToRemote()
{
    std::ostringstream cmd;

    cmd << "rsync -avz --delete --exclude 'processor*' --exclude '*.foam' --exclude '*.socket' . \""<<server_<<":"<<remoteDir_.string()<<"\"";

    std::system(cmd.str().c_str());
}

void RemoteExecutionConfig::syncToLocal()
{
    std::ostringstream cmd;

    cmd << "rsync -avz --exclude 'processor*' --exclude '*.foam' --exclude '*.socket' \""<<server_<<":"<<remoteDir_.string()<<"/*\" .";

    std::system(cmd.str().c_str());
}

void RemoteExecutionConfig::queueRemoteCommand(const std::string& command, bool waitForPreviousFinished)
{
  if (waitForPreviousFinished)
      execRemoteCmd("tsp -d " + command);
  else
      execRemoteCmd("tsp " + command);
}


void RemoteExecutionConfig::waitRemoteQueueFinished()
{
    execRemoteCmd("while tsp -c; do tsp -C; done");
}

void RemoteExecutionConfig::waitLastCommandFinished()
{
    execRemoteCmd("tsp -t");
}

void RemoteExecutionConfig::cancelRemoteCommands()
{
    execRemoteCmd("tsp -C; tsp -k; tsp -K");
}

void RemoteExecutionConfig::removeRemoteDir()
{
    execRemoteCmd("tsp -C; tsp -k; tsp -K");
    execRemoteCmd("rm -r *; cd ..; rmdir "+remoteDir_.string());
}

}
