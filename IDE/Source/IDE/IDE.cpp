//
// Copyright (c) 2008-2015 the Sviga project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
#include "../Urho3D.h"
#include "IDE.h"

#include <Urho3D/Engine/Application.h>
#include <Urho3D/Urho3D.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Engine/Engine.h>

#ifdef MessageBox
#undef MessageBox
#endif

#include <Urho3D/UI/Font.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Engine/Console.h>
#include <Urho3D/UI/Cursor.h>
#include <Urho3D/Engine/DebugHud.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Input/InputEvents.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/UI/Sprite.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Core/Timer.h>
#include <Urho3D/Resource/XMLFile.h>
#include "../UI/UIElement.h"
#include "../Graphics/Zone.h"
#include "../Math/Color.h"
#include "../UI/Window.h"
#include "../IO/Log.h"
#include "../UI/Font.h"
#include "../UI/Button.h"
#include "../UI/UIEvents.h"
#include "../UI/ListView.h"
#include "../UI/FileSelector.h"
#include "../Graphics/Graphics.h"
#include "../UI/MessageBox.h"
#include "IDESettings.h"
#include "ProjectManager.h"
#include "ProjectWindow.h"
#include "UI/ModalWindow.h"
#include "UI/ResourcePicker.h"
#include "Editor/EditorSelection.h"
#include "UI/MenuBarUI.h"
#include "UI/ToolBarUI.h"
#include "UI/MiniToolBarUI.h"
#include "Editor/EditorPlugin.h"
#include "TemplateManager.h"
#include "Editor/Editor.h"
#include "Editor/EditorView.h"
#include "UI/UIGlobals.h"
#include "UI/TabWindow.h"
#include "UI/HierarchyWindow.h"

#include "../Resource/XMLFile.h"
#include "UI/AttributeInspector.h"
#include "../UI/View3D.h"
#include "../Graphics/StaticModel.h"
#include "../Scene/Scene.h"
#include "../Graphics/Octree.h"
#include "../Graphics/Model.h"
#include "../Graphics/Material.h"
#include "../Graphics/Light.h"
#include "../Scene/Node.h"

using namespace Urho3D;
DEFINE_APPLICATION_MAIN(IDE)
namespace Urho3D
{
	IDE::IDE(Context * context) : Application(context)
	{
		prjMng_ = NULL;
		cache_ = NULL;
		ui_ = NULL;
		console_ = NULL;
		debugHud_ = NULL;
		graphics_ = NULL;
	}

	void IDE::RegisterPlusLib()
	{
		ModalWindow::RegisterObject(context_);
		IDESettings::RegisterObject(context_);
		ProjectManager::RegisterObject(context_);
		ProjectSettings::RegisterObject(context_);
		Editor::RegisterObject(context_);
		ProjectWindow::RegisterObject(context_);
		ResourcePickerManager::RegisterObject(context_);

		MenuBarUI::RegisterObject(context_);
		ToolBarUI::RegisterObject(context_);
		MiniToolBarUI::RegisterObject(context_);

		TemplateManager::RegisterObject(context_);
		TabWindow::RegisterObject(context_);
	}

	void IDE::Setup()
	{
		RegisterPlusLib();

		context_->RegisterSubsystem(new IDESettings(context_));
		settings_ = GetSubsystem<IDESettings>();
		settings_->LoadConfigFile();

		engineParameters_ = settings_->ToVariantMap();
		// Modify engine startup parameters
		engineParameters_["LogName"] = GetTypeName() + ".log";//GetSubsystem<FileSystem>()->GetAppPreferencesDir("urho3d", "logs")
		//	engineParameters_["AutoloadPaths"] = "Data;CoreData;IDEData";
	}

