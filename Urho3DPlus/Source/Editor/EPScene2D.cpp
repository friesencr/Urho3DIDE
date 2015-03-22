
#include "../Urho3D.h"
#include "EPScene2D.h"
#include "../Core/Context.h"
#include "../Scene/Component.h"
#include "../Scene/Node.h"
#include "../Scene/Scene.h"
#include "../UI/Window.h"
#include "EditorData.h"
#include "../UI/Text.h"
#include "../UI/UIElement.h"


namespace Urho3D
{



	EPScene2D::EPScene2D(Context* context) : EditorPlugin(context)
	{

	}

	EPScene2D::~EPScene2D()
	{

	}

	void EPScene2D::RegisterObject(Context* context)
	{
		context->RegisterFactory<EPScene2D>();
	}

	bool EPScene2D::HasMainScreen()
	{
		return true;
	}

	Urho3D::String EPScene2D::GetName() const
	{
		return String("2DView");
	}

	void EPScene2D::Edit(Object *object)
	{

	}

	bool EPScene2D::Handles(Object *object) const
	{
		return false;
	}

	UIElement* EPScene2D::GetMainScreen()
	{
		if (!window_)
		{
			EditorData* editorData_ = GetSubsystem<EditorData>();
			window_ = new Window(context_);
			window_->SetFixedSize(200, 200);
			window_->SetStyleAuto(editorData_->GetDefaultStyle());
			Text* hello = window_->CreateChild<Text>();
			hello->SetText("hello 2d view ... ");
			hello->SetStyleAuto(editorData_->GetDefaultStyle());
		}
		return window_;
	}

	void EPScene2D::SetVisible(bool visible)
	{
		visible_ = visible;
	}

}