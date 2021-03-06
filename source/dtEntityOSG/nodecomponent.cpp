/*
* dtEntity Game and Simulation Engine
*
* This library is free software; you can redistribute it and/or modify it under
* the terms of the GNU Lesser General Public License as published by the Free
* Software Foundation; either version 2.1 of the License, or (at your option)
* any later version.
*
* This library is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
* FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
* details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this library; if not, write to the Free Software Foundation, Inc.,
* 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*
* Martin Scheffler
*/

#include <dtEntityOSG/nodecomponent.h>
#include <dtEntityOSG/groupcomponent.h>
#include <dtEntityOSG/layercomponent.h>
#include <dtEntityOSG/nodemaskvisitor.h>
#include <osg/NodeVisitor>
#include <osg/Group>

namespace dtEntityOSG
{

   ////////////////////////////////////////////////////////////////////////////////
   ////////////////////////////////////////////////////////////////////////////////
   const dtEntity::StringId NodeComponent::TYPE(dtEntity::SID("Node"));
   
   ////////////////////////////////////////////////////////////////////////////
   NodeComponent::NodeComponent()
      : mEntity(NULL)
      , mNode(new osg::Node())
      , mParentComponent(dtEntity::StringId())
   {
      GetNode()->setName("NodeComponent");
   }

   ////////////////////////////////////////////////////////////////////////////
   NodeComponent::NodeComponent(osg::Node* node)
      : mEntity(NULL)
      , mNode(node)
      , mParentComponent(dtEntity::StringId())
   {
   }
    
   ////////////////////////////////////////////////////////////////////////////
   NodeComponent::~NodeComponent()
   {
   }

   ////////////////////////////////////////////////////////////////////////////
   void NodeComponent::OnAddedToEntity(dtEntity::Entity& entity)
   {
      mEntity = &entity;

      // for picking
      mNode->setUserData(mEntity);
   }

   ////////////////////////////////////////////////////////////////////////////
   void NodeComponent::OnRemovedFromEntity(dtEntity::Entity& entity)
   {
      if(GetParentComponent() != dtEntity::StringId())
      {
         dtEntity::Component* comp;
         if(entity.GetComponent(GetParentComponent(), comp))
         {
            GroupComponent* grp = dynamic_cast<GroupComponent*>(comp);
            if(grp)
            {
               grp->GetAttachmentGroup()->removeChild(GetNode());
            }
         }
      }
      mEntity = NULL;
   }

   ////////////////////////////////////////////////////////////////////////////
   osg::Node* NodeComponent::GetNode() const
   {
      return mNode;
   }

   ////////////////////////////////////////////////////////////////////////////
   void NodeComponent::SetNode(osg::Node* node)
   {
      GroupComponent* parent = NULL;
      // remove from parent
      if(mNode.valid() && mEntity != NULL && GetParentComponent() != dtEntity::StringId())
      {
         Component* comp;
         if(mEntity->GetComponent(GetParentComponent(), comp))
         {
            if(GetParentComponent() == LayerComponent::TYPE)
            {
               static_cast<LayerComponent*>(comp)->SetAttachedComponent(dtEntity::StringId());
            }
            else
            {
               parent = static_cast<GroupComponent*>(comp);
               parent->GetAttachmentGroup()->removeChild(mNode);
            }
         }
         mNode = node;
         mNode->setUserData(mEntity);
         if(parent == NULL)
         {
            if(GetParentComponent() == LayerComponent::TYPE)
            {
               static_cast<LayerComponent*>(comp)->SetAttachedComponent(GetType());
            }
         }
         else
         {
            parent->GetAttachmentGroup()->addChild(mNode);
         }
      }
      else
      {
         mNode = node;
         mNode->setUserData(mEntity);
      }
   }

   ////////////////////////////////////////////////////////////////////////////
   void NodeComponent::SetNodeMask(unsigned int nodemask, bool recursive)
   {
      if(recursive)
      {
         NodeMaskVisitor nv(nodemask);
         GetNode()->accept(nv);
      }
      else
      {
         GetNode()->setNodeMask(nodemask);
      }
   }
  
   ////////////////////////////////////////////////////////////////////////////
   unsigned int NodeComponent::GetNodeMask() const
   {
      return GetNode()->getNodeMask();
   }
}
