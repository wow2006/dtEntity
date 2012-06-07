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

#include <dtEntity/osganimationcomponent.h>

#include <dtEntity/entity.h>
#include <dtEntity/entitymanager.h>
#include <dtEntity/layercomponent.h>
#include <dtEntity/mapcomponent.h>
#include <dtEntity/systemmessages.h>
#include <dtEntity/staticmeshcomponent.h>
#include <osg/Geode>
#include <osgAnimation/RigGeometry>
#include <osgAnimation/RigTransformHardware>
#include <osgAnimation/BoneMapVisitor>
#include <osgAnimation/MorphGeometry>
#include <osgDB/ReadFile>
#include <sstream>
#include <osg/ShapeDrawable>
#include <osgDB/FileUtils>

namespace dtEntity
{

   ////////////////////////////////////////////////////////////////////////////////
   struct AnimationManagerFinder : public osg::NodeVisitor
   {
       osg::ref_ptr<osgAnimation::BasicAnimationManager> _am;

       AnimationManagerFinder() : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN) {}

       void apply(osg::Node& node) {
           if (_am.valid())
               return;
           if (node.getUpdateCallback()) {
               osgAnimation::AnimationManagerBase* b = dynamic_cast<osgAnimation::AnimationManagerBase*>(node.getUpdateCallback());
               if (b) {
                   _am = new osgAnimation::BasicAnimationManager(*b);
                   node.removeUpdateCallback(_am);

                   return;
               }
           }
           traverse(node);
       }
   };

   ////////////////////////////////////////////////////////////////////////////////
   struct BoneFinder : public osg::NodeVisitor
   {
       std::string mName;
       osgAnimation::Bone* mBone;
       osg::Matrix mWorldToLocal;

       BoneFinder(const std::string& name)
          : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
          , mName(name)
          , mBone(NULL)
       {
       }

       void apply(osg::Transform& node) {
          osgAnimation::Bone* bone = dynamic_cast<osgAnimation::Bone*>(&node);
          if(bone != NULL && bone->getName() == mName)
          {
             mBone = bone;
             mWorldToLocal = osg::computeWorldToLocal(getNodePath());
          }
          traverse(node);
       }
   };


   osg::ref_ptr<osg::Program> program;

   ////////////////////////////////////////////////////////////////////////////////
   struct MyRigTransformHardware : public osgAnimation::RigTransformHardware
   {

      std::string mVertexShader;
      std::string mFragmentShader;

      MyRigTransformHardware(const std::string& vertshader, const std::string& fragshader)
         : mVertexShader(vertshader)
         , mFragmentShader(fragshader)
      {
      }

       void operator()(osgAnimation::RigGeometry& geom)
       {
           if (_needInit)
               if (!init(geom))
                   return;
           computeMatrixPaletteUniform(geom.getMatrixFromSkeletonToGeometry(), geom.getInvMatrixFromSkeletonToGeometry());
       }

       bool init(osgAnimation::RigGeometry& geom)
       {
           osg::Vec3Array* pos = dynamic_cast<osg::Vec3Array*>(geom.getVertexArray());
           if (!pos) {
               osg::notify(osg::WARN) << "RigTransformHardware no vertex array in the geometry " << geom.getName() << std::endl;
               return false;
           }

           if (!geom.getSkeleton()) {
               osg::notify(osg::WARN) << "RigTransformHardware no skeleting set in geometry " << geom.getName() << std::endl;
               return false;
           }

           osgAnimation::BoneMapVisitor mapVisitor;
           geom.getSkeleton()->accept(mapVisitor);
           osgAnimation::BoneMap bm = mapVisitor.getBoneMap();

           if (!createPalette(pos->size(),bm, geom.getVertexInfluenceSet().getVertexToBoneList()))
               return false;

           int attribIndex = 11;
           int nbAttribs = getNumVertexAttrib();

           // use a global program for all avatar
           if (!program.valid()) {
               program = new osg::Program;
               program->setName("HardwareSkinning");
               if (!_shader.valid())
               {
                   std::string shaderPath = osgDB::findDataFile(mVertexShader);
                   _shader = osg::Shader::readShaderFile(osg::Shader::VERTEX, shaderPath);

               }

               if (!_shader.valid()) {
                   osg::notify(osg::WARN) << "RigTransformHardware can't load VertexShader" << std::endl;
                   return false;
               }

               // replace max matrix by the value from uniform
               {
                   std::string str = _shader->getShaderSource();
                   std::string toreplace = std::string("MAX_MATRIX");
                   std::size_t start = str.find(toreplace);
                   std::stringstream ss;
                   ss << getMatrixPaletteUniform()->getNumElements();
                   str.replace(start, toreplace.size(), ss.str());
                   _shader->setShaderSource(str);
                   osg::notify(osg::INFO) << "Shader " << str << std::endl;
               }

               program->addShader(_shader.get());

               std::string shaderPath2 = osgDB::findDataFile(mFragmentShader);
               osg::Shader* shader2 = osg::Shader::readShaderFile(osg::Shader::FRAGMENT, shaderPath2);
               if(shader2)
               {
                  program->addShader(shader2);
               }

               for (int i = 0; i < nbAttribs; i++)
               {
                   std::stringstream ss;
                   ss << "boneWeight" << i;
                   program->addBindAttribLocation(ss.str(), attribIndex + i);

                   osg::notify(osg::INFO) << "set vertex attrib " << ss.str() << std::endl;
               }
           }
           for (int i = 0; i < nbAttribs; i++)
           {
               std::stringstream ss;
               ss << "boneWeight" << i;
               geom.setVertexAttribData(attribIndex + i, osg::Geometry::ArrayData(getVertexAttrib(i),osg::Geometry::BIND_PER_VERTEX));
           }

           osg::ref_ptr<osg::StateSet> ss = geom.getOrCreateStateSet();
           ss->addUniform(getMatrixPaletteUniform());
           ss->addUniform(new osg::Uniform("nbBonesPerVertex", getNumBonesPerVertex()));
           ss->setAttributeAndModes(program.get());

          // geom.setStateSet(ss.get());
           _needInit = false;
           return true;
       }

   };

   ////////////////////////////////////////////////////////////////////////////////
   ////////////////////////////////////////////////////////////////////////////////
   struct SetupRigGeometry : public osg::NodeVisitor
   {
      std::string mVertexShader;
      std::string mFragmentShader;
       SetupRigGeometry(const std::string& vertexShader, const std::string& fragmentShader)
         : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
         , mVertexShader(vertexShader)
         , mFragmentShader(fragmentShader)
       {
       }

       void apply(osg::Geode& geode)
       {
           for (unsigned int i = 0; i < geode.getNumDrawables(); i++)
           {
              apply(*geode.getDrawable(i));
           }
       }

       void apply(osg::Drawable& geom)
       {          
           osgAnimation::RigGeometry* rig = dynamic_cast<osgAnimation::RigGeometry*>(&geom);
           if (rig)
           {
              rig->setRigTransformImplementation(new MyRigTransformHardware(mVertexShader, mFragmentShader));
           }           
       }
   };


   ////////////////////////////////////////////////////////////////////////////////
   ////////////////////////////////////////////////////////////////////////////////
   const StringId OSGAnimationComponent::TYPE(dtEntity::SID("OSGAnimation"));
   const StringId OSGAnimationComponent::EnabledId(dtEntity::SID("Enabled"));
   
   ////////////////////////////////////////////////////////////////////////////
   OSGAnimationComponent::OSGAnimationComponent()
      : mEntity(NULL)
   {
      Register(EnabledId, &mEnabled);
      mEnabled.Set(true);
   }


   ////////////////////////////////////////////////////////////////////////////
   OSGAnimationComponent::~OSGAnimationComponent()
   {
   }

   ////////////////////////////////////////////////////////////////////////////
   void OSGAnimationComponent::OnAddedToEntity(Entity& entity)
   {
      mEntity = &entity;
      StaticMeshComponent* smc;
      if(mEntity->GetComponent(smc))
      {
         SetupMesh(smc->GetNode());
      }
   }

   ////////////////////////////////////////////////////////////////////////////
   void OSGAnimationComponent::OnPropertyChanged(StringId propname, Property& prop)
   {
      if(propname == EnabledId)
      {
         SetEnabled(prop.BoolValue());
      }
   }

   ////////////////////////////////////////////////////////////////////////////
   bool OSGAnimationComponent::AddAttachment(const std::string& boneName, osg::Node* node, const osg::Matrix& m)
   {
      StaticMeshComponent* smc;
      if(!mEntity->GetComponent(smc, true))
      {
         LOG_ERROR("Cannot attach, no static mesh component found!");
         return false;
      }
      BoneFinder f("Bip01 R Finger1Nub");
      smc->GetNode()->accept(f);
      if(f.mBone == NULL)
      {
         LOG_ERROR("Cannot attach, bone not found: " + boneName);
         return false;
      }
      else
      {
         osg::MatrixTransform* mt = new osg::MatrixTransform();
         osg::Vec3 scale = f.mWorldToLocal.getScale();
         osg::Matrix sm; sm.makeScale(scale);
         mt->setMatrix(m * sm);
         mt->addChild(node);
         f.mBone->addChild(mt);
      }
      return true;
   }

   ////////////////////////////////////////////////////////////////////////////
   bool OSGAnimationComponent::RemoveAtachment(const std::string& boneName, osg::Node* node)
   {
      StaticMeshComponent* smc;
      if(!mEntity->GetComponent(smc, true))
      {
         LOG_ERROR("Cannot attach, no static mesh component found!");
         return false;
      }
      BoneFinder f("Bip01 R Finger1Nub");
      smc->GetNode()->accept(f);
      if(f.mBone == NULL)
      {
         LOG_ERROR("Cannot remove, bone not found: " + boneName);
         return false;
      }
      else
      {
         for(unsigned int i = 0; i < f.mBone->getNumChildren(); ++i)
         {
            osg::MatrixTransform* mt = dynamic_cast<osg::MatrixTransform*>(f.mBone->getChild(i));
            if(mt && mt->getNumChildren() > 0 && mt->getChild(0) == node)
            {
               f.mBone->removeChild(mt);
               return true;
            }
         }
      }
      return false;
   }

   ////////////////////////////////////////////////////////////////////////////
   void OSGAnimationComponent::RemoveAtachments(const std::string& boneName)
   {
      StaticMeshComponent* smc;
      if(!mEntity->GetComponent(smc, true))
      {
         LOG_ERROR("Cannot attach, no static mesh component found!");
         return;
      }
      BoneFinder f("Bip01 R Finger1Nub");
      smc->GetNode()->accept(f);
      if(f.mBone == NULL)
      {
         LOG_ERROR("Cannot remove, bone not found: " + boneName);
      }
      else
      {
         std::list<osg::MatrixTransform*> toRemove;
         for(unsigned int i = 0; i < f.mBone->getNumChildren(); ++i)
         {
            osg::MatrixTransform* mt = dynamic_cast<osg::MatrixTransform*>(f.mBone->getChild(i));
            if(mt)
            {
               toRemove.push_back(mt);
            }
         }
         for(std::list<osg::MatrixTransform*>::iterator i = toRemove.begin(); i != toRemove.end(); ++i)
         {
            f.mBone->removeChild(*i);
         }
      }
   }

   ////////////////////////////////////////////////////////////////////////////
   void OSGAnimationComponent::SetupMesh(osg::Node* node)
   {
      AnimationManagerFinder finder;
      node->accept(finder);
      mAnimationManager = finder._am;
      
      if(mAnimationManager == NULL)
      {
         return;
      }

      OSGAnimationSystem* sys;
      mEntity->GetEntityManager().GetEntitySystem(TYPE, sys);
      SetupRigGeometry switcher(sys->GetVertexShader(), sys->GetFragmentShader());
      node->accept(switcher);     
      SetEnabled(mEnabled.Get());

   }


   class EmptyCB : public osg::NodeCallback
   {
   public:
      virtual void operator()(osg::Node* node, osg::NodeVisitor* nv) {}
   };

   ////////////////////////////////////////////////////////////////////////////
   void OSGAnimationComponent::SetEnabled(bool v)
   {
      mEnabled.Set(v);

      if(mAnimationManager)
      {         
         StaticMeshComponent* smc;
         if(mEntity->GetComponent(smc, true))
         {
            OSGAnimationSystem* sys;
            mEntity->GetEntityManager().GetEntitySystem(TYPE, sys);
            bool reallyEnabled = mEnabled.Get() && sys->GetEnabled();

            if(reallyEnabled)
            {
               smc->GetNode()->setUpdateCallback(mAnimationManager);
            }
            else
            {
               smc->GetNode()->setUpdateCallback(new EmptyCB());
            }
         }         
      }
   }

   ////////////////////////////////////////////////////////////////////////////
   bool OSGAnimationComponent::GetEnabled() const
   {
      return mEnabled.Get();
   }

   ////////////////////////////////////////////////////////////////////////////
   const StringId OSGAnimationSystem::TYPE(dtEntity::SID("OSGAnimation"));
   const StringId OSGAnimationSystem::VertexShaderId(dtEntity::SID("VertexShader"));
   const StringId OSGAnimationSystem::FragmentShaderId(dtEntity::SID("FragmentShader"));
   const StringId OSGAnimationSystem::EnabledId(dtEntity::SID("Enabled"));
   
   ////////////////////////////////////////////////////////////////////////////
   OSGAnimationSystem::OSGAnimationSystem(EntityManager& em)
     : DefaultEntitySystem<OSGAnimationComponent>(em)
   {
      Register(VertexShaderId, &mVertexShader);
      Register(FragmentShaderId, &mFragmentShader);
      Register(EnabledId, &mEnabled);

      mVertexShader.Set("shaders/osganimationskinning.vert");
      mFragmentShader.Set("shaders/osganimationskinning.frag");
      mEnabled.Set(true);

      mMeshChangedFunctor = MessageFunctor(this, &OSGAnimationSystem::OnMeshChanged);
      em.RegisterForMessages(MeshChangedMessage::TYPE, mMeshChangedFunctor,
                            FilterOptions::ORDER_DEFAULT, "OSGAnimationSystem::OnMeshChanged");
      AddScriptedMethod("playAnimation", ScriptMethodFunctor(this, &OSGAnimationSystem::ScriptPlayAnimation));
      AddScriptedMethod("stopAnimation", ScriptMethodFunctor(this, &OSGAnimationSystem::ScriptStopAnimation));
      AddScriptedMethod("getAnimations", ScriptMethodFunctor(this, &OSGAnimationSystem::ScriptGetAnimations));
      AddScriptedMethod("getAnimationLength", ScriptMethodFunctor(this, &OSGAnimationSystem::ScriptGetAnimationLength));
      AddScriptedMethod("getAnimationPlayMode", ScriptMethodFunctor(this, &OSGAnimationSystem::ScriptGetAnimationPlayMode));
      AddScriptedMethod("setAnimationPlayMode", ScriptMethodFunctor(this, &OSGAnimationSystem::ScriptSetAnimationPlayMode));
      AddScriptedMethod("addAttachment", ScriptMethodFunctor(this, &OSGAnimationSystem::ScriptAddAttachment));
      AddScriptedMethod("removeAttachments", ScriptMethodFunctor(this, &OSGAnimationSystem::ScriptRemoveAttachments));
   }

   ////////////////////////////////////////////////////////////////////////////
   OSGAnimationSystem::~OSGAnimationSystem()
   {
      GetEntityManager().UnregisterForMessages(MeshChangedMessage::TYPE, mMeshChangedFunctor);
   }

   ////////////////////////////////////////////////////////////////////////////
   void OSGAnimationSystem::OnPropertyChanged(StringId propname, Property& prop)
   {
      if(propname == EnabledId)
      {
         SetEnabled(prop.BoolValue());
      }
   }

   ////////////////////////////////////////////////////////////////////////////
   void OSGAnimationSystem::SetEnabled(bool v)
   {
      mEnabled.Set(v);
      for(ComponentStore::iterator i = mComponents.begin(); i != mComponents.end(); ++i)
      {
         OSGAnimationComponent* comp = i->second;
         comp->SetEnabled(comp->GetEnabled());
      }

   }

   ////////////////////////////////////////////////////////////////////////////
   bool OSGAnimationSystem::GetEnabled() const
   {
      return mEnabled.Get();
   }

   ////////////////////////////////////////////////////////////////////////////
   bool OSGAnimationSystem::GetAnimationList(EntityId id, const osgAnimation::AnimationList*& list, osgAnimation::BasicAnimationManager*& manager)
   {

      OSGAnimationComponent* comp;
      if(!GetEntityManager().GetComponent(id, comp))
      {
         LOG_ERROR("No OSGAnimationComponent found for this entity id");
         return false;
      }
      manager = comp->GetAnimationManager();
      if(manager == NULL)
      {
         return false;
      }

      list = &manager->getAnimationList();
      return !list->empty();
   }

   ////////////////////////////////////////////////////////////////////////////
   void OSGAnimationSystem::OnMeshChanged(const Message& m)
   {
     const MeshChangedMessage& msg = static_cast<const MeshChangedMessage&>(m);
     EntityId id = msg.GetAboutEntityId();
     OSGAnimationComponent* comp = GetComponent(id);
     if(comp)
     {
       StaticMeshComponent* smc;
       if(GetEntityManager().GetComponent(id, smc, true))
       {
          comp->SetupMesh(smc->GetNode());
       }
     }
   }

   ////////////////////////////////////////////////////////////////////////////
   Property* OSGAnimationSystem::ScriptPlayAnimation(const PropertyArgs& args)
   {
      if(args.size() < 2)
      {
         LOG_ERROR("Usage: playAnimation(entityid, name,[priority = 0, weight = 1])");
         return NULL;
      }
      float priority = (args.size() > 2) ? args[2]->FloatValue() : 0;
      float weight = (args.size() > 3) ? args[3]->FloatValue() : 1;
      EntityId id = args[0]->UIntValue();
      std::string name = args[1]->StringValue();

      osgAnimation::BasicAnimationManager* manager;
      const osgAnimation::AnimationList* list;
      if(!GetAnimationList(id, list, manager))
      {
         return NULL;
      }

      for(osgAnimation::AnimationList::const_iterator i = list->begin(); i != list->end(); ++i)
      {
         if((*i)->getName() == name)
         {
            (manager)->playAnimation(*i, priority, weight);
             return NULL;
         }
      }

      return NULL;
   }

   ////////////////////////////////////////////////////////////////////////////
   Property* OSGAnimationSystem::ScriptStopAnimation(const PropertyArgs& args)
   {
      if(args.size() < 2)
      {
         LOG_ERROR("Usage: stopAnimation(entityid, name)");
         return NULL;
      }
      EntityId id = args[0]->UIntValue();
      std::string name = args[1]->StringValue();

      osgAnimation::BasicAnimationManager* manager;
      const osgAnimation::AnimationList* list;
      if(!GetAnimationList(id, list, manager))
      {
         return NULL;
      }

      for(osgAnimation::AnimationList::const_iterator i = list->begin(); i != list->end(); ++i)
      {
         if((*i)->getName() == name)
         {
             manager->stopAnimation(*i);
             return NULL;
         }
      }

      return NULL;
   }

   ////////////////////////////////////////////////////////////////////////////
   Property* OSGAnimationSystem::ScriptGetAnimations(const PropertyArgs& args)
   {
      if(args.size() != 1)
      {
         LOG_ERROR("Usage: getAnimations(entityid)");
         return NULL;
      }

      EntityId id = args[0]->UIntValue();

      osgAnimation::BasicAnimationManager* manager;
      const osgAnimation::AnimationList* list;
      if(!GetAnimationList(id, list, manager))
      {
         return NULL;
      }

      ArrayProperty* arr = new ArrayProperty();

      for(osgAnimation::AnimationList::const_iterator i = list->begin(); i != list->end(); ++i)
      {
         arr->Add(new StringProperty((*i)->getName()));
      }
      return arr;
   }

   ////////////////////////////////////////////////////////////////////////////
   Property* OSGAnimationSystem::ScriptGetAnimationLength(const PropertyArgs& args)
   {
      if(args.size() < 2)
      {
         LOG_ERROR("Usage: getAnimationLength(entityid, name)");
         return NULL;
      }
      EntityId id = args[0]->UIntValue();
      std::string name = args[1]->StringValue();

      osgAnimation::BasicAnimationManager* manager;
      const osgAnimation::AnimationList* list;
      if(!GetAnimationList(id, list, manager))
      {
         return NULL;
      }

      for(osgAnimation::AnimationList::const_iterator i = list->begin(); i != list->end(); ++i)
      {
         if((*i)->getName() == name)
         {
            (*i)->computeDuration();
            return new dtEntity::DoubleProperty((*i)->getDuration());
         }
      }
      return NULL;
   }

   ////////////////////////////////////////////////////////////////////////////
   Property* OSGAnimationSystem::ScriptGetAnimationPlayMode(const PropertyArgs& args)
   {

      if(args.size() < 2)
      {
         LOG_ERROR("Usage: getAnimationPlayMode(entityid, name)");
         return NULL;
      }
      EntityId id = args[0]->UIntValue();
      std::string name = args[1]->StringValue();

      osgAnimation::BasicAnimationManager* manager;
      const osgAnimation::AnimationList* list;
      if(!GetAnimationList(id, list, manager))
      {
         return NULL;
      }

      for(osgAnimation::AnimationList::const_iterator i = list->begin(); i != list->end(); ++i)
      {
         if((*i)->getName() == name)
         {
            osgAnimation::Animation::PlayMode pm = (*i)->getPlayMode();
            switch(pm)
            {
            case osgAnimation::Animation::LOOP: return new dtEntity::StringProperty("LOOP");
            case osgAnimation::Animation::ONCE: return new dtEntity::StringProperty("ONCE");
            case osgAnimation::Animation::STAY: return new dtEntity::StringProperty("STAY");
            case osgAnimation::Animation::PPONG: return new dtEntity::StringProperty("PPONG");
            }
         }         
      }
      return NULL;
   }

   ////////////////////////////////////////////////////////////////////////////
   Property* OSGAnimationSystem::ScriptSetAnimationPlayMode(const PropertyArgs& args)
   {
      // WARNING: THIS METHOD SEEMS TO SCREW UP RUNNING ANIMATIONS SOMEHOW,
      // MAYBE A THREAD ISSUE!!!


      if(args.size() < 3)
      {
         LOG_ERROR("Usage: setAnimationPlayMode(entityid, name, mode=[LOOP|ONCE|STAY|PPONG])");
         return NULL;
      }
      EntityId id = args[0]->UIntValue();
      std::string name = args[1]->StringValue();
      std::string mode = args[2]->StringValue();

      osgAnimation::BasicAnimationManager* manager;
      const osgAnimation::AnimationList* list;
      if(!GetAnimationList(id, list, manager))
      {
         return NULL;
      }

      manager->stopAll();

      for(osgAnimation::AnimationList::const_iterator i = list->begin(); i != list->end(); ++i)
      {
         osg::ref_ptr<osgAnimation::Animation> anim = *i;
         if(anim->getName() == name)
         {
            manager->unregisterAnimation(anim);
            if(mode == "LOOP")
            {
               anim->setPlayMode(osgAnimation::Animation::LOOP);
            }
            else if(mode == "ONCE")
            {
               anim->setPlayMode(osgAnimation::Animation::ONCE);
            }
            else if(mode == "STAY")
            {
               anim->setPlayMode(osgAnimation::Animation::STAY);
            }
            else if(mode == "PPONG")
            {
               anim->setPlayMode(osgAnimation::Animation::PPONG);
            }
            else
            {
               LOG_ERROR("Unknown animation play mode: " + mode);
            }
            manager->registerAnimation(anim);
            anim->computeDuration();
         }


      }
      return NULL;
   }

   ////////////////////////////////////////////////////////////////////////////
   Property* OSGAnimationSystem::ScriptAddAttachment(const PropertyArgs& args)
   {

      if(args.size() < 3)
      {
         LOG_ERROR("Usage: addAttachment(entityid, bone name, mesh path, [vec3 trans, quat rot])");
         return NULL;
      }
      EntityId id = args[0]->UIntValue();
      std::string bonename = args[1]->StringValue();
      std::string meshpath = args[2]->StringValue();
      osg::Vec3 trans;
      osg::Quat rot(0,0,0,1);

      if(args.size() > 3)
      {
         trans = args[3]->Vec3Value();
      }

      if(args.size() > 4)
      {
         rot = args[4]->QuatValue();
      }

      osg::Matrix mat;
      mat.setTrans(trans);
      mat.setRotate(rot);

      OSGAnimationComponent* comp = GetComponent(id);
      if(comp == NULL)
      {
         LOG_ERROR("addAttachment: No animation component found!");
         return NULL;
      }

      osg::Node* node = osgDB::readNodeFile(meshpath);

      if(node == NULL)
      {
         LOG_ERROR("addAttachment: Mesh not found at " << meshpath);
         return NULL;
      }
      bool success = comp->AddAttachment(bonename, node, mat);
      if(!success)
      {
         LOG_ERROR("Can't attach mesh to bone " << bonename);
      }

      return NULL;
   }

   ////////////////////////////////////////////////////////////////////////////
   Property* OSGAnimationSystem::ScriptRemoveAttachments(const PropertyArgs& args)
   {

      if(args.size() < 2)
      {
         LOG_ERROR("Usage: removeAttachments(entityid, bone name)");
         return NULL;
      }
      EntityId id = args[0]->UIntValue();
      std::string bonename = args[1]->StringValue();

      OSGAnimationComponent* comp = GetComponent(id);
      if(comp == NULL)
      {
         LOG_ERROR("removeAttachments: No animation component found!");
      }
      comp->RemoveAtachments(bonename);

      return NULL;
   }
}
