
#include "tw_faces/face_AppList.h"
#include "fonts/RobotoMono_Light_All.h"
#include "fonts/RobotoMono_Regular_All.h"
#include "tinywatch.h"

// Apps
class tw_app;

/// @brief container holding all of the registered apps via their App name (Dictionary)
static std::map<String, tw_app *> app_icons;

/**
 * @brief Called when this face is set to be the current face
 *
 */
void FaceAppList::setup()
{
	if (!is_setup)
	{
		is_setup = true;
		// Add any one time setup code here
	}
}

/**
 * @brief Add an app to the std::map holding all of the apps by name
 *
 * @param app
 */
void FaceAppList::add_app(tw_app *app)
{
	app_icons[app->name] = app;
}

/**
 * @brief Check to see if an icon has been touched/clicked
 *
 * @param touch_pos_x
 * @param touch_pos_y
 * @return true
 * @return false
 */
bool FaceAppList::icon_process_clicks(uint16_t touch_pos_x, uint16_t touch_pos_y)
{
	for (auto _app : app_icons)
	{
		if (_app.second->click_icon(touch_pos_x, touch_pos_y))
		{
			current_app = _app.second;
			current_app->pre_start();
			prevent_dragging(true);
			next_update = 0;
			return true;
		}
	}

	return false;
}

/**
 * @brief Find a way to make the apps animate in and out nicely, so it's not so fugly.
 *
 * @return true
 * @return false
 */
bool FaceAppList::animate_app_in()
{
	is_animating = true;
	// Let's ramp up the CPU clock speed to make the animation buttery smooth.
	setCpuFrequencyMhz(160);
	float step = 0.0;

	// grab the selected icon ps and size so we can animate it
	current_app->get_icon_pos(anim_icon_pos_x, anim_icon_pos_y, anim_icon_width, anim_icon_height);

	anim_backlight = display.get_current_backlight_val();

	while (step < 1.0)
	{

		step = constrain(step + 0.2, 0.0, 1.0);
		anim_icon_height = (uint16_t)linear_lerp((float)anim_icon_height, 279.0, step);
		anim_icon_width = (uint16_t)linear_lerp((float)anim_icon_width, 239.0, step);
		anim_icon_pos_x = (uint16_t)linear_lerp((float)anim_icon_pos_x, 0.0, step);
		anim_icon_pos_y = (uint16_t)linear_lerp((float)anim_icon_pos_y, 0.0, step);
		anim_corner_roundness = (uint8_t)linear_lerp((float)anim_corner_roundness, 45.0, step);
		anim_backlight = (uint8_t)linear_lerp((float)anim_backlight, 0.0, step);

		draw(true);

		display.set_backlight_val_direct(anim_backlight);
	}
	yield;
	is_animating = false;

	// Drop the CPU speed back to default
	setCpuFrequencyMhz(40);

	return false;
}

/**
 * @brief Draw the Face which also draws the App Icons
 *
 * @param force
 */
void FaceAppList::draw(bool force)
{
	// If we have launched an app, it becomes the thing we see, any messaging to the AppList Face goes to the app
	// So long as we are not currently animating the app in or out
	if (!is_animating && current_app != nullptr)
	{
		current_app->draw(force);

		if (display.get_current_backlight_val() == 0)
			display.set_backlight(0, true);
		return;
	}

	// No app running, so we treat the face like a normal face
	if (force || millis() - next_update > update_period)
	{
		setup();

		next_update = millis();

		if (!is_dragging || !is_cached)
		{
			if (is_dragging)
				is_cached = true;

			canvas[canvasid].fillSprite(themes.current().col_background_dull);
			canvas[canvasid].setTextDatum(MC_DATUM); // Middle, Center
			canvas[canvasid].setFreeFont(RobotoMono_Regular[15]);
			canvas[canvasid].setTextColor(TFT_GREEN);

			int8_t icon_spacing = 9;

			int16_t icon_x = icon_spacing + 5;
			int16_t icon_y = icon_spacing;

			for (auto _app : app_icons)
			{
				_app.second->draw_icon(canvasid, icon_x, icon_y);
				icon_x += 64 + icon_spacing;
				if (icon_x > ((64 + icon_spacing) * 3))
				{
					icon_x = icon_spacing + 5;
					icon_y += 64 + icon_spacing;
				}
			}
		}

		if (is_animating)
		{
			canvas[canvasid].fillRoundRect(anim_icon_pos_x, anim_icon_pos_y, anim_icon_width, anim_icon_height, anim_corner_roundness, 0);
			canvas[canvasid].drawRoundRect(anim_icon_pos_x, anim_icon_pos_y, anim_icon_width, anim_icon_height, anim_corner_roundness, themes.current().col_secondary);
		}

		draw_navigation(canvasid);

		update_screen();
	}
}

/**
 * @brief Process touches in the app via the passed in touch_event
 *
 * @param touch_event
 * @return true
 * @return false
 */
bool FaceAppList::process_touch(touch_event_t touch_event)
{
	if (touch_event.type == TOUCH_TAP)
	{
		if (current_app != nullptr)
		{
			bool was_clicked = (current_app->process_touch(touch_event));
			// info_printf("Clicked app %s result %d\n",current_app->name, was_clicked);
			return (was_clicked);
		}

		if (icon_process_clicks(touch_event.x, touch_event.y))
		{
			haptics.play_trigger(Triggers::EVENT);
			animate_app_in();
			return true;
		}
	}
	else if (touch_event.type == TOUCH_DOUBLE)
	{
		//
	}
	else if (touch_event.type == TOUCH_LONG)
	{
		if (current_app != nullptr)
		{
			display.set_backlight_val_direct(0);
			haptics.play_trigger(Triggers::LONGPRESS);
			BuzzerUI({{2000, 400}});
			while (display.get_current_backlight_val() > 0)
				yield;

			close_app();
			return true;
		}
	}
	else if (touch_event.type == TOUCH_SWIPE)
	{
		if (current_app != nullptr)
		{
			if (current_app->process_touch(touch_event))
				return true;
		}
	}

	return false;
}

/**
 * @brief Returns if there is a current app or not via nullptr check
 *
 * @return true
 * @return false
 */
bool FaceAppList::app_running()
{
	return (current_app != nullptr);
}

/**
 * @brief Closes the running app, calling pre_close on the app first, and then does all cleanup
 *
 *
 */
void FaceAppList::close_app()
{
	if (current_app != nullptr)
	{
		current_app->pre_close();
	}

	prevent_dragging(false);
	reset_cache_status();
	draw(true);
	display.set_backlight(0, true);
	tinywatch.pause_can_sleep();
	current_app = nullptr;
}

FaceAppList face_applist;