	void IDE::Start()
	{
		// cache subsystems
		cache_ = GetSubsystem<ResourceCache>();
		ui_ = GetSubsystem<UI>();
		graphics_ = GetSubsystem<Graphics>();

		context_->RegisterSubsystem(new ProjectManager(context_));
		prjMng_ = GetSubsystem<ProjectManager>();
	
		// Get default style
		XMLFile* xmlFile = cache_->GetResource<XMLFile>("UI/DefaultStyle.xml");
		XMLFile* styleFile = cache_->GetResource<XMLFile>("UI/IDEStyle.xml");
		XMLFile* iconstyle = cache_->GetResource<XMLFile>("UI/IDEIcons.xml");

		ui_->GetRoot()->SetDefaultStyle(styleFile);

		// Create console
		console_ = engine_->CreateConsole();
		console_->SetDefaultStyle(xmlFile);
		console_->SetAutoVisibleOnError(true);

		// Create debug HUD.
		debugHud_ = engine_->CreateDebugHud();
		debugHud_->SetDefaultStyle(xmlFile);

		// Subscribe key down event
		SubscribeToEvent(E_KEYDOWN, HANDLER(IDE, HandleKeyDown));
		SubscribeToEvent(E_MENUBAR_ACTION, HANDLER(IDE, HandleMenuBarAction));
		SubscribeToEvent(E_OPENPROJECT, HANDLER(IDE, HandleOpenProject));

		// Create Cursor
		Cursor* cursor_ = new Cursor(context_);
		cursor_->SetStyleAuto(xmlFile);
		ui_->SetCursor(cursor_);
		if (GetPlatform() == "Android" || GetPlatform() == "iOS")
			ui_->GetCursor()->SetVisible(false);
		Input* input = GetSubsystem<Input>();
		// Use OS mouse without grabbing it
		input->SetMouseVisible(true);

		// create main ui
		rootUI_ = ui_->GetRoot()->CreateChild<UIElement>("IDERoot");
		rootUI_->SetSize(ui_->GetRoot()->GetSize());
		rootUI_->SetTraversalMode(TM_DEPTH_FIRST);     // This is needed for root-like element to prevent artifacts
		rootUI_->SetDefaultStyle(styleFile);


		editor_ = new Editor(context_);

		editor_->Create(NULL, NULL);

		
	//	ShowWelcomeScreen();
		
	}

	void IDE::Stop()
	{
		settings_->Save();
	}

