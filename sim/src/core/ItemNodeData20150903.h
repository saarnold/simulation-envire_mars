/*
 *  Copyright 2015, DFKI GmbH Robotics Innovation Center
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
 *
 */

#ifndef ITEM_NODE_DATA_H
#define ITEM_NODE_DATA_H

#ifdef _PRINT_HEADER_
  #warning "ItemNodeData.h"
#endif

#include "ItemMars.h"
#include <mars/interfaces/NodeData.h>


#include <mars/utils/Mutex.h>
#include <mars/interfaces/graphics/GraphicsUpdateInterface.h>
#include <mars/interfaces/sim/ControlCenter.h>
//#include <mars/interfaces/sim/ItemManagerInterface.h>


using namespace envire::core;
using namespace std;

namespace mars {
  namespace sim {
    using namespace interfaces;	 
    class SimNode; 
    typedef std::map<interfaces::NodeId, SimNode*> NodeMap;
    	  
    class ItemNodeData : public ItemMars<mars::interfaces::NodeData>,
							public interfaces::GraphicsUpdateInterface {
      public:
        ItemNodeData(ControlCenter *c);
        int addItem();
        void ItemNodeData::updateItemDynamics(sReal calc_ms, bool physics_thread)        
      private:  
        interfaces::ControlCenter *control;
        int visual_rep; 
         

    };
  }
}

#endif  // ITEM_NODE_DATA_H
