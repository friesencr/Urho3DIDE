#include "../Urho3D.h"
#include "../Core/Context.h"

#include "Editor.h"

#include "../Resource/ResourceCache.h"
#include "../UI/UI.h"
#include "../Graphics/Graphics.h"
#include "../Input/InputEvents.h"
#include "../UI/Window.h"
#include "../UI/UIElement.h"
#include "../UI/UIEvents.h"
#include "../Scene/Scene.h"
#include "../UI/View3D.h"
#include "../Scene/Node.h"
#include "../Graphics/Camera.h"
#include "../Graphics/Zone.h"
#include "../Math/BoundingBox.h"
#include "../Math/Color.h"
#include "../Math/Quaternion.h"
#include "../Graphics/StaticModel.h"
#include "../Graphics/Model.h"
#include "../Graphics/Material.h"
#include "../Graphics/Light.h"
#include "../Graphics/Octree.h"
#include "../UI/UIElement.h"
#include "../Resource/Resource.h"
#include "../Container/Str.h"
#include "../UI/FileSelector.h"
#include "../Resource/XMLFile.h"

#include "Editor/EditorSelection.h"
#include "UI/HierarchyWindow.h"
#include "UI/AttributeInspector.h"
#include "UI/MenuBarUI.h"
#include "UI/ToolBarUI.h"
#include "UI/MiniToolBarUI.h"
#include "UI/UIUtils.h"
#include "EditorView.h"
#include "TabWindow.h"
#include "AttributeContainer.h"
#include "ResourcePicker.h"
#include "EditorData.h"
#include "../Graphics/Texture2D.h"
#include "EditorPlugin.h"
#include "EPScene3D.h"
#include "EPScene2D.h"
#include "../Scene/Component.h"
#include "../Graphics/DebugRenderer.h"
#include "../Graphics/Octree.h"
#include "../Core/CoreEvents.h"
#include "../UI/ListView.h"


