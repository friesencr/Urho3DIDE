#pragma once

#include "EditorPlugin.h"
#include "..\UI\BorderImage.h"
#include "..\Math\Color.h"
#include "UIGlobals.h"
#include "..\UI\UIElement.h"

namespace Urho3D
{
	enum MouseOrbitMode
	{
		ORBIT_RELATIVE = 0,
		ORBIT_WRAP
	};

	enum EditMode
	{
		EDIT_MOVE = 0,
		EDIT_ROTATE,
		EDIT_SCALE,
		EDIT_SELECT,
		EDIT_SPAWN
	};

	enum AxisMode
	{
		AXIS_WORLD = 0,
		AXIS_LOCAL
	};

	enum SnapScaleMode
	{
		SNAP_SCALE_FULL = 0,
		SNAP_SCALE_HALF,
		SNAP_SCALE_QUARTER
	};

	class Window;
	class View3D;
	class Camera;
	class Node;
	class Scene;
	class CustomGeometry;
	class Texture2D;
	class Viewport;
	class EditorData;
	class EditorView;
	class EditorSelection;
	class DebugRenderer;
	class UI;
	class Input;
	class SoundListener;
	class Renderer;
	class ResourceCache;
	class Text;
	class Font;
	class LineEdit;
	class CheckBox;
	class XMLFile;

	class EPScene3D;

	class EPScene3DView : public BorderImage
	{
		OBJECT(EPScene3DView);
		friend class EPScene3D;
	public:
		/// Construct.
		EPScene3DView(Context* context);
		/// Destruct.
		virtual ~EPScene3DView();
		/// Register object factory.
		static void RegisterObject(Context* context);

		virtual void Update(float timeStep) override;
		/// React to resize.
		virtual void OnResize() override;
		/// React to mouse hover.
		virtual void OnHover(const IntVector2& position, const IntVector2& screenPosition, int buttons, int qualifiers, Cursor* cursor) override;
		/// React to mouse click begin.
		virtual void OnClickBegin(const IntVector2& position, const IntVector2& screenPosition, int button, int buttons, int qualifiers, Cursor* cursor) override;
		/// React to mouse click end.
		virtual void OnClickEnd(const IntVector2& position, const IntVector2& screenPosition, int button, int buttons, int qualifiers, Cursor* cursor, UIElement* beginElement) override;

		/// Define the scene and camera to use in rendering. When ownScene is true the View3D will take ownership of them with shared pointers.
		void SetView(Scene* scene, bool ownScene = true);
		/// Set render texture pixel format. Default is RGB.
		void SetFormat(unsigned format);
		/// Set render target auto update mode. Default is true.
		void SetAutoUpdate(bool enable);
		/// Queue manual update on the render texture.
		void QueueUpdate();

		/// Return render texture pixel format.
		unsigned GetFormat() const { return rttFormat_; }
		/// Return whether render target updates automatically.
		bool GetAutoUpdate() const { return autoUpdate_; }
		/// Return scene.
		Scene* GetScene() const;
		/// Return camera scene node.
		Node* GetCameraNode() const;
		/// Return render texture.
		Texture2D* GetRenderTexture() const;
		/// Return depth stencil texture.
		Texture2D* GetDepthTexture() const;
		/// Return viewport.
		Viewport* GetViewport() const;

		Camera* GetCamera() const { return camera_; }
		float	GetYaw() const { return cameraYaw_; }
		float	GetPitch() const { return cameraPitch_; }

		void ResetCamera();
		void ReacquireCameraYawPitch();
		void CreateViewportContextUI(XMLFile* uiStyle, XMLFile* iconStyle_);
	protected:
		/// Reset scene.
		void ResetScene();

		void ToggleOrthographic();

		void SetOrthographic(bool orthographic);

		void HandleResize();

		void HandleSettingsLineEditTextChange(StringHash eventType, VariantMap& eventData);
		void HandleOrthographicToggled(StringHash eventType, VariantMap& eventData);
		void ToggleViewportSettingsWindow(StringHash eventType, VariantMap& eventData);
		void ResetCamera(StringHash eventType, VariantMap& eventData);
		void CloseViewportSettingsWindow(StringHash eventType, VariantMap& eventData);
		void UpdateSettingsUI(StringHash eventType, VariantMap& eventData);
		void OpenViewportSettingsWindow();

		/// Renderable texture.
		SharedPtr<Texture2D> renderTexture_;
		/// Depth stencil texture.
		SharedPtr<Texture2D> depthTexture_;
		/// Viewport.
		SharedPtr<Viewport> viewport_;
		/// Scene.
		SharedPtr<Scene> scene_;
		/// Camera scene node.
		SharedPtr<Node>		cameraNode_;
		/// Camera
		SharedPtr<Camera>	camera_;
		/// SoundListener
		SharedPtr<SoundListener> soundListener_;
		/// 
		float cameraYaw_;
		/// 
		float cameraPitch_;
		/// Own scene.
		bool ownScene_;
		/// Render texture format.
		unsigned rttFormat_;
		/// Render texture auto update mode.
		bool autoUpdate_;
		/// ui stuff
		SharedPtr<UIElement>statusBar;
		SharedPtr<Text> cameraPosText;

