/*
 *  Copyright 2013, DFKI GmbH Robotics Innovation Center
 *
 *  This file is part of the MARS simulation framework.
 *
 *  MARS is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation, either version 3
 *  of the License, or (at your option) any later version.
 *
 *  MARS is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with MARS.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/**
 * \file EnvireMls.cpp
 * \author Raul (Raul.Dominguez@dfki.de)
 * \brief Provides
 *
 * Version 0.1
 */

#include "EnvireMls.hpp"

#include <fstream>
#include <boost/archive/binary_iarchive.hpp>
#include <envire_core/items/Transform.hpp>


#include <envire_core/graph/GraphViz.hpp>

#include <mars/sim/NodePhysics.h>

// Hardcoded parameters:
#define MLS_NAME std::string("mls_01")
#define MLS_FRAME_NAME std::string("mls_01")
#define DUMPED_MLS_FRAME_NAME std::string("mls_map")
#define SIM_CENTER_FRAME_ID std::string("center")
//#define TEST_MLS_PATH std::string("./testMlsData/MLSMapKalman_waves.bin")
//#define TEST_MLS_PATH std::string("/home/dfki.uni-bremen.de/rdominguez/Entern/old_navigation/simulation/mars/plugins/envire_mls/testMlsData/MLSMapKalman_waves.bin")
//#define TEST_MLS_PATH std::string("/home/dfki.uni-bremen.de/rdominguez/Entern/old_navigation/models/environments/dlr_map/mls/mls_map.bin")
#define TEST_MLS_PATH std::string("/home/dfki.uni-bremen.de/rdominguez/Entern/old_navigation/simulation/mars/plugins/envire_mls/testMlsData/crater_simulation_mls.graph")
#define MLS_FRAME_TF_X 1
#define MLS_FRAME_TF_Y 0
#define MLS_FRAME_TF_Z 0

#define GD_SENSE_CONTACT_FORCE 0
#define GD_PARENT_GEOM 0
#define GD_C_PARAMS_CFM 0.001
#define GD_C_PARAMS_ERP 0.001
#define GD_C_PARAMS_BOUNCE 0.0

#define DEBUG 1

namespace mars {
  namespace plugins {
    namespace envire_mls {

      using namespace mars::utils;
      using namespace mars::interfaces;

      using namespace envire::core;
      using namespace maps::grid;


      EnvireMls::EnvireMls(lib_manager::LibManager *theManager)
        : MarsPluginTemplate(theManager, "EnvireMls") 
      {
        mlsCollision = envire::collision::MLSCollision::getInstance();
      }

      void EnvireMls::init() 
      {
#ifdef DEBUG
        LOG_DEBUG( "[EnvireMls::init]"); 
#endif
        // Create the default frame for the MLS 
        envire::core::FrameId mlsFrameId = MLS_FRAME_NAME; 
        control->graph->addFrame(mlsFrameId);
        envire::core::Transform mlsTf(base::Time::now());
        mlsTf.transform.translation << MLS_FRAME_TF_X, MLS_FRAME_TF_Y, MLS_FRAME_TF_Z;
        mlsTf.transform.orientation = base::AngleAxisd(0.25, base::Vector3d::UnitX());
        control->graph->addTransform(MLS_FRAME_NAME, SIM_CENTER_FRAME_ID, mlsTf);

        tested = false;
      }

      void EnvireMls::reset() { }

      EnvireMls::~EnvireMls() { }

      void EnvireMls::update(sReal time_ms) 
      {
        if (not tested){
          testAddMLS();
          tested = true;
        }
      }

      void EnvireMls::loadMLSMap(const std::string & mlsPath)
      {
        /* Loads in the envire graph the mls given in the path after
         * deserializing it.
         */
        EnvireGraph auxMlsGraph;
        auxMlsGraph.loadFromFile(mlsPath);
        FrameId dumpedFrameId(DUMPED_MLS_FRAME_NAME);
        mlsType mlsAux = getMLSMap(auxMlsGraph, dumpedFrameId);
        //mlsType mlsAux = getMLSMap(mlsGraph, dumpedFrameId);
        Item<mlsType>::Ptr mlsItemPtr(new Item<mlsType>(mlsAux));
        FrameId targetFrameId(MLS_FRAME_NAME);
        control->graph->addItemToFrame(targetFrameId, mlsItemPtr);
      }

