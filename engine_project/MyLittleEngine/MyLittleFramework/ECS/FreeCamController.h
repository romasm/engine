#pragma once

#include "ControllerSystem.h"

namespace EngineCore
{
	class FreeCamController: public Controller
	{
	public:
		FreeCamController();

		float rot_speed;
		float move_speed;

		void SetPlayerStand(float x, float y, float z);

		ALIGNED_ALLOCATION

	protected:
		void Process();
		void GetInput(ControllerComands cmd, float param1, float param2);
		
		XMVECTOR forward, rightward, upward;

		bool move_forward;
		bool move_backward;
		bool move_left;
		bool move_right;
		bool move_down;
		bool move_up;

		POINT rot_coords;
	};
}
