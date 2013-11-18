/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2013  hannes <email>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "analysisform.h"
#include "ui_analysisform.h"
#include "parameterwrapper.h"

#include <QMessageBox>
#include <QFileDialog>

int metaid1=qRegisterMetaType<insight::ParameterSet>("insight::ParameterSet");
int metaid2=qRegisterMetaType<insight::ResultSetPtr>("insight::ResultSetPtr");


AnalysisWorker::AnalysisWorker(const boost::shared_ptr<insight::Analysis>& analysis)
: analysis_(analysis)
{}

void AnalysisWorker::doWork(const insight::ParameterSet& p, insight::ProgressDisplayer* pd)
{
  emit resultReady( (*analysis_)(p, pd) );
}

AnalysisForm::AnalysisForm(QWidget* parent, const std::string& analysisName)
: QMdiSubWindow(parent)
{
  
  insight::Analysis::FactoryTable::const_iterator i = insight::Analysis::factories_.find(analysisName);
  if (i==insight::Analysis::factories_.end())
    throw insight::Exception("Could not lookup analysis type "+analysisName);
  
  analysis_.reset( (*i->second)( insight::NoParameters() ) );
  parameters_ = analysis_->defaultParameters();
  
  ui = new Ui::AnalysisForm;
  QWidget* iw=new QWidget(this);
  ui->setupUi(iw);
  setWidget(iw);
  
  progdisp_=new GraphProgressDisplayer(ui->runTab);
  ui->runTabLayout->addWidget(progdisp_);
  
  this->setWindowTitle(analysis_->getName().c_str());
  connect(ui->runBtn, SIGNAL(clicked()), this, SLOT(onRunAnalysis()));
  connect(ui->killBtn, SIGNAL(clicked()), this, SLOT(onKillAnalysis()));

  connect(ui->saveParamBtn, SIGNAL(clicked()), this, SLOT(onSaveParameters()));
  connect(ui->loadParamBtn, SIGNAL(clicked()), this, SLOT(onLoadParameters()));

  addWrapperToWidget(parameters_, ui->inputContents, this);
      
}

AnalysisForm::~AnalysisForm()
{
  workerThread_.quit();
  workerThread_.wait();
  delete ui;
}

void AnalysisForm::onSaveParameters()
{
  QString fn = QFileDialog::getSaveFileName(this, "Save Parameters", QString(), "*.xml");
  if (!fn.isEmpty())
  {
    parameters_.saveToFile(fn.toStdString());
  }
}

void AnalysisForm::onLoadParameters()
{
}

void AnalysisForm::onRunAnalysis()
{
  if (!workerThread_.isRunning())
  {
    emit apply();
    
    progdisp_->reset();
    
    AnalysisWorker *worker = new AnalysisWorker(analysis_);
    worker->moveToThread(&workerThread_);
    connect(&workerThread_, SIGNAL(finished()), worker, SLOT(deleteLater()));
    connect(this, SIGNAL(runAnalysis(const insight::ParameterSet&, insight::ProgressDisplayer*)), 
	    worker, SLOT(doWork(const insight::ParameterSet&, insight::ProgressDisplayer*)));
    connect(worker, SIGNAL(resultReady(insight::ResultSetPtr)), this, SLOT(onResultReady(insight::ResultSetPtr)));
    workerThread_.start();

    ui->tabWidget->setCurrentWidget(ui->runTab);
    //(*analysis_)(parameters_, progdisp_);
    emit runAnalysis(parameters_, progdisp_);
  }
}

void AnalysisForm::onKillAnalysis()
{
  if (workerThread_.isRunning())
  {
    analysis_->cancel();
    workerThread_.quit();
    workerThread_.wait();
  }
}

void AnalysisForm::onResultReady(insight::ResultSetPtr results)
{
  QMessageBox::information(this, "Finished!", "The analysis has finished");
  results->writeLatexFile( "report.tex" );
}


