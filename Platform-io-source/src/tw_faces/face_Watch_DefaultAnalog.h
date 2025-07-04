#pragma once

#include "tw_faces/tw_face.h"

class FaceWatch_DefaultAnalog : public tw_face
{
	public:
		void setup(void);
		void draw(bool force);

		bool process_touch(touch_event_t touch_event);

	private:
		void setup_trig(void);
		void draw_hand(int x, int y, int x1, int y1, uint16_t color);
		void live_recalc_xy(uint8_t mins, uint8_t old_len, uint8_t new_len, int *new_x, int *new_y);
		String version = "1.0";
		bool cachedTrig = false;

		float pos_secs[60][2];
		// float pos_mins[60][2];
		// float pos_hours[12][2];

		int cached_mins = -1;
		int h_x = 0;
		int h_y = 0;
		int m_x = 0;
		int m_y = 0;

		int center_x = 120;
		int center_y = 131;
		float face_radius = 107;
};

extern FaceWatch_DefaultAnalog face_watch_default_analog;