      mlsType EnvireMls::getMLSMap(const envire::core::EnvireGraph & graph, envire::core::FrameId mlsFrameId)
      {
        EnvireGraph::ItemIterator<Item<mlsType>> beginItem, endItem;
        boost::tie(beginItem, endItem) = graph.getItems<Item<mlsType>>(mlsFrameId);
        mlsType mls = beginItem->getData();
        return mls;
      }

      NodeData* EnvireMls::setUpNodeData()
      {
        /*
         * Look up the stored mls map and generate the correspondent MLSNodeData
         */
        NodeData* node(new NodeData);
        node->init(MLS_NAME, mars::utils::Vector(0,0,0));
        node->physicMode = interfaces::NODE_TYPE_MLS;
        //node->env_path = mlsPath;
        FrameId mlsFrameId(MLS_FRAME_NAME);
        mlsType mls = getMLSMap(*(control->graph), mlsFrameId);
	boost::shared_ptr<maps::grid::MLSMapKalman> mlsPtr(& mls);
        // Store MLS geometry in simulation node
        node->g_mls = (void*)(mlsCollision->createNewCollisionObject(mlsPtr));//_userdata);	

        // NOTE What is this pos1 for?
        mars::utils::Vector pos1(1,1,1);
        node->pos = pos1;

        // The position should be read from the envire graph

        dVector3 pos;
        pos[ 0 ] = 0;
        pos[ 1 ] = 0;
        pos[ 2 ] = 0;

        // Rotate so Z is up, not Y (which is the default orientation)
        // NOTE is this to be done for all MLS or only for this particular case?
        dMatrix3 R;
        dRSetIdentity( R );
        //dRFromAxisAndAngle( R, 1, 0, 0, (3.141592/180) * 90 );  //DEGTORAD

        // Place it.
        dGeomSetRotation( (dGeomID)node->g_mls, R );
        dGeomSetPosition( (dGeomID)node->g_mls, pos[0], pos[1], pos[2] );

        // set geom data (move to its own method)
        mars::sim::geom_data* gd = new mars::sim::geom_data;
        (*gd).setZero();
        gd->sense_contact_force = GD_SENSE_CONTACT_FORCE;
        gd->parent_geom = GD_PARENT_GEOM;
        gd->c_params.cfm = GD_C_PARAMS_CFM;
        gd->c_params.erp = GD_C_PARAMS_ERP;
        gd->c_params.bounce = GD_C_PARAMS_BOUNCE;
        dGeomSetData((dGeomID)node->g_mls, gd);

        node->movable = false;	

        return node;
      }


      void EnvireMls::addMLSNode()
      {
        // TODO for loading various MLSs.
        // If the frame where the MLS should be
        // stored does not exists, create it by now we assume that the frame to
        // add to is the default one for the mls, created in the init step
        NodeData* node = setUpNodeData();
        envire::core::Item<NodeData>::Ptr itemPtr(new envire::core::Item<NodeData>(*node));
        envire::core::FrameId mlsFrameId = MLS_FRAME_NAME;
        control->graph->addItemToFrame(mlsFrameId, itemPtr);        
      }

      void EnvireMls::testAddMLS()
      {
#ifdef DEBUG
        LOG_DEBUG( "[EnvireMls::addMLS] 1"); 
#endif
        loadMLSMap(TEST_MLS_PATH);
        // Next is to instantiate a load the correspondent nodeData
        addMLSNode();
        
#ifdef DEBUG
        LOG_DEBUG( "[EnvireMls::addMLS] 2"); 
#endif
      }

    } // end of namespace envire_mls
  } // end of namespace plugins
} // end of namespace mars

DESTROY_LIB(mars::plugins::envire_mls::EnvireMls);
CREATE_LIB(mars::plugins::envire_mls::EnvireMls);