namespace Urho3D
{


void Editor::RegisterObject(Context* context)
{
	context->RegisterFactory<Editor>();
	EPScene3DView::RegisterObject(context);
	EPScene3D::RegisterObject(context);
	EPScene2D::RegisterObject(context);

	EditorData::RegisterObject(context);
	EditorView::RegisterObject(context);
	EditorSelection::RegisterObject(context);
}

Editor::~Editor()
{

}

Editor::Editor(Context* context) : Object(context),
visible_(false),
editorPluginMain_(NULL),
editorPluginOver_(NULL)
{


}

bool Editor::Create(Scene* scene, UIElement* sceneUI)
{
	cache_ = GetSubsystem<ResourceCache>();
	ui_ = GetSubsystem<UI>();
	graphics_ = GetSubsystem<Graphics>();

	/// create editable scene and ui
	sceneRootUI_ = sceneUI;
	if (!scene)
	{
		scene_ = new Scene(context_);		
	}else
	scene_ = scene;
	scene_->GetOrCreateComponent<Octree>();
	scene_->GetOrCreateComponent<DebugRenderer>();
	// Allow access to the scene from the console
	//script.defaultScene = editorScene;
	// Always pause the scene, and do updates manually
	scene_->SetUpdateEnabled(true);


	if (!GetSubsystem<ResourcePickerManager>())
	{
		/// ResourcePickerManager is needed for the Attribute Inspector, so don't forget to init it
		context_->RegisterSubsystem(new ResourcePickerManager(context_));
		GetSubsystem<ResourcePickerManager>()->Init();
	}

	context_->RegisterSubsystem(new EditorData(context_));
	editorData_ = GetSubsystem<EditorData>();
	editorData_->Load();

	rootUI_ = editorData_->rootUI_;

	editorData_->SetEditorScene(scene_);
	context_->RegisterSubsystem(new EditorView(context_, editorData_));
	editorView_ = GetSubsystem<EditorView>();

	MenuBarUI* menubar = editorView_->GetGetMenuBar();

	menubar->CreateMenu("File");
	menubar->CreateMenuItem("File", "Quit", A_QUITEDITOR_VAR);

	SubscribeToEvent(editorView_->GetGetMenuBar(), E_MENUBAR_ACTION, HANDLER(Editor, HandleMenuBarAction));


	context_->RegisterSubsystem(new EditorSelection(context_,this));
	editorSelection_ = GetSubsystem<EditorSelection>();


	//////////////////////////////////////////////////////////////////////////
	/// create the hierarchy editor
	hierarchyWindow_ = new HierarchyWindow(context_);
	hierarchyWindow_->SetResizable(true);
	hierarchyWindow_->SetIconStyle(editorData_->iconStyle_);
	hierarchyWindow_->SetTitle("Scene Hierarchy");
	hierarchyWindow_->SetDefaultStyle(editorData_->defaultStyle_);
	hierarchyWindow_->SetStyleAuto();
	/// \todo
	// dont know why the auto style does not work ...
	hierarchyWindow_->SetTexture(cache_->GetResource<Texture2D>("Textures/UI.png"));
	hierarchyWindow_->SetImageRect(IntRect(112, 0, 128, 16));
	hierarchyWindow_->SetBorder(IntRect(2, 2, 2, 2));
	hierarchyWindow_->SetResizeBorder(IntRect(0, 0, 0, 0));
	hierarchyWindow_->SetLayoutSpacing(0);
	hierarchyWindow_->SetLayoutBorder(IntRect(0, 4, 0, 0));
	/// remove the title bar from the window 
	hierarchyWindow_->SetTitleBarVisible(false);

	SubscribeToEvent(hierarchyWindow_->GetHierarchyList(), E_SELECTIONCHANGED, HANDLER(Editor, HandleHierarchyListSelectionChange));
	SubscribeToEvent(hierarchyWindow_->GetHierarchyList(), E_ITEMDOUBLECLICKED, HANDLER(Editor, HandleHierarchyListDoubleClick));

	/// add Hierarchy inspector to the left side of the editor.
	editorView_->GetLeftFrame()->AddTab("Hierarchy",hierarchyWindow_);
	/// connect the hierarchy with the editable scene.
	hierarchyWindow_->SetScene(scene_);
	/// connect the hierarchy with the editable  ui.
	hierarchyWindow_->SetUIElement(sceneUI);

	//////////////////////////////////////////////////////////////////////////
	/// create the attribute editor
	attributeWindow_ = new AttributeInspector(context_);
	Window* atrele = (Window*)attributeWindow_->Create();
	atrele->SetResizable(false);
	atrele->SetMovable(false);
	/// remove the title bar from the window 
	UIElement* titlebar = atrele->GetChild("TitleBar", true);
	if (titlebar)
		titlebar->SetVisible(false);
	/// add Attribute inspector to the right side of the editor.
	editorView_->GetRightFrame()->AddTab("Inspector", atrele);


	SubscribeToEvent(editorView_->GetMiddleFrame(), E_ACTIVETABCHANGED, HANDLER(Editor, HandleMainEditorTabChanged));

	AddEditorPlugin(new EPScene3D(context_));
	AddEditorPlugin(new EPScene2D(context_));

	SubscribeToEvent(E_UPDATE, HANDLER(Editor, HandleUpdate));
	visible_ = true;
	return true;
}


void Editor::AddEditorPlugin(EditorPlugin* plugin)
{
	if (plugin->HasMainScreen()) 
	{
		// push fist because tabwindow send tabchanged event on first add and that activates the first plugin.
		mainEditorPlugins_.Push(plugin); 
		editorView_->GetMiddleFrame()->AddTab(plugin->GetName(), plugin->GetMainScreen());	
	}
	editorData_->AddEditorPlugin(plugin);
}

void Editor::RemoveEditorPlugin(EditorPlugin* plugin)
{
	if (plugin->HasMainScreen()) 
	{
		editorView_->GetMiddleFrame()->RemoveTab(plugin->GetName());
		mainEditorPlugins_.Remove(plugin);
	}
	editorData_->RemoveEditorPlugin(plugin);
}

Scene* Editor::GetScene()
{
	return scene_;
}

void Editor::SetScene(Scene* scene)
{
	scene_ = scene;
}

void Editor::SetSceneUI(UIElement* sceneUI)
{
	sceneRootUI_ = sceneUI;
}

Component* Editor::GetListComponent(UIElement* item)
{
	if (scene_.Null())
		return NULL;

	if (item == NULL)
		return NULL;

	if (item->GetVar(TYPE_VAR).GetInt() != ITEM_COMPONENT)
		return NULL;

	return scene_->GetComponent(item->GetVar(COMPONENT_ID_VAR).GetUInt());
}

Node* Editor::GetListNode(UIElement* item)
{
	if (scene_.Null())
		return NULL;

	if (item == NULL)
		return NULL;

	if (item->GetVar(TYPE_VAR).GetInt() != ITEM_NODE)
		return NULL;

	return scene_->GetNode(item->GetVar(NODE_ID_VAR).GetUInt());
}

UIElement* Editor::GetListUIElement(UIElement*  item)
{
	if (scene_.Null())
		return NULL;

	if (item == NULL)
		return NULL;

	// Get the text item's ID and use it to retrieve the actual UIElement the text item is associated to
	return GetUIElementByID(UIUtils::GetUIElementID(item));
}

UIElement* Editor::GetUIElementByID(const Variant& id)
{
	return id == UI_ELEMENT_BASE_ID ? NULL : sceneRootUI_->GetChild(UI_ELEMENT_ID_VAR, id, true);
}

void Editor::CreateFileSelector(const String& title, const String& ok, const String& cancel, const String& initialPath, Vector<String>& filters, unsigned int initialFilter)
{
	// Within the editor UI, the file selector is a kind of a "singleton". When the previous one is overwritten, also
	// the events subscribed from it are disconnected, so new ones are safe to subscribe.
	uiFileSelector_ = new  FileSelector(context_);
	uiFileSelector_->SetDefaultStyle(cache_->GetResource<XMLFile>("UI/DefaultStyle.xml"));
	uiFileSelector_->SetTitle(title);
	uiFileSelector_->SetPath(initialPath);
	uiFileSelector_->SetButtonTexts(ok, cancel);
	uiFileSelector_->SetFilters(filters, initialFilter);

	IntVector2 size = uiFileSelector_->GetWindow()->GetSize();
	uiFileSelector_->GetWindow()->SetPosition((graphics_->GetWidth() - size.x_) / 2, (graphics_->GetHeight() - size.y_) / 2);
}

void Editor::CloseFileSelector(unsigned int& filterIndex, String& path)
{
	// Save filter & path for next time
	filterIndex = uiFileSelector_->GetFilterIndex();
	path = uiFileSelector_->GetPath();

	uiFileSelector_ = NULL;
}

void Editor::CloseFileSelector()
{
	uiFileSelector_ = NULL;
}

FileSelector* Editor::GetUIFileSelector()
{
	return uiFileSelector_;
}

void Editor::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
	using namespace Update;
	float timestep = eventData[P_TIMESTEP].GetFloat();
	if (editorPluginMain_)
	{
		editorPluginMain_->Update(timestep);
	}
	if (editorPluginOver_)
	{
		editorPluginOver_->Update(timestep);
	}
}