		SharedPtr<Window> settingsWindow;
		SharedPtr<LineEdit> cameraPosX;
		SharedPtr<LineEdit> cameraPosY;
		SharedPtr<LineEdit> cameraPosZ;
		SharedPtr<LineEdit> cameraRotX;
		SharedPtr<LineEdit> cameraRotY;
		SharedPtr<LineEdit> cameraRotZ;
		SharedPtr<LineEdit> cameraZoom;
		SharedPtr<LineEdit> cameraOrthoSize;
		SharedPtr<CheckBox> cameraOrthographic;
	};

	class EPScene3D : public EditorPlugin
	{
		OBJECT(EPScene3D);
	public:
		/// Construct.
		EPScene3D(Context* context);
		/// Destruct.
		virtual ~EPScene3D();
		/// Register object factory.
		static void RegisterObject(Context* context);


		virtual bool HasMainScreen() override;
		virtual String GetName() const override;
		virtual void Edit(Object *object) override;
		virtual bool Handles(Object *object) const override;
		/// calls Start, because EPScene3D is a main Editor plugin
		///	GetMainScreen will be called in AddEditorPlugin() once, so use it as Start().
		virtual UIElement* GetMainScreen() override;
		virtual void SetVisible(bool visible) override;
		virtual void Update(float timeStep) override;
		// debug handling
		void ToggleRenderingDebug()	{renderingDebug = !renderingDebug;	}
		void TogglePhysicsDebug(){physicsDebug = !physicsDebug;	}
		void ToggleOctreeDebug(){	octreeDebug = !octreeDebug;	}

		// camera handling
		void ResetCamera();
		void ReacquireCameraYawPitch();
		void UpdateViewParameters();
		// grid 
		void HideGrid();
		void ShowGrid();
	protected:
		void Start();

		void CreateStatsBar();
		void SetupStatsBarText(Text* text, Font* font, int x, int y, HorizontalAlignment hAlign, VerticalAlignment vAlign);
		void UpdateStats(float timeStep);

		Vector3 SelectedNodesCenterPoint();
		void	DrawNodeDebug(Node* node, DebugRenderer* debug, bool drawNode = true);
		/// edit nodes
		bool MoveNodes(Vector3 adjust);
		bool RotateNodes(Vector3 adjust);
		bool ScaleNodes(Vector3 adjust);

		/// Picking
		void ViewRaycast(bool mouseClick);

		/// mouse handling
		void SetMouseMode(bool enable);
		void SetMouseLock();
		void ReleaseMouseLock();

		/// Engine Events Handling
		void HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData);
		void ViewMouseClick(StringHash eventType, VariantMap& eventData);
		void ViewMouseMove(StringHash eventType, VariantMap& eventData);
		void ViewMouseClickEnd(StringHash eventType, VariantMap& eventData);
		void HandleBeginViewUpdate(StringHash eventType, VariantMap& eventData);
		void HandleEndViewUpdate(StringHash eventType, VariantMap& eventData);
		void HandleBeginViewRender(StringHash eventType, VariantMap& eventData);
		void HandleEndViewRender(StringHash eventType, VariantMap& eventData);
		/// Resize the view 
		void HandleResizeView(StringHash eventType, VariantMap& eventData);

		SharedPtr<UIElement>		window_;
		SharedPtr<EPScene3DView>	activeView;
		SharedPtr<Node>				cameraNode_;
		SharedPtr<Camera>			camera_;

		/// cache editor subsystems
		EditorData*	editorData_;
		EditorView*	editorView_;
		EditorSelection* editorSelection_;

		/// cache subsystems
		UI* ui_;
		Input* input_;
		Renderer* renderer;
		ResourceCache* cache_;

		///  properties \todo make an input edit state system or ... 
		// mouse handling
		bool	toggledMouseLock_;
		int		mouseOrbitMode;
		// scene update handling
		bool	runUpdate  = true;
		//camera handling
		float	cameraBaseSpeed = 10.0f;
		float	cameraBaseRotationSpeed = 0.2f;
		float	cameraShiftSpeedMultiplier = 5.0f;
		bool	mouseWheelCameraPosition = false;
		bool	orbiting = false;
		bool	limitRotation = false;
		float	viewNearClip = 0.1f;
		float	viewFarClip = 1000.0f;
		float	viewFov = 45.0f;
		// create node
		float	newNodeDistance = 20.0f;
		// edit input states 
		float	moveStep = 0.5f;
		float	rotateStep = 5.0f;
		float	scaleStep = 0.1f;
		float	snapScale = 1.0f;
		bool	moveSnap = false;
		bool	rotateSnap = false;
		bool	scaleSnap = false;
		// debug handling
		bool	renderingDebug = false;
		bool	physicsDebug = false;
		bool	octreeDebug = false;
		// mouse pick handling
		int		pickMode = PICK_GEOMETRIES;

		EditMode editMode = EDIT_MOVE;
		AxisMode axisMode = AXIS_WORLD;
		FillMode fillMode = FILL_SOLID;
		SnapScaleMode snapScaleMode = SNAP_SCALE_FULL;

		// ui stuff
		SharedPtr<Text> editorModeText;
		SharedPtr<Text> renderStatsText;

		//////////////////////////////////////////////////////////////////////////
		/// Grid handling \todo put it into a component or object ...

		void CreateGrid();
		void UpdateGrid(bool updateGridGeometry = true);

		SharedPtr<Node>			gridNode_;
		SharedPtr<CustomGeometry> grid_;
		bool showGrid_;
		bool grid2DMode_;
		Color gridColor;
		Color gridSubdivisionColor;
		Color gridXColor;
		Color gridYColor;
		Color gridZColor;
	};
}
