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

#pragma once

#include <QtGui/QtGui>
#include <dtEntity/messagepump.h>
#include <set>

namespace dtEntityQtWidgets
{
   class OSGGraphicsWindowQt;
}

namespace dtEntityEditor
{
   class EditorApplication;
   
   ////////////////////////////////////////////////////////////////////////////////
   class ResizerWidget : public QWidget
   {
      Q_OBJECT

   public:
      ResizerWidget(QWidget* parent = 0, Qt::WindowFlags f = 0);
      void SetWidget(QWidget* widget);
      void EmitCurrentSize();
   signals:

      void Resized(const QSize&);
      void Closing();
      void LoadScene(const QString&);

   protected:

      virtual void resizeEvent(QResizeEvent* evt);
 
   };

   ////////////////////////////////////////////////////////////////////////////////
   class EditorMainWindow 
      : public QMainWindow
   {
     Q_OBJECT

   public:

      EditorMainWindow(EditorApplication* app, QWidget *parent = 0);
      virtual ~EditorMainWindow();

      void OnToolsUpdated(const dtEntity::Message& msg);
      void OnMapLoaded(const dtEntity::Message& msg);
      void OnMapUnloaded(const dtEntity::Message& msg);
      void OnSceneLoaded(const dtEntity::Message& msg);
      void OnSceneUnloaded(const dtEntity::Message& msg);

      QStringList GetLoadedMaps() const;

   signals:      

      void LoadScene(const QString& path);
      void UnloadScene();
      void NewScene();
      void Closed(bool);
      void ViewResized(const QSize& size);
      void ViewClosing();
      void DataPathsChanged(const QStringList&);

      void SaveScene(const QString& path);

      void TextDroppedOntoGLWidget(const QPointF& pos, const QString&);

      void MapLoaded(const QString& map);
      void MapUnloaded(const QString& map);

   public slots:

      void OnDisplayError(const QString& msg);
      void SetOSGWindow(dtEntityQtWidgets::OSGGraphicsWindowQt*);

      void OnViewResized(const QSize& size);
      void OnViewClosing();

      // interpret line in jump tool bar and do jump
      void DoJump();


      void OnTextDroppedOntoGLWidget(const QPointF& pos, const QString&);
      void ShutDown();

      void CreateEntityTree();
      void CreateSpawners();
      void CreatePropertyEditor();
      void AddToKnownComponentList(std::set<dtEntity::ComponentType> newTypes);

   protected slots:

      void OnToggleCoordSystem(bool v);
      void OnToggleGroundClamp(bool v);
      void OnToolActionTriggered(bool);
      void OnChooseScene();
      void OnNewScene();
      void OnChooseDataPaths();
      void OnSaveScene();
      void OnSaveSceneAs();
      void OnAddPlugin();
      void EmitQueuedMessages();

   protected:
      virtual void closeEvent(QCloseEvent* e);
      void ReadSettings();
      void WriteSettings();
      void createActions();
      void createMenus();
      void createToolBars();
      void CreateDockWidgets();


   private:

      EditorApplication* mApplication;

      QMenu* mFileMenu;
      QMenu* mViewMenu;

      QToolBar* mToolsToolbar;
      QActionGroup* mToolsActionGroup;

      QAction* mChangeDataPathsAct;
      QAction* mChangeSceneAct;
      QAction* mNewSceneAct;
      QAction* mSaveSceneAct;
      QAction* mSaveSceneAsAct;
      QAction* mAddPluginAct;
      QAction* mExitAct;

      // line edit for jump line in tool box
      QLineEdit* mJumpEdit;

      // The scene currently being edited
      QString mCurrentScene;

      QSet<QString> mCurrentMaps;

      dtEntity::MessagePump mMessagePump;
      QTimer* mUpdateTimer;

      QDockWidget* mEntityTreeDock;
      QDockWidget* mSpawnersDock;
      QDockWidget* mPropertyEditorDock;
   };

}