void Editor::HandleMenuBarAction(StringHash eventType, VariantMap& eventData)
{
	using namespace MenuBarAction;

	StringHash action = eventData[P_ACTION].GetStringHash();
	if (action == A_QUITEDITOR_VAR)
	{
		
	}
	else if (action == A_SHOWHIERARCHY_VAR)
	{
	
	}
	else if (action == A_SHOWATTRIBUTE_VAR)
	{

	}
}

void Editor::HandleMainEditorTabChanged(StringHash eventType, VariantMap& eventData)
{
	using namespace ActiveTabChanged;

	unsigned index = eventData[P_TABINDEX].GetUInt();

	if (index >= mainEditorPlugins_.Size())
		return; // error ...

	EditorPlugin *new_editor = mainEditorPlugins_[index];
	if (!new_editor)
		return; // error

	if (editorPluginMain_ == new_editor)
		return; // do nothing

	if (editorPluginMain_) 
		editorPluginMain_->SetVisible(false);
	
	editorPluginMain_ = new_editor;
	editorPluginMain_->SetVisible(true);
	//editorPluginMain_->selectedNotify();
}

void Editor::HandleHierarchyListSelectionChange(StringHash eventType, VariantMap& eventData)
{
	editorSelection_->OnHierarchyListSelectionChange(hierarchyWindow_->GetHierarchyList()->GetItems(), hierarchyWindow_->GetHierarchyList()->GetSelections());
	/// \todo dont copy 
	attributeWindow_->GetEditNodes() = editorSelection_->GetEditNodes();
	attributeWindow_->GetEditComponents() = editorSelection_->GetEditComponents();
	attributeWindow_->GetEditUIElements() = editorSelection_->GetEditUIElements();
	attributeWindow_->Update();

	// 	OnSelectionChange();
// 
// 	// 		PositionGizmo();
// 	UpdateAttributeInspector();
// 	// 		UpdateCameraPreview();
}

void Editor::HandleHierarchyListDoubleClick(StringHash eventType, VariantMap& eventData)
{
	using namespace ItemDoubleClicked;

	UIElement* item = dynamic_cast<UIElement*>(eventData[P_ITEM].GetPtr());
	/// \todo 
}

}