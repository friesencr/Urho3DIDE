#pragma once




#include "EditorPlugin.h"



namespace Urho3D
{
	class Window;

	class EPScene2D : public EditorPlugin
	{
		OBJECT(EPScene2D);
	public:
		/// Construct.
		EPScene2D(Context* context);
		/// Destruct.
		virtual ~EPScene2D();
		/// Register object factory.
		static void RegisterObject(Context* context);

		virtual bool HasMainScreen() override;

		virtual String GetName() const override;

		virtual void Edit(Object *object) override;

		virtual bool Handles(Object *object) const override;

		virtual UIElement* GetMainScreen() override;

		virtual void SetVisible(bool visible) override;

	protected:
		SharedPtr<Window> window_;

	};
}