	void IDE::CreateIdeEditor()
	{
// 		XMLFile* styleFile = cache_->GetResource<XMLFile>("UI/IDEStyle.xml");
// 		XMLFile* iconstyle = cache_->GetResource<XMLFile>("UI/IDEIcons.xml");
// 
// 		hierarchyWindow_ = new HierarchyWindow(context_);
// 		hierarchyWindow_->SetMovable(true);
// 		hierarchyWindow_->SetIconStyle(iconstyle);
// 		hierarchyWindow_->SetTitle("Scene Hierarchy");
// 		hierarchyWindow_->SetDefaultStyle(styleFile);
// 		hierarchyWindow_->SetStyleAuto();
// 		/// \todo
// 		// dont know why the auto style does not work ...
// 		hierarchyWindow_->SetTexture(cache_->GetResource<Texture2D>("Textures/UI.png"));
// 		hierarchyWindow_->SetImageRect(IntRect(112, 0, 128, 16));
// 		hierarchyWindow_->SetBorder(IntRect(2, 2, 2, 2));
// 		hierarchyWindow_->SetResizeBorder(IntRect(0, 0, 0, 0));
// 		hierarchyWindow_->SetLayoutSpacing(0);
// 		hierarchyWindow_->SetLayoutBorder(IntRect(0, 4, 0, 0));
// 		hierarchyWindow_->SetTitleBarVisible(false);
// 		hierarchyWindow_->SetSuppressSceneChanges(true);
// 
// 		attributeWindow_ = new AttributeInspector(context_);
// 		SharedPtr<UIElement> atrele(attributeWindow_->Create());
// 		UIElement* titlebar = atrele->GetChild("TitleBar", true);
// 
// 		if (titlebar)
// 			titlebar->SetVisible(false);
// 		Renderer* renderer = GetSubsystem<Renderer>();
// 		Zone* zone = renderer->GetDefaultZone();
// 
// 		attributeWindow_->GetEditComponents().Push(zone);
// 		attributeWindow_->Update();
// 
// 		editorView_->GetLeftFrame()->AddTab("Hierarchy", SharedPtr<UIElement>(hierarchyWindow_));
// 		editorView_->GetRightFrame()->AddTab("Attributes", atrele);
// 
// 		//////////////////////////////////////////////////////////////////////////
// 		ResourceCache* cache = GetSubsystem<ResourceCache>();
// 
// 		Scene* scene_ = new Scene(context_);
// 
// 		// Create the Octree component to the scene. This is required before adding any drawable components, or else nothing will
// 		// show up. The default octree volume will be from (-1000, -1000, -1000) to (1000, 1000, 1000) in world coordinates; it
// 		// is also legal to place objects outside the volume but their visibility can then not be checked in a hierarchically
// 		// optimizing manner
// 		scene_->CreateComponent<Octree>();
// 
// 		// Create a child scene node (at world origin) and a StaticModel component into it. Set the StaticModel to show a simple
// 		// plane mesh with a "stone" material. Note that naming the scene nodes is optional. Scale the scene node larger
// 		// (100 x 100 world units)
// 		Node* planeNode = scene_->CreateChild("Plane");
// 		planeNode->SetScale(Vector3(100.0f, 1.0f, 100.0f));
// 		StaticModel* planeObject = planeNode->CreateComponent<StaticModel>();
// 		planeObject->SetModel(cache->GetResource<Model>("Models/Plane.mdl"));
// 		planeObject->SetMaterial(cache->GetResource<Material>("Materials/StoneTiled.xml"));
// 
// 		// Create a directional light to the world so that we can see something. The light scene node's orientation controls the
// 		// light direction; we will use the SetDirection() function which calculates the orientation from a forward direction vector.
// 		// The light will use default settings (white light, no shadows)
// 		Node* lightNode = scene_->CreateChild("DirectionalLight");
// 		lightNode->SetDirection(Vector3(0.6f, -1.0f, 0.8f)); // The direction vector does not need to be normalized
// 		Light* light = lightNode->CreateComponent<Light>();
// 		light->SetLightType(LIGHT_DIRECTIONAL);
// 
// 		// Create more StaticModel objects to the scene, randomly positioned, rotated and scaled. For rotation, we construct a
// 		// quaternion from Euler angles where the Y angle (rotation about the Y axis) is randomized. The mushroom model contains
// 		// LOD levels, so the StaticModel component will automatically select the LOD level according to the view distance (you'll
// 		// see the model get simpler as it moves further away). Finally, rendering a large number of the same object with the
// 		// same material allows instancing to be used, if the GPU supports it. This reduces the amount of CPU work in rendering the
// 		// scene.
// 		const unsigned NUM_OBJECTS = 200;
// 		for (unsigned i = 0; i < NUM_OBJECTS; ++i)
// 		{
// 			Node* mushroomNode = scene_->CreateChild("Mushroom");
// 			mushroomNode->SetPosition(Vector3(Random(90.0f) - 45.0f, 0.0f, Random(90.0f) - 45.0f));
// 			mushroomNode->SetRotation(Quaternion(0.0f, Random(360.0f), 0.0f));
// 			mushroomNode->SetScale(0.5f + Random(2.0f));
// 			StaticModel* mushroomObject = mushroomNode->CreateComponent<StaticModel>();
// 			mushroomObject->SetModel(cache->GetResource<Model>("Models/Mushroom.mdl"));
// 			mushroomObject->SetMaterial(cache->GetResource<Material>("Materials/Mushroom.xml"));
// 		}
// 
// 		// Create a scene node for the camera, which we will move around
// 		// The camera will use default settings (1000 far clip distance, 45 degrees FOV, set aspect ratio automatically)
// 		Node* cameraNode_ = scene_->CreateChild("Camera");
// 		Camera* cam = cameraNode_->CreateComponent<Camera>();
// 
// 		// Set an initial position for the camera scene node above the plane
// 		cameraNode_->SetPosition(Vector3(0.0f, 5.0f, 0.0f));
// 
// 
// 		scene3DWindow_ = new View3D(context_);
// 		scene3DWindow_->SetView(scene_,cam);
// 		hierarchyWindow_->SetScene(scene_);
// 
// 		editorView_->GetMiddleFrame()->AddTab("View3D", SharedPtr<UIElement>(scene3DWindow_));
// 	
	}

	void IDE::CreateDefaultScene()
	{

	}

	void IDE::HandleQuitMessageAck(StringHash eventType, VariantMap& eventData)
	{
		using namespace MessageACK;

		bool ok_ = eventData[P_OK].GetBool();

		if (ok_)
			GetSubsystem<Engine>()->Exit();
	}

