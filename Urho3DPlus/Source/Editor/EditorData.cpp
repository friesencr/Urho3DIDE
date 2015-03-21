
#include "../Urho3D.h"
#include "../Core/Context.h"
#include "EditorData.h"

#include "../Core/CoreEvents.h"
#include "../UI/DropDownList.h"
#include "../Engine/EngineEvents.h"
#include "../UI/Font.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/GraphicsEvents.h"
#include "../Input/Input.h"
#include "../Input/InputEvents.h"
#include "../IO/IOEvents.h"
#include "../UI/LineEdit.h"
#include "../UI/ListView.h"
#include "../IO/Log.h"
#include "../Resource/ResourceCache.h"
#include "../UI/ScrollBar.h"
#include "../UI/Text.h"
#include "../UI/UI.h"
#include "../UI/UIEvents.h"
#include "../Scene/Scene.h"
#include "../Graphics/Texture.h"
#include "../Graphics/Texture2D.h"
#include "../Resource/XMLFile.h"
#include "Editor.h"
#include "EditorPlugin.h"


namespace Urho3D
{


	EditorData::EditorData(Context* context) : Object(context)
	{
	
	}

	EditorData::~EditorData()
	{

	}

	void EditorData::RegisterObject(Context* context)
	{
		context->RegisterFactory<EditorData>("Editor");
	}

	void EditorData::Load()
	{
		/// \todo load from settings file ...
		ResourceCache* cache = GetSubsystem<ResourceCache>();
		UI* ui_ = GetSubsystem<UI>();
		defaultStyle_ = cache->GetResource<XMLFile>("UI/IDEStyle.xml");
		iconStyle_ = cache->GetResource<XMLFile>("UI/IDEIcons.xml");

		rootUI_ = ui_->GetRoot()->CreateChild<UIElement>("EditorRoot");
		rootUI_->SetSize(ui_->GetRoot()->GetSize());
		rootUI_->SetTraversalMode(TM_DEPTH_FIRST);     // This is needed for root-like element to prevent artifacts
		rootUI_->SetDefaultStyle(defaultStyle_);
		rootUI_->SetLayout(LM_VERTICAL);
	
	}

	void EditorData::SetGlobalVarNames(const String& name)
	{
		globalVarNames_[name] = name;
	}

	const Variant& EditorData::GetGlobalVarNames(StringHash& name)
	{
		return globalVarNames_[name];
	}

	Scene* EditorData::GetEditorScene()
	{
		return scene_;
	}

	void EditorData::SetEditorScene(Scene* scene)
	{
		scene_ = scene;
	}

	EditorPlugin* EditorData::GetEditor(Object *object)
	{
		for (unsigned i = 0; i < editorPlugins_.Size(); i++)
		{

			if (editorPlugins_[i]->HasMainScreen() && editorPlugins_[i]->Handles(object))
				return editorPlugins_[i];
		}

		return NULL;
	}

	EditorPlugin* EditorData::GetEditor(const String& name)
	{
		for (unsigned i = 0; i < editorPlugins_.Size(); i++) 
		{

			if (editorPlugins_[i]->GetName() == name)
				return editorPlugins_[i];
		}

		return NULL;
	}

	EditorPlugin* EditorData::GetSubeditor(Object *object)
	{
		for (unsigned i = 0; i < editorPlugins_.Size(); i++)
		{

			if (!editorPlugins_[i]->HasMainScreen() && editorPlugins_[i]->Handles(object))
				return editorPlugins_[i];
		}

		return NULL;
	}

	void EditorData::AddEditorPlugin(EditorPlugin* plugin)
	{
		//p_plugin->undo_redo = &undo_redo;
		editorPlugins_.Push(SharedPtr<EditorPlugin>(plugin));
	}

	void EditorData::RemoveEditorPlugin(EditorPlugin* plugin)
	{
		//p_plugin->undo_redo = NULL;
		editorPlugins_.Remove(SharedPtr<EditorPlugin>(plugin));
	}

}