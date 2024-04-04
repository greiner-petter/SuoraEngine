#pragma once
#include "ViewportPanel.h"
#include "Suora/Common/Map.h"
#include "ViewportModules.generated.h"

namespace Suora
{
	class World;
	class CameraNode;

	class ViewportCameraGizmo : public ViewportDebugGizmo
	{
		SUORA_CLASS(46783823473333);
	public:
		virtual void DrawDebugGizmos(World* world, CameraNode* camera, int* pickingID, Map<int, Node*>* pickingMap, bool isHandlingMousePick)  override;

	};

}