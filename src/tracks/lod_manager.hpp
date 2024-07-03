//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2004-2015 Steve Baker <sjbaker1@airmail.net>
//  Copyright (C) 2010-2015 Steve Baker, Joerg Henrichs
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#ifndef HEADER_LOD_MANAGER_HPP
#define HEADER_LOD_MANAGER_HPP

#include "graphics/lod_node.hpp"

#include "IMesh.h"

#include <vector>

/**
  * \brief Manager for automatically deciding level of registered LOD nodes
  * \ingroup graphics
  */
class LODManager
{
private:

    std::vector<LODNode*> m_lod_nodes;

    std::vector<float> m_importance;

    std::vector<std::vector<int> > m_children_triangle_count;

    int m_max_capacity;

    static const int ObjectCapacityOffset = 8;
    static const int OverallCapacityOffset = 16384;

public:
    // All children of this LODNode must have valid triangle count.
    void registerNode(LODNode* node, float importance, int *children_triangle_count);

    void autoComputeLevel();

    void clear();
};   // MaterialManager

extern LODManager *lod_manager;

#endif

/* EOF */