	void IDE::HandleKeyDown(StringHash eventType, VariantMap& eventData)
	{
		using namespace KeyDown;

		int key = eventData[P_KEY].GetInt();

		// Close console (if open) or exit when ESC is pressed
		if (key == KEY_ESC)
		{
			Console* console = GetSubsystem<Console>();
			if (console->IsVisible())
				console->SetVisible(false);
			else
				Quit();
		}

		// Toggle console with F1
		else if (key == KEY_F1)
			GetSubsystem<Console>()->Toggle();

		// Toggle debug HUD with F2
		else if (key == KEY_F2)
			GetSubsystem<DebugHud>()->ToggleAll();

		// Common rendering quality controls, only when UI has no focused element
		else if (!GetSubsystem<UI>()->GetFocusElement())
		{
			Renderer* renderer = GetSubsystem<Renderer>();

			// Texture quality
			if (key == '1')
			{
				int quality = renderer->GetTextureQuality();
				++quality;
				if (quality > QUALITY_HIGH)
					quality = QUALITY_LOW;
				renderer->SetTextureQuality(quality);
			}

			// Material quality
			else if (key == '2')
			{
				int quality = renderer->GetMaterialQuality();
				++quality;
				if (quality > QUALITY_HIGH)
					quality = QUALITY_LOW;
				renderer->SetMaterialQuality(quality);
			}

			// Specular lighting
			else if (key == '3')
				renderer->SetSpecularLighting(!renderer->GetSpecularLighting());

			// Shadow rendering
			else if (key == '4')
				renderer->SetDrawShadows(!renderer->GetDrawShadows());

			// Shadow map resolution
			else if (key == '5')
			{
				int shadowMapSize = renderer->GetShadowMapSize();
				shadowMapSize *= 2;
				if (shadowMapSize > 2048)
					shadowMapSize = 512;
				renderer->SetShadowMapSize(shadowMapSize);
			}

			// Shadow depth and filtering quality
			else if (key == '6')
			{
				int quality = renderer->GetShadowQuality();
				++quality;
				if (quality > SHADOWQUALITY_HIGH_24BIT)
					quality = SHADOWQUALITY_LOW_16BIT;
				renderer->SetShadowQuality(quality);
			}

			// Occlusion culling
			else if (key == '7')
			{
				bool occlusion = renderer->GetMaxOccluderTriangles() > 0;
				occlusion = !occlusion;
				renderer->SetMaxOccluderTriangles(occlusion ? 5000 : 0);
			}

			// Instancing
			else if (key == '8')
				renderer->SetDynamicInstancing(!renderer->GetDynamicInstancing());
		}
		// Take screenshot
		else if (key == '9')
		{
			Graphics* graphics = GetSubsystem<Graphics>();
			Image screenshot(context_);
			graphics->TakeScreenShot(screenshot);
			// Here we save in the Data folder with date and time appended
			screenshot.SavePNG(GetSubsystem<FileSystem>()->GetProgramDir() + "Data/Screenshot_" +
				Time::GetTimeStamp().Replaced(':', '_').Replaced('.', '_').Replaced(' ', '_') + ".png");
		}
	}

	void IDE::HandleMenuBarAction(StringHash eventType, VariantMap& eventData)
	{
		using namespace  MenuBarAction;

		StringHash action = eventData[P_ACTION].GetStringHash();

		if (action == A_QUITEDITOR_VAR)
		{
			Quit();
		}
	}

	void IDE::ShowWelcomeScreen()
	{
// 		welcomeUI_ = prjMng_->CreateWelcomeScreen();
// 		rootUI_->AddChild(welcomeUI_);
// 		//welcomeUI_->SetSize(rootUI_->GetSize());
// 		editorView_->SetToolBarVisible(false);
// 		editorView_->SetLeftFrameVisible(false);
// 		editorView_->SetRightFrameVisible(false);
// 		unsigned index = editorView_->GetMiddleFrame()->AddTab("Welcome", welcomeUI_);
// 		editorView_->GetMiddleFrame()->SetActiveTab(index);
	}

	void IDE::HandleOpenProject(StringHash eventType, VariantMap& eventData)
	{
// 		using namespace  OpenProject;
// 
// 		prjMng_->ShowWelcomeScreen(false);
// 
// 		editorView_->GetMiddleFrame()->RemoveTab("Welcome");
// 		editorView_->SetToolBarVisible(true);
// 		editorView_->SetLeftFrameVisible(true);
// 		editorView_->SetRightFrameVisible(true);
// 
// 		CreateIdeEditor();
	}

	void IDE::Quit()
	{
		SharedPtr<Urho3D::MessageBox> messageBox(new Urho3D::MessageBox(context_, "Do you really want to exit the Editor ?", "Quit Editor ?"));

		if (messageBox->GetWindow() != NULL)
		{
			Button* cancelButton = (Button*)messageBox->GetWindow()->GetChild("CancelButton", true);
			cancelButton->SetVisible(true);
			cancelButton->SetFocus(true);
			SubscribeToEvent(messageBox, E_MESSAGEACK, HANDLER(IDE, HandleQuitMessageAck));
		}

		messageBox->AddRef();
	